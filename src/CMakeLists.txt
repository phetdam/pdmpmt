cmake_minimum_required(VERSION 3.21)

# build the C library component
add_library(pdmpmt mcpi.c)
target_link_libraries(pdmpmt PUBLIC GSL::gsl PRIVATE OpenMP::OpenMP_C)
# if building as shared, define relevant macro
if(BUILD_SHARED_LIBS)
    target_compile_definitions(pdmpmt PRIVATE PDMPMT_BUILD_DLL)
endif()
