include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
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
