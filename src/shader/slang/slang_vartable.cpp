
#include "imports.h"
#include "slang_compile.h"
#include "slang_compile_variable.h"
#include "slang_vartable.h"
#include "slang_ir.h"
#include "prog_instruction.h"
#include <memory>
#include <vector>


static int dbg = 0;


enum TempState {
    FREE,
    VAR,
    TEMP
};


/**
 * Variable/register info for one variable scope.
 * C++17: Parent is a unique_ptr so the whole push-down stack is auto-freed.
 */
struct table {
    int Level{0};
    std::vector<slang_variable *> Vars;

    TempState Temps[MAX_PROGRAM_TEMPS * 4]{};  /* per-component state */
    int ValSize[MAX_PROGRAM_TEMPS]{};     /* For debug only */

    std::unique_ptr<table> Parent;  /**< Owned: next-lower scope table */
};


/**
 * A variable table is a stack of tables, one per scope.
 */
struct slang_var_table {
    GLint CurLevel{0};
    GLuint MaxRegisters{0};
    std::unique_ptr<table> Top;  /**< Table at top of stack (owns the chain) */
};



slang_var_table *
_slang_new_var_table(GLuint maxRegisters)
{
    auto *vt = new slang_var_table{};
    vt->MaxRegisters = maxRegisters;
    return vt;
}


void
_slang_delete_var_table(slang_var_table *vt)
{
    if (vt->Top) {
	_mesa_problem(nullptr, "non-empty var table in _slang_delete_var_table()");
	return;
    }
    delete vt;
}



/**
 * Create new table, push it as the new top of the stack.
 */
void
_slang_push_var_table(slang_var_table *vt)
{
    auto t = std::make_unique<table>();
    t->Level = vt->CurLevel++;
    if (vt->Top) {
	/* copy the info indicating which temp regs are in use */
	memcpy(t->Temps, vt->Top->Temps, sizeof(t->Temps));
	memcpy(t->ValSize, vt->Top->ValSize, sizeof(t->ValSize));
    }
    if (dbg) printf("Pushing level %d\n", t->Level);
    t->Parent = std::move(vt->Top);
    vt->Top = std::move(t);
}


/**
 * Destroy top table, restore to parent.
 */
void
_slang_pop_var_table(slang_var_table *vt)
{
    table *t = vt->Top.get();

    if (dbg) printf("Popping level %d\n", t->Level);

    /* free the storage allocated for each variable */
    for (slang_variable *v : t->Vars) {
	slang_ir_storage *store = static_cast<slang_ir_storage *>(v->aux);
	GLint j;
	GLuint comp;
	if (dbg) printf("  Free var %s, size %d at %d\n",
			    v->a_name, store->Size, store->Index);

	if (store->Size == 1)
	    comp = GET_SWZ(store->Swizzle, 0);
	else
	    comp = 0;

	assert(store->Index >= 0);
	for (j = 0; j < store->Size; j++) {
	    assert(t->Temps[store->Index * 4 + j + comp] == VAR);
	    t->Temps[store->Index * 4 + j + comp] = FREE;
	}
	store->Index = -1;
    }
    if (t->Parent) {
	/* just verify that any remaining allocations in this scope
	 * were for temps
	 */
	for (int i = 0; i < static_cast<int>(vt->MaxRegisters) * 4; i++) {
	    if (t->Temps[i] != FREE && t->Parent->Temps[i] == FREE) {
		if (dbg) printf("  Free reg %d\n", i/4);
		assert(t->Temps[i] == TEMP);
	    }
	}
    }

    /* Pop: transfer Top ownership to parent in one move; old Top destructs */
    vt->Top = std::move(vt->Top->Parent);
    vt->CurLevel--;
}


/**
 * Add a new variable to the given symbol table.
 */
void
_slang_add_variable(slang_var_table *vt, slang_variable *v)
{
    struct table *t;
    assert(vt);
    t = vt->Top.get();
    assert(t);
    if (dbg) printf("Adding var %s\n", v->a_name);
    t->Vars.push_back(v);
}


/**
 * Look for variable by name in given table.
 * If not found, Parent table will be searched.
 */
slang_variable *
_slang_find_variable(const slang_var_table *vt, slang_atom name)
{
    struct table *t = vt->Top.get();
    while (1) {
	int i;
	for (i = 0; i < static_cast<int>(t->Vars.size()); i++) {
	    if (t->Vars[i]->a_name == name)
		return t->Vars[i];
	}
	if (t->Parent)
	    t = t->Parent.get();
	else
	    return nullptr;
    }
}


/**
 * Allocation helper.
 * \param size  var size in floats
 * \return  position for var, measured in floats
 */
static GLint
alloc_reg(slang_var_table *vt, GLint size, bool isTemp)
{
    struct table *t = vt->Top.get();
    /* if size == 1, allocate anywhere, else, pos must be multiple of 4 */
    const GLuint step = (size == 1) ? 1 : 4;
    GLuint i, j;
    assert(size > 0); /* number of floats */

    for (i = 0; i <= vt->MaxRegisters * 4 - size; i += step) {
	GLuint found = 0;
	for (j = 0; j < size; j++) {
	    if (i + j < vt->MaxRegisters * 4 && t->Temps[i + j] == FREE) {
		found++;
	    } else {
		break;
	    }
	}
	if (found == size) {
	    /* found block of size free regs */
	    if (size > 1)
		assert(i % 4 == 0);
	    for (j = 0; j < size; j++)
		t->Temps[i + j] = isTemp ? TEMP : VAR;
	    t->ValSize[i] = size;
	    return i;
	}
    }
    return -1;
}


/**
 * Allocate temp register(s) for storing a variable.
 * \param size  size needed, in floats
 * \param swizzle  returns swizzle mask for accessing var in register
 * \return  register allocated, or -1
 */
bool
_slang_alloc_var(slang_var_table *vt, slang_ir_storage *store)
{
    struct table *t = vt->Top.get();
    const int i = alloc_reg(vt, store->Size, false);
    if (i < 0)
	return false;

    store->Index = i / 4;
    if (store->Size == 1) {
	const GLuint comp = i % 4;
	store->Swizzle = MAKE_SWIZZLE4(comp, comp, comp, comp);
	if (dbg) printf("Alloc var sz %d at %d.%c (level %d)\n",
			    store->Size, store->Index, "xyzw"[comp], t->Level);
    } else {
	store->Swizzle = SWIZZLE_NOOP;
	if (dbg) printf("Alloc var sz %d at %d.xyzw (level %d)\n",
			    store->Size, store->Index, t->Level);
    }
    return true;
}



/**
 * Allocate temp register(s) for storing an unnamed intermediate value.
 */
bool
_slang_alloc_temp(slang_var_table *vt, slang_ir_storage *store)
{
    struct table *t = vt->Top.get();
    const int i = alloc_reg(vt, store->Size, true);
    if (i < 0)
	return false;

    store->Index = i / 4;
    if (store->Size == 1) {
	const GLuint comp = i % 4;
	store->Swizzle = MAKE_SWIZZLE4(comp, comp, comp, comp);
	if (dbg) printf("Alloc temp sz %d at %d.%c (level %d)\n",
			    store->Size, store->Index, "xyzw"[comp], t->Level);
    } else {
	store->Swizzle = SWIZZLE_NOOP;
	if (dbg) printf("Alloc temp sz %d at %d.xyzw (level %d)\n",
			    store->Size, store->Index, t->Level);
    }
    return true;
}


void
_slang_free_temp(slang_var_table *vt, slang_ir_storage *store)
{
    struct table *t = vt->Top.get();
    GLuint i;
    GLuint r = store->Index;
    assert(store->Size > 0);
    assert(r + store->Size <= vt->MaxRegisters * 4);
    if (dbg) printf("Free temp sz %d at %d (level %d)\n", store->Size, r, t->Level);
    if (store->Size == 1) {
	const GLuint comp = GET_SWZ(store->Swizzle, 0);
	assert(store->Swizzle == MAKE_SWIZZLE4(comp, comp, comp, comp));
	assert(comp < 4);
	assert(t->ValSize[r * 4 + comp] == 1);
	assert(t->Temps[r * 4 + comp] == TEMP);
	t->Temps[r * 4 + comp] = FREE;
    } else {
	/*assert(store->Swizzle == SWIZZLE_NOOP);*/
	assert(t->ValSize[r*4] == store->Size);
	for (i = 0; i < store->Size; i++) {
	    assert(t->Temps[r * 4 + i] == TEMP);
	    t->Temps[r * 4 + i] = FREE;
	}
    }
}


bool
_slang_is_temp(const slang_var_table *vt, const slang_ir_storage *store)
{
    struct table *t = vt->Top.get();
    GLuint comp;
    assert(store->Index >= 0);
    assert(store->Index < vt->MaxRegisters);
    if (store->Swizzle == SWIZZLE_NOOP)
	comp = 0;
    else
	comp = GET_SWZ(store->Swizzle, 0);

    if (t->Temps[store->Index * 4 + comp] == TEMP)
	return true;
    else
	return false;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
