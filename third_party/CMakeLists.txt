cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# CMakeLists.txt for third-party vendored dependencies

# add prand static (PIC if BUILD_SHARED_LIBS=ON) checked out at 37c5bba
add_subdirectory(prand-37c5bba)
