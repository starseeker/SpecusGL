include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/main
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/glapi
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/math
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/shader
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/shader/grammar
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/shader/slang
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/swrast
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/swrast_setup
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/tnl
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/drivers/common
  )

add_executable(mesa_tests mesa_tests.cpp)
target_link_libraries(mesa_tests osmesa ${M_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET mesa_tests APPEND PROPERTY COMPILE_DEFINITIONS "GL_DLL_IMPORTS")

enable_testing()
add_test(NAME mesa_tests COMMAND mesa_tests)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
