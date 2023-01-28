cmake_minimum_required(VERSION 3.21)

# build the C library component
add_library(pdmpmt_c mcpi.c)
target_link_libraries(pdmpmt_c PUBLIC GSL::gsl OpenMP::OpenMP_C)
