cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# pdmpmt: C library implementation
add_library(pdmpmt block.c mcpi.c)
add_library(pdmpmt::pdmpmt ALIAS pdmpmt)
set_target_properties(pdmpmt PROPERTIES DEFINE_SYMBOL PDMPMT_BUILD_DLL)
target_include_directories(
    pdmpmt PUBLIC
    $<BUILD_INTERFACE:${PDMPMT_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pdmpmt PRIVATE prand)
if(OpenMP_FOUND)
    target_link_libraries(pdmpmt PRIVATE OpenMP::OpenMP_C)
endif()
