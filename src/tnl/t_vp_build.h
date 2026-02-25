/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Tungsten Graphics   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef _T_ARB_BUILD_H
#define _T_ARB_BUILD_H



#include "mtypes.h"

#include <cstring>
#include <unordered_map>

/*
 * The state_key struct and associated hasher/equality types are placed here
 * so that TNLcontext can own a std::unique_ptr<tnl_vp_cache> without needing
 * to know the full implementation detail.
 */

/** Fixed-function state key used to look up/cache compiled vertex programs. */
struct state_key {
    unsigned light_global_enabled:1;
    unsigned light_local_viewer:1;
    unsigned light_twoside:1;
    unsigned light_color_material:1;
    unsigned light_color_material_mask:12;
    unsigned light_material_mask:12;

    unsigned normalize:1;
    unsigned rescale_normals:1;
    unsigned fog_source_is_depth:1;
    unsigned tnl_do_vertex_fog:1;
    unsigned separate_specular:1;
    unsigned fog_mode:2;
    unsigned point_attenuated:1;
    unsigned texture_enabled_global:1;
    unsigned fragprog_inputs_read:12;

    struct {
	unsigned light_enabled:1;
	unsigned light_eyepos3_is_zero:1;
	unsigned light_spotcutoff_is_180:1;
	unsigned light_attenuated:1;
	unsigned texunit_really_enabled:1;
	unsigned texmat_enabled:1;
	unsigned texgen_enabled:4;
	unsigned texgen_mode0:4;
	unsigned texgen_mode1:4;
	unsigned texgen_mode2:4;
	unsigned texgen_mode3:4;
    } unit[8];
};

/** Hasher for state_key: XOR all 32-bit words. */
struct StateKeyHash {
    std::size_t operator()(const state_key &k) const noexcept {
	const GLuint *ikey = reinterpret_cast<const GLuint *>(&k);
	std::size_t hash = 0;
	for (std::size_t i = 0; i < sizeof(k) / sizeof(GLuint); ++i)
	    hash ^= ikey[i];
	return hash;
    }
};

/** Equality for state_key: byte-level comparison. */
struct StateKeyEqual {
    bool operator()(const state_key &a, const state_key &b) const noexcept {
	return std::memcmp(&a, &b, sizeof(state_key)) == 0;
    }
};

/** Cache mapping a state_key to the compiled gl_vertex_program. */
struct tnl_vp_cache {
    std::unordered_map<state_key, struct gl_vertex_program *,
		       StateKeyHash, StateKeyEqual> map;
};

extern void _tnl_UpdateFixedFunctionProgram(GLcontext *ctx);

extern void _tnl_ProgramCacheInit(GLcontext *ctx);
extern void _tnl_ProgramCacheDestroy(GLcontext *ctx);



#endif

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
