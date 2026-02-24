include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/main
  ${CMAKE_CURRENT_SOURCE_DIR}/glapi
  ${CMAKE_CURRENT_SOURCE_DIR}/shader
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common
  ${CMAKE_CURRENT_SOURCE_DIR}/tnl
  ${CMAKE_CURRENT_SOURCE_DIR}/swrast
  ${CMAKE_CURRENT_SOURCE_DIR}/swrast_setup
  ${CMAKE_CURRENT_SOURCE_DIR}/shader/grammar
  ${CMAKE_CURRENT_SOURCE_DIR}/shader/slang
  ${CMAKE_CURRENT_SOURCE_DIR}/fxaa
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

set(osmesa_srcs
  drivers/common/driverfuncs.c
  drivers/osmesa/osmesa.c
  fxaa/fxaa_cpu.c
  glapi/glapi.cpp
  glapi/glthread.cpp
  main/accum.cpp
  main/api_arrayelt.cpp
  main/api_loopback.cpp
  main/api_noop.cpp
  main/api_validate.cpp
  main/arrayobj.cpp
  main/attrib.cpp
  main/blend.cpp
  main/bufferobj.cpp
  main/buffers.cpp
  main/clip.cpp
  main/colortab.cpp
  main/context.cpp
  main/convolve.cpp
  main/debug.cpp
  main/depth.cpp
  main/depthstencil.cpp
  main/dispatch.cpp
  main/dlist.cpp
  main/drawpix.cpp
  main/enable.cpp
  main/enums.cpp
  main/eval.cpp
  main/extensions.cpp
  main/fbobject.cpp
  main/feedback.cpp
  main/fog.cpp
  main/framebuffer.cpp
  main/get.cpp
  main/getstring.cpp
  main/hash.cpp
  main/hint.cpp
  main/histogram.cpp
  main/image.cpp
  main/imports.cpp
  main/light.cpp
  main/lines.cpp
  main/matrix.cpp
  main/mipmap.cpp
  main/pixel.cpp
  main/points.cpp
  main/polygon.cpp
  main/queryobj.cpp
  main/rastpos.cpp
  main/rbadaptors.cpp
  main/renderbuffer.cpp
  main/shaders.cpp
  main/state.cpp
  main/stencil.cpp
  main/texcompress.cpp
  main/texcompress_fxt1.cpp
  main/texcompress_s3tc.cpp
  main/texenvprogram.cpp
  main/texformat.cpp
  main/teximage.cpp
  main/texobj.cpp
  main/texrender.cpp
  main/texstate.cpp
  main/texstore.cpp
  main/varray.cpp
  main/vtxfmt.cpp
  math/m_debug_clip.cpp
  math/m_debug_norm.cpp
  math/m_debug_xform.cpp
  math/m_eval.cpp
  math/m_matrix.cpp
  math/m_translate.cpp
  math/m_vector.cpp
  math/m_xform.cpp
  shader/arbprogparse.c
  shader/arbprogram.c
  shader/atifragshader.c
  shader/grammar/grammar_mesa.c
  shader/nvfragparse.c
  shader/nvprogram.c
  shader/nvvertparse.c
  shader/prog_execute.c
  shader/prog_instruction.cpp
  shader/prog_parameter.cpp
  shader/prog_print.c
  shader/prog_statevars.c
  shader/program.cpp
  shader/programopt.c
  shader/shader_api.c
  shader/slang/slang_builtin.c
  shader/slang/slang_codegen.c
  shader/slang/slang_compile.c
  shader/slang/slang_compile_function.c
  shader/slang/slang_compile_operation.c
  shader/slang/slang_compile_struct.c
  shader/slang/slang_compile_variable.c
  shader/slang/slang_emit.c
  shader/slang/slang_ir.c
  shader/slang/slang_label.c
  shader/slang/slang_library_noise.c
  shader/slang/slang_link.c
  shader/slang/slang_log.c
  shader/slang/slang_mem.c
  shader/slang/slang_preprocess.c
  shader/slang/slang_print.c
  shader/slang/slang_simplify.c
  shader/slang/slang_storage.c
  shader/slang/slang_typeinfo.c
  shader/slang/slang_utility.c
  shader/slang/slang_vartable.c
  swrast/s_aaline.cpp
  swrast/s_aatriangle.cpp
  swrast/s_accum.cpp
  swrast/s_alpha.cpp
  swrast/s_atifragshader.cpp
  swrast/s_bitmap.cpp
  swrast/s_blend.cpp
  swrast/s_blit.cpp
  swrast/s_buffers.cpp
  swrast/s_context.cpp
  swrast/s_copypix.cpp
  swrast/s_depth.cpp
  swrast/s_drawpix.cpp
  swrast/s_feedback.cpp
  swrast/s_fog.cpp
  swrast/s_fragprog.cpp
  swrast/s_imaging.cpp
  swrast/s_lines.cpp
  swrast/s_logic.cpp
  swrast/s_masking.cpp
  swrast/s_points.cpp
  swrast/s_readpix.cpp
  swrast/s_span.cpp
  swrast/s_stencil.cpp
  swrast/s_texcombine.cpp
  swrast/s_texfilter.cpp
  swrast/s_texstore.cpp
  swrast/s_triangle.cpp
  swrast/s_zoom.cpp
  swrast_setup/ss_context.cpp
  swrast_setup/ss_triangle.cpp
  tnl/t_context.cpp
  tnl/t_draw.cpp
  tnl/t_pipeline.cpp
  tnl/t_vb_cull.cpp
  tnl/t_vb_fog.cpp
  tnl/t_vb_light.cpp
  tnl/t_vb_normals.cpp
  tnl/t_vb_points.cpp
  tnl/t_vb_program.cpp
  tnl/t_vb_render.cpp
  tnl/t_vb_texgen.cpp
  tnl/t_vb_texmat.cpp
  tnl/t_vb_vertex.cpp
  tnl/t_vertex.cpp
  tnl/t_vertex_generic.cpp
  tnl/t_vp_build.cpp
  vbo/vbo_context.cpp
  vbo/vbo_exec.cpp
  vbo/vbo_exec_api.cpp
  vbo/vbo_exec_array.cpp
  vbo/vbo_exec_draw.cpp
  vbo/vbo_exec_eval.cpp
  vbo/vbo_rebase.cpp
  vbo/vbo_save.cpp
  vbo/vbo_save_api.cpp
  vbo/vbo_save_draw.cpp
  vbo/vbo_save_loopback.cpp
  vbo/vbo_split.cpp
  vbo/vbo_split_copy.cpp
  vbo/vbo_split_inplace.cpp
  )

add_library(osmesa SHARED ${osmesa_srcs})
target_compile_features(osmesa PRIVATE cxx_std_17)
target_link_libraries(osmesa ${M_LIBRARY})
install(TARGETS osmesa
  RUNTIME DESTINATION ${BIN_DIR}
  LIBRARY DESTINATION ${LIB_DIR}
  ARCHIVE DESTINATION ${LIB_DIR})
set_property(TARGET osmesa APPEND PROPERTY COMPILE_DEFINITIONS "UTIL_ARCH_LITTLE_ENDIAN=1")
set_property(TARGET osmesa APPEND PROPERTY COMPILE_DEFINITIONS "LIBRARYBUILD")
set_property(TARGET osmesa APPEND PROPERTY COMPILE_DEFINITIONS "GL_DLL_EXPORTS")
set_property(TARGET osmesa APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS "GL_DLL_IMPORTS")
if (OSMESA_FAST AND O3_COMPILER_FLAG)
  target_compile_options(osmesa PRIVATE "-O3")
elseif (O2_COMPILER_FLAG)
  target_compile_options(osmesa PRIVATE "-O2")
endif ()

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

