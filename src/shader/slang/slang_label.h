#ifndef SLANG_LABEL_H
#define SLANG_LABEL_H 1



#include "imports.h"
#include "mtypes.h"
#include "prog_instruction.h"

#include <string>
#include <vector>


/**
 * A branch-target label in a compiled GLSL shader.
 *
 * C++17 modernisation: Name is now a std::string, References is a
 * std::vector<GLuint>, and the struct is allocated/deleted with new/delete
 * instead of the slang memory pool.
 */
struct slang_label {
    std::string Name;
    GLint Location{-1};
    /**
     * List of instruction references (numbered starting at zero) which need
     * their BranchTarget field filled in with the location eventually
     * assigned to the label.
     */
    std::vector<GLuint> References;

    explicit slang_label(const std::string &name) : Name(name) {}
};


extern slang_label *
_slang_label_new(const char *name);

extern slang_label *
_slang_label_new_unique(const char *name);

extern void
_slang_label_delete(slang_label *l);

extern void
_slang_label_add_reference(slang_label *l, GLuint inst);

extern GLint
_slang_label_get_location(const slang_label *l);

extern void
_slang_label_set_location(slang_label *l, GLint location,
			  struct gl_program *prog);




#endif /* SLANG_LABEL_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
