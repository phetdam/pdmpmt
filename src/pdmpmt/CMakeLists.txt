cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# pdmpmt: C library implementation
add_library(pdmpmt block.c mcpi.c)
set_target_properties(pdmpmt PROPERTIES DEFINE_SYMBOL PDMPMT_BUILD_DLL)
target_link_libraries(pdmpmt PRIVATE prand OpenMP::OpenMP_C)
