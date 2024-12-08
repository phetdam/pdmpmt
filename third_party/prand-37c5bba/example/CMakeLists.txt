cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# prand_multistream: example program (renamed from multistream)
add_executable(prand_multistream multistream.c)
target_link_libraries(prand_multistream PRIVATE prand)
# allow use as a test
add_test(NAME prand_multistream COMMAND prand_multistream)
