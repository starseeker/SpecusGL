

/**
 * Functions for managing instruction labels.
 * Basically, this is used to manage the problem of forward branches where
 * we have a branch instruciton but don't know the target address yet.
 */


#include "slang_label.h"
#include "slang_mem.h"
#include <string>



slang_label *
_slang_label_new(const char *name)
{
    slang_label *l = (slang_label *) _slang_alloc(sizeof(slang_label));
    if (l) {
	l->Name = _slang_strdup(name);
	l->Location = -1;
    }
    return l;
}

/**
 * As above, but suffix the name with a unique number.
 */
slang_label *
_slang_label_new_unique(const char *name)
{
    static int id = 1;
    slang_label *l = static_cast<slang_label *>(_slang_alloc(sizeof(slang_label)));
    if (l) {
	const std::string unique = std::string(name) + "_" + std::to_string(id++);
	l->Name = _slang_strdup(unique.c_str());
	if (!l->Name) {
	    _slang_free(l);
	    return nullptr;
	}
	l->Location = -1;
    }
    return l;
}

void
_slang_label_delete(slang_label *l)
{
    if (l->Name) {
	_slang_free(l->Name);
	l->Name = nullptr;
    }
    if (l->References) {
	_slang_free(l->References);
	l->References = nullptr;
    }
    _slang_free(l);
}


void
_slang_label_add_reference(slang_label *l, GLuint inst)
{
    const GLuint oldSize = l->NumReferences * sizeof(GLuint);
    assert(l->Location < 0);
    l->References = static_cast<GLuint*>(_slang_realloc(l->References,
				   oldSize, oldSize + sizeof(GLuint)));
    if (l->References) {
	l->References[l->NumReferences] = inst;
	l->NumReferences++;
    }
}


GLint
_slang_label_get_location(const slang_label *l)
{
    return l->Location;
}


void
_slang_label_set_location(slang_label *l, GLint location,
			  struct gl_program *prog)
{
    GLuint i;

    assert(l->Location < 0);
    assert(location >= 0);

    l->Location = location;

    /* for the instructions that were waiting to learn the label's location: */
    for (i = 0; i < l->NumReferences; i++) {
	const GLuint j = l->References[i];
	prog->Instructions[j].BranchTarget = location;
    }

    if (l->References) {
	_slang_free(l->References);
	l->References = nullptr;
    }
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
