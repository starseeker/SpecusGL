

/**
 * Functions for managing instruction labels.
 * Basically, this is used to manage the problem of forward branches where
 * we have a branch instruciton but don't know the target address yet.
 *
 * C++17 modernisation: slang_label now owns its Name (std::string) and
 * References (std::vector<GLuint>) directly, and is allocated/deleted with
 * new/delete instead of the slang memory pool.
 */


#include "slang_label.h"
#include <string>



slang_label *
_slang_label_new(const char *name)
{
    return new slang_label(name ? name : "");
}

/**
 * As above, but suffix the name with a unique number.
 */
slang_label *
_slang_label_new_unique(const char *name)
{
    static int id = 1;
    const std::string unique = std::string(name ? name : "") + "_" + std::to_string(id++);
    return new slang_label(unique);
}

void
_slang_label_delete(slang_label *l)
{
    delete l;
}


void
_slang_label_add_reference(slang_label *l, GLuint inst)
{
    assert(l->Location < 0);
    l->References.push_back(inst);
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
    assert(l->Location < 0);
    assert(location >= 0);

    l->Location = location;

    /* for the instructions that were waiting to learn the label's location: */
    for (GLuint inst : l->References) {
	prog->Instructions[inst].BranchTarget = location;
    }
    l->References.clear();
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
