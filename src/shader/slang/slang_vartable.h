
#ifndef SLANG_VARTABLE_H
#define SLANG_VARTABLE_H



struct slang_ir_storage;

struct slang_var_table;

struct slang_variable;

extern slang_var_table *
_slang_new_var_table(GLuint maxRegisters);

extern void
_slang_delete_var_table(slang_var_table *vt);

extern void
_slang_push_var_table(slang_var_table *parent);

extern void
_slang_pop_var_table(slang_var_table *t);

extern void
_slang_add_variable(slang_var_table *t, slang_variable *v);

extern slang_variable *
_slang_find_variable(const slang_var_table *t, slang_atom name);

extern bool
_slang_alloc_var(slang_var_table *t, slang_ir_storage *store);

extern bool
_slang_alloc_temp(slang_var_table *t, slang_ir_storage *store);

extern void
_slang_free_temp(slang_var_table *t, slang_ir_storage *store);

extern bool
_slang_is_temp(const slang_var_table *t, const slang_ir_storage *store);




#endif /* SLANG_VARTABLE_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
