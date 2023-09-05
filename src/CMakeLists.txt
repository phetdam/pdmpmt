cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# build the C library component
add_library(pdmpmt mcpi.c)
set_target_properties(pdmpmt PROPERTIES DEFINE_SYMBOL PDMPMT_BUILD_DLL)
target_link_libraries(pdmpmt PUBLIC GSL::gsl PRIVATE OpenMP::OpenMP_C)
