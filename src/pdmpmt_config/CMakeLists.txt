cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# pdmpmt_config: pdmpmt configuration library
add_library(pdmpmt_config SHARED paths.cc)
target_include_directories(
    pdmpmt_config PUBLIC
    $<BUILD_INTERFACE:${PDMPMT_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(pdmpmt_config PROPERTIES DEFINE_SYMBOL PDMPMT_BUILD_CONFIG)
# on Windows need kernel32
if(WIN32)
    target_link_libraries(pdmpmt_config PRIVATE kernel32)
# other platforms, e.g. POSIX, would use libdl
else()
    target_link_libraries(pdmpmt_config PRIVATE dl)
endif()
