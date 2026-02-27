/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "extensions.h"
#include "mtypes.h"

#include <cstring>
#include <string>
#include <string_view>


/*
 * Note: The GL_MESAX_* extensions are placeholders for future ARB extensions.
 */
/** Descriptor for a single OpenGL extension in the default table. */
struct ExtEntry {
    bool         default_on;  /**< true if enabled by default */
    std::string_view name;    /**< GL extension name string */
    GLboolean gl_extensions::*flag;  /**< ptr-to-member for the enable flag,
                                          or nullptr if always included */
};

static const ExtEntry default_extensions[] = {
    { false, "GL_ARB_depth_texture", &gl_extensions::ARB_depth_texture },
    { false, "GL_ARB_draw_buffers", &gl_extensions::ARB_draw_buffers },
    { false, "GL_ARB_fragment_program", &gl_extensions::ARB_fragment_program },
    { false, "GL_ARB_fragment_shader", &gl_extensions::ARB_fragment_shader },
    { false, "GL_ARB_half_float_pixel", &gl_extensions::ARB_half_float_pixel },
    { false, "GL_ARB_imaging", &gl_extensions::ARB_imaging },
    { false, "GL_ARB_multisample", &gl_extensions::ARB_multisample },
    { false, "GL_ARB_multitexture", &gl_extensions::ARB_multitexture },
    { false, "GL_ARB_occlusion_query", &gl_extensions::ARB_occlusion_query },
    { false, "GL_ARB_pixel_buffer_object", &gl_extensions::EXT_pixel_buffer_object },
    { false, "GL_ARB_point_parameters", &gl_extensions::EXT_point_parameters },
    { false, "GL_ARB_point_sprite", &gl_extensions::ARB_point_sprite },
    { false, "GL_ARB_shader_objects", &gl_extensions::ARB_shader_objects },
    { false, "GL_ARB_shading_language_100", &gl_extensions::ARB_shading_language_100 },
    { false, "GL_ARB_shading_language_120", &gl_extensions::ARB_shading_language_120 },
    { false, "GL_ARB_shadow", &gl_extensions::ARB_shadow },
    { false, "GL_ARB_shadow_ambient", &gl_extensions::SGIX_shadow_ambient },
    { false, "GL_ARB_texture_border_clamp", &gl_extensions::ARB_texture_border_clamp },
    { false, "GL_ARB_texture_compression", &gl_extensions::ARB_texture_compression },
    { false, "GL_ARB_texture_cube_map", &gl_extensions::ARB_texture_cube_map },
    { false, "GL_ARB_texture_env_add", &gl_extensions::EXT_texture_env_add },
    { false, "GL_ARB_texture_env_combine", &gl_extensions::ARB_texture_env_combine },
    { false, "GL_ARB_texture_env_crossbar", &gl_extensions::ARB_texture_env_crossbar },
    { false, "GL_ARB_texture_env_dot3", &gl_extensions::ARB_texture_env_dot3 },
    { false, "GL_ARB_texture_float", &gl_extensions::ARB_texture_float },
    { false, "GL_ARB_texture_mirrored_repeat", &gl_extensions::ARB_texture_mirrored_repeat },
    { false, "GL_ARB_texture_non_power_of_two", &gl_extensions::ARB_texture_non_power_of_two },
    { false, "GL_ARB_texture_rectangle", &gl_extensions::NV_texture_rectangle },
    { true, "GL_ARB_transpose_matrix", &gl_extensions::ARB_transpose_matrix },
    { false, "GL_ARB_vertex_buffer_object", &gl_extensions::ARB_vertex_buffer_object },
    { false, "GL_ARB_vertex_program", &gl_extensions::ARB_vertex_program },
    { false, "GL_ARB_vertex_shader", &gl_extensions::ARB_vertex_shader },
    { true, "GL_ARB_window_pos", &gl_extensions::ARB_window_pos },
    { true, "GL_EXT_abgr", &gl_extensions::EXT_abgr },
    { true, "GL_EXT_bgra", &gl_extensions::EXT_bgra },
    { false, "GL_EXT_blend_color", &gl_extensions::EXT_blend_color },
    { false, "GL_EXT_blend_equation_separate", &gl_extensions::EXT_blend_equation_separate },
    { false, "GL_EXT_blend_func_separate", &gl_extensions::EXT_blend_func_separate },
    { false, "GL_EXT_blend_logic_op", &gl_extensions::EXT_blend_logic_op },
    { false, "GL_EXT_blend_minmax", &gl_extensions::EXT_blend_minmax },
    { false, "GL_EXT_blend_subtract", &gl_extensions::EXT_blend_subtract },
    { true, "GL_EXT_clip_volume_hint", &gl_extensions::EXT_clip_volume_hint },
    { false, "GL_EXT_cull_vertex", &gl_extensions::EXT_cull_vertex },
    { true, "GL_EXT_compiled_vertex_array", &gl_extensions::EXT_compiled_vertex_array },
    { false, "GL_EXT_convolution", &gl_extensions::EXT_convolution },
    { true, "GL_EXT_copy_texture", &gl_extensions::EXT_copy_texture },
    { false, "GL_EXT_depth_bounds_test", &gl_extensions::EXT_depth_bounds_test },
    { true, "GL_EXT_draw_range_elements", &gl_extensions::EXT_draw_range_elements },
    { false, "GL_EXT_framebuffer_object", &gl_extensions::EXT_framebuffer_object },
    { false, "GL_EXT_framebuffer_blit", &gl_extensions::EXT_framebuffer_blit },
    { false, "GL_EXT_fog_coord", &gl_extensions::EXT_fog_coord },
    { false, "GL_EXT_gpu_program_parameters", &gl_extensions::EXT_gpu_program_parameters },
    { false, "GL_EXT_histogram", &gl_extensions::EXT_histogram },
    { false, "GL_EXT_multi_draw_arrays", &gl_extensions::EXT_multi_draw_arrays },
    { false, "GL_EXT_packed_depth_stencil", &gl_extensions::EXT_packed_depth_stencil },
    { true, "GL_EXT_packed_pixels", &gl_extensions::EXT_packed_pixels },
    { false, "GL_EXT_paletted_texture", &gl_extensions::EXT_paletted_texture },
    { false, "GL_EXT_pixel_buffer_object", &gl_extensions::EXT_pixel_buffer_object },
    { false, "GL_EXT_point_parameters", &gl_extensions::EXT_point_parameters },
    { true, "GL_EXT_polygon_offset", &gl_extensions::EXT_polygon_offset },
    { true, "GL_EXT_rescale_normal", &gl_extensions::EXT_rescale_normal },
    { false, "GL_EXT_secondary_color", &gl_extensions::EXT_secondary_color },
    { true, "GL_EXT_separate_specular_color", &gl_extensions::EXT_separate_specular_color },
    { false, "GL_EXT_shadow_funcs", &gl_extensions::EXT_shadow_funcs },
    { false, "GL_EXT_shared_texture_palette", &gl_extensions::EXT_shared_texture_palette },
    { false, "GL_EXT_stencil_two_side", &gl_extensions::EXT_stencil_two_side },
    { false, "GL_EXT_stencil_wrap", &gl_extensions::EXT_stencil_wrap },
    { true, "GL_EXT_subtexture", &gl_extensions::EXT_subtexture },
    { true, "GL_EXT_texture", &gl_extensions::EXT_texture },
    { true, "GL_EXT_texture3D", &gl_extensions::EXT_texture3D },
    { false, "GL_EXT_texture_compression_s3tc", &gl_extensions::EXT_texture_compression_s3tc },
    { true, "GL_EXT_texture_edge_clamp", &gl_extensions::SGIS_texture_edge_clamp },
    { false, "GL_EXT_texture_env_add", &gl_extensions::EXT_texture_env_add },
    { false, "GL_EXT_texture_env_combine", &gl_extensions::EXT_texture_env_combine },
    { false, "GL_EXT_texture_env_dot3", &gl_extensions::EXT_texture_env_dot3 },
    { false, "GL_EXT_texture_filter_anisotropic", &gl_extensions::EXT_texture_filter_anisotropic },
    { false, "GL_EXT_texture_lod_bias", &gl_extensions::EXT_texture_lod_bias },
    { false, "GL_EXT_texture_mirror_clamp", &gl_extensions::EXT_texture_mirror_clamp },
    { true, "GL_EXT_texture_object", &gl_extensions::EXT_texture_object },
    { false, "GL_EXT_texture_rectangle", &gl_extensions::NV_texture_rectangle },
    { false, "GL_EXT_texture_sRGB", &gl_extensions::EXT_texture_sRGB },
    { false, "GL_EXT_timer_query", &gl_extensions::EXT_timer_query },
    { true, "GL_EXT_vertex_array", &gl_extensions::EXT_vertex_array },
    { false, "GL_EXT_vertex_array_set", &gl_extensions::EXT_vertex_array_set },
    { false, "GL_3DFX_texture_compression_FXT1", &gl_extensions::TDFX_texture_compression_FXT1 },
    { false, "GL_APPLE_client_storage", &gl_extensions::APPLE_client_storage },
    { true, "GL_APPLE_packed_pixels", &gl_extensions::APPLE_packed_pixels },
    { false, "GL_APPLE_vertex_array_object", &gl_extensions::APPLE_vertex_array_object },
    { false, "GL_ATI_blend_equation_separate", &gl_extensions::EXT_blend_equation_separate },
    { false, "GL_ATI_texture_env_combine3", &gl_extensions::ATI_texture_env_combine3 },
    { false, "GL_ATI_texture_mirror_once", &gl_extensions::ATI_texture_mirror_once },
    { false, "GL_ATI_fragment_shader", &gl_extensions::ATI_fragment_shader },
    { false, "GL_ATI_separate_stencil", &gl_extensions::ATI_separate_stencil },
    { false, "GL_IBM_multimode_draw_arrays", &gl_extensions::IBM_multimode_draw_arrays },
    { true, "GL_IBM_rasterpos_clip", &gl_extensions::IBM_rasterpos_clip },
    { false, "GL_IBM_texture_mirrored_repeat", &gl_extensions::ARB_texture_mirrored_repeat },
    { false, "GL_INGR_blend_func_separate", &gl_extensions::EXT_blend_func_separate },
    { false, "GL_MESA_pack_invert", &gl_extensions::MESA_pack_invert },
    { false, "GL_MESA_packed_depth_stencil", &gl_extensions::MESA_packed_depth_stencil },
    { false, "GL_MESA_program_debug", &gl_extensions::MESA_program_debug },
    { false, "GL_MESA_resize_buffers", &gl_extensions::MESA_resize_buffers },
    { false, "GL_MESA_ycbcr_texture", &gl_extensions::MESA_ycbcr_texture },
    { true, "GL_MESA_window_pos", &gl_extensions::ARB_window_pos },
    { false, "GL_NV_blend_square", &gl_extensions::NV_blend_square },
    { false, "GL_NV_fragment_program", &gl_extensions::NV_fragment_program },
    { true, "GL_NV_light_max_exponent", &gl_extensions::NV_light_max_exponent },
    { false, "GL_NV_point_sprite", &gl_extensions::NV_point_sprite },
    { false, "GL_NV_texture_rectangle", &gl_extensions::NV_texture_rectangle },
    { true, "GL_NV_texgen_reflection", &gl_extensions::NV_texgen_reflection },
    { false, "GL_NV_vertex_program", &gl_extensions::NV_vertex_program },
    { false, "GL_NV_vertex_program1_1", &gl_extensions::NV_vertex_program1_1 },
    { true, "GL_OES_read_format", &gl_extensions::OES_read_format },
    { false, "GL_SGI_color_matrix", &gl_extensions::SGI_color_matrix },
    { false, "GL_SGI_color_table", &gl_extensions::SGI_color_table },
    { false, "GL_SGI_texture_color_table", &gl_extensions::SGI_texture_color_table },
    { false, "GL_SGIS_generate_mipmap", &gl_extensions::SGIS_generate_mipmap },
    { false, "GL_SGIS_texture_border_clamp", &gl_extensions::ARB_texture_border_clamp },
    { true, "GL_SGIS_texture_edge_clamp", &gl_extensions::SGIS_texture_edge_clamp },
    { true, "GL_SGIS_texture_lod", &gl_extensions::SGIS_texture_lod },
    { false, "GL_SGIX_depth_texture", &gl_extensions::SGIX_depth_texture },
    { false, "GL_SGIX_shadow", &gl_extensions::SGIX_shadow },
    { false, "GL_SGIX_shadow_ambient", &gl_extensions::SGIX_shadow_ambient },
    { false, "GL_SUN_multi_draw_arrays", &gl_extensions::EXT_multi_draw_arrays },
    { false, "GL_S3_s3tc", &gl_extensions::S3_s3tc },
};



/**
 * Enable all extensions suitable for a software-only renderer.
 * This is a convenience function used by the XMesa, OSMesa, GGI drivers, etc.
 */
void
_mesa_enable_sw_extensions(GLcontext *ctx)
{
    ctx->Extensions.ARB_depth_texture = GL_TRUE;
    ctx->Extensions.ARB_draw_buffers = GL_TRUE;
#if FEATURE_ARB_fragment_program
    ctx->Extensions.ARB_fragment_program = GL_TRUE;
#endif
#if FEATURE_ARB_fragment_shader
    ctx->Extensions.ARB_fragment_shader = GL_TRUE;
#endif
    ctx->Extensions.ARB_half_float_pixel = GL_TRUE;
    ctx->Extensions.ARB_imaging = GL_TRUE;
    ctx->Extensions.ARB_multitexture = GL_TRUE;
#if FEATURE_ARB_occlusion_query
    ctx->Extensions.ARB_occlusion_query = GL_TRUE;
#endif
    ctx->Extensions.ARB_point_sprite = GL_TRUE;
#if FEATURE_ARB_shader_objects
    ctx->Extensions.ARB_shader_objects = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_100
    ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_120
    ctx->Extensions.ARB_shading_language_120 = GL_FALSE; /* not quite done */
#endif
    ctx->Extensions.ARB_shadow = GL_TRUE;
    ctx->Extensions.ARB_texture_border_clamp = GL_TRUE;
    ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
    ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
    ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
    ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
    ctx->Extensions.ARB_texture_float = GL_TRUE;
    ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
    ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
#if FEATURE_ARB_vertex_program
    ctx->Extensions.ARB_vertex_program = GL_TRUE;
#endif
#if FEATURE_ARB_vertex_shader
    ctx->Extensions.ARB_vertex_shader = GL_TRUE;
#endif
#if FEATURE_ARB_vertex_buffer_object
    ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;
#endif
    ctx->Extensions.APPLE_vertex_array_object = GL_TRUE;
#if FEATURE_ATI_fragment_shader
    ctx->Extensions.ATI_fragment_shader = GL_TRUE;
#endif
    ctx->Extensions.ATI_texture_env_combine3 = GL_TRUE;
    ctx->Extensions.ATI_texture_mirror_once = GL_TRUE;
    ctx->Extensions.ATI_separate_stencil = GL_TRUE;
    ctx->Extensions.EXT_blend_color = GL_TRUE;
    ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
    ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
    ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
    ctx->Extensions.EXT_blend_minmax = GL_TRUE;
    ctx->Extensions.EXT_blend_subtract = GL_TRUE;
    ctx->Extensions.EXT_convolution = GL_TRUE;
    ctx->Extensions.EXT_depth_bounds_test = GL_TRUE;
    ctx->Extensions.EXT_fog_coord = GL_TRUE;
#if FEATURE_EXT_framebuffer_object
    ctx->Extensions.EXT_framebuffer_object = GL_TRUE;
#endif
#if FEATURE_EXT_framebuffer_blit
    ctx->Extensions.EXT_framebuffer_blit = GL_TRUE;
#endif
    ctx->Extensions.EXT_histogram = GL_TRUE;
    ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;
    ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
    ctx->Extensions.EXT_paletted_texture = GL_TRUE;
#if FEATURE_EXT_pixel_buffer_object
    ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
#endif
    ctx->Extensions.EXT_point_parameters = GL_TRUE;
    ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
    ctx->Extensions.EXT_secondary_color = GL_TRUE;
    ctx->Extensions.EXT_shared_texture_palette = GL_TRUE;
    ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
    ctx->Extensions.EXT_stencil_two_side = GL_FALSE; /* obsolete */
    ctx->Extensions.EXT_texture_env_add = GL_TRUE;
    ctx->Extensions.EXT_texture_env_combine = GL_TRUE;
    ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
    ctx->Extensions.EXT_texture_mirror_clamp = GL_TRUE;
    ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;
#if FEATURE_EXT_texture_sRGB
    ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
#endif
    ctx->Extensions.IBM_multimode_draw_arrays = GL_TRUE;
    ctx->Extensions.MESA_pack_invert = GL_TRUE;
#if FEATURE_MESA_program_debug
    ctx->Extensions.MESA_program_debug = GL_TRUE;
#endif
    ctx->Extensions.MESA_resize_buffers = GL_TRUE;
    ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
    ctx->Extensions.NV_blend_square = GL_TRUE;
    /*ctx->Extensions.NV_light_max_exponent = GL_TRUE;*/
    ctx->Extensions.NV_point_sprite = GL_TRUE;
    ctx->Extensions.NV_texture_rectangle = GL_TRUE;
    /*ctx->Extensions.NV_texgen_reflection = GL_TRUE;*/
#if FEATURE_NV_vertex_program
    ctx->Extensions.NV_vertex_program = GL_TRUE;
    ctx->Extensions.NV_vertex_program1_1 = GL_TRUE;
#endif
#if FEATURE_NV_fragment_program
    ctx->Extensions.NV_fragment_program = GL_TRUE;
#endif
    ctx->Extensions.SGI_color_matrix = GL_TRUE;
    ctx->Extensions.SGI_color_table = GL_TRUE;
    ctx->Extensions.SGI_texture_color_table = GL_TRUE;
    ctx->Extensions.SGIS_generate_mipmap = GL_TRUE;
    ctx->Extensions.SGIS_texture_edge_clamp = GL_TRUE;
    ctx->Extensions.SGIX_depth_texture = GL_TRUE;
    ctx->Extensions.SGIX_shadow = GL_TRUE;
    ctx->Extensions.SGIX_shadow_ambient = GL_TRUE;
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
    ctx->Extensions.EXT_gpu_program_parameters = GL_TRUE;
#endif
}


/**
 * Enable GL_ARB_imaging and all the EXT extensions that are subsets of it.
 */
void
_mesa_enable_imaging_extensions(GLcontext *ctx)
{
    ctx->Extensions.ARB_imaging = GL_TRUE;
    ctx->Extensions.EXT_blend_color = GL_TRUE;
    ctx->Extensions.EXT_blend_minmax = GL_TRUE;
    ctx->Extensions.EXT_blend_subtract = GL_TRUE;
    ctx->Extensions.EXT_convolution = GL_TRUE;
    ctx->Extensions.EXT_histogram = GL_TRUE;
    ctx->Extensions.SGI_color_matrix = GL_TRUE;
    ctx->Extensions.SGI_color_table = GL_TRUE;
}



/**
 * Enable all OpenGL 1.3 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_3_extensions(GLcontext *ctx)
{
    ctx->Extensions.ARB_multisample = GL_TRUE;
    ctx->Extensions.ARB_multitexture = GL_TRUE;
    ctx->Extensions.ARB_texture_border_clamp = GL_TRUE;
    ctx->Extensions.ARB_texture_compression = GL_TRUE;
    ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
    ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
    ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
    ctx->Extensions.EXT_texture_env_add = GL_TRUE;
    /*ctx->Extensions.ARB_transpose_matrix = GL_TRUE;*/
}



/**
 * Enable all OpenGL 1.4 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_4_extensions(GLcontext *ctx)
{
    ctx->Extensions.ARB_depth_texture = GL_TRUE;
    ctx->Extensions.ARB_shadow = GL_TRUE;
    ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
    ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
    ctx->Extensions.ARB_window_pos = GL_TRUE;
    ctx->Extensions.EXT_blend_color = GL_TRUE;
    ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
    ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
    ctx->Extensions.EXT_blend_minmax = GL_TRUE;
    ctx->Extensions.EXT_blend_subtract = GL_TRUE;
    ctx->Extensions.EXT_fog_coord = GL_TRUE;
    ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;
    ctx->Extensions.EXT_point_parameters = GL_TRUE;
    ctx->Extensions.EXT_secondary_color = GL_TRUE;
    ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
    ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;
    ctx->Extensions.SGIS_generate_mipmap = GL_TRUE;
}


/**
 * Enable all OpenGL 1.5 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_5_extensions(GLcontext *ctx)
{
    ctx->Extensions.ARB_occlusion_query = GL_TRUE;
    ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;
    ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
}


/**
 * Enable all OpenGL 2.0 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_0_extensions(GLcontext *ctx)
{
    ctx->Extensions.ARB_draw_buffers = GL_TRUE;
#if FEATURE_ARB_fragment_shader
    ctx->Extensions.ARB_fragment_shader = GL_TRUE;
#endif
    ctx->Extensions.ARB_point_sprite = GL_TRUE;
    ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
#if FEATURE_ARB_shader_objects
    ctx->Extensions.ARB_shader_objects = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_100
    ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
#endif
    ctx->Extensions.EXT_stencil_two_side = GL_FALSE; /* obsolete */
#if FEATURE_ARB_vertex_shader
    ctx->Extensions.ARB_vertex_shader = GL_TRUE;
#endif
}


/**
 * Enable all OpenGL 2.1 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_1_extensions(GLcontext *ctx)
{
#if FEATURE_EXT_pixel_buffer_object
    ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
#endif
#if FEATURE_EXT_texture_sRGB
    ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
#endif
#ifdef FEATURE_ARB_shading_language_120
    ctx->Extensions.ARB_shading_language_120 = GL_FALSE; /* not quite done */
#endif
}



/**
 * Either enable or disable the named extension.
 */
static void
set_extension(GLcontext *ctx, const char *name, GLboolean state)
{
    if (!ctx->Extensions.String.empty()) {
	/* The string was already queried - can't change it now! */
	_mesa_problem(ctx, "Trying to enable/disable extension after glGetString(GL_EXTENSIONS): %s", name);
	return;
    }

    for (const auto &e : default_extensions) {
	if (e.name == name) {
	    if (e.flag)
		ctx->Extensions.*(e.flag) = state;
	    return;
	}
    }
    _mesa_problem(ctx, "Trying to enable unknown extension: %s", name);
}


/**
 * Enable the named extension.
 * Typically called by drivers.
 */
void
_mesa_enable_extension(GLcontext *ctx, const char *name)
{
    set_extension(ctx, name, GL_TRUE);
}


/**
 * Disable the named extension.
 * XXX is this really needed???
 */
void
_mesa_disable_extension(GLcontext *ctx, const char *name)
{
    set_extension(ctx, name, GL_FALSE);
}


/**
 * Test if the named extension is enabled in this context.
 */
GLboolean
_mesa_extension_is_enabled(GLcontext *ctx, const char *name)
{
    for (const auto &e : default_extensions) {
	if (e.name == name) {
	    if (!e.flag)
		return GL_TRUE;
	    return ctx->Extensions.*(e.flag);
	}
    }
    return GL_FALSE;
}


/**
 * Run through the default_extensions array above and set the
 * ctx->Extensions.ARB/EXT_* flags accordingly.
 * To be called during context initialization.
 */
void
_mesa_init_extensions(GLcontext *ctx)
{
    for (const auto &e : default_extensions) {
	if (e.default_on && e.flag)
	    ctx->Extensions.*(e.flag) = GL_TRUE;
    }
}


/**
 * Construct the GL_EXTENSIONS string.  Called the first time that
 * glGetString(GL_EXTENSIONS) is called.
 */
std::string
_mesa_make_extension_string(GLcontext *ctx)
{
    std::string ext;

    for (const auto &e : default_extensions) {
	if (!e.flag || ctx->Extensions.*(e.flag)) {
	    if (!ext.empty())
		ext += ' ';
	    ext += e.name;
	}
    }

    assert(!ext.empty());

    return ext;
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
