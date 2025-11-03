cmake_minimum_required(VERSION 3.15)

##
# pdmpmt_config_compile.cmake
#
# Sets up project-wide compilation options.
#

##
# Configure project-wide compilation options.
#
macro(pdmpmt_config_compile)
    # add MSVC-specific compile options
    # TODO: add /Zc:lambda for conformant lambda processing
    if(MSVC)
        # CMake adds /O2 by default for release version
        add_compile_options(
            $<$<COMPILE_LANGUAGE:C,CXX>:/Wall>
            # C4061: no enum value handled in switch with default case
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4061>
            # pplwin.h: 'this' used in base member initializer list
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4355>
            # ignore removal of unused inline functions
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4514>
            # Google Test: implicitly deleted copy ctor, copy assignment
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4625>
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4626>
            # C4710: function ont inlined
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4710>
            # C4711: function selected for automatic inline expansion
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4711>
            # C4820: 'n' bytes of padding added after data member
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd4820>
            # Google Test: implicitly deleted move ctor, move assignment
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd5026>
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd5027>
            # disable note on Spectre mitigiation insertion if using /QSpectre
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd5045>
            # pplwin.h: class with virtual functions has non-virtual dtor
            $<$<COMPILE_LANGUAGE:C,CXX>:/wd5204>
            # ensure CUDA compiler makes MSVC use /W3. we don't want to have
            # the warning level too high when using NVCC because those headers
            # tend to trigger MSVC a lot when using /Wall
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/W3>
            # 177-D: function declared but never referenced
            $<$<COMPILE_LANGUAGE:CUDA>:-diag-suppress=177>
        )
    # options are also accepted by Clang
    else()
        add_compile_options(
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:C,CXX>:$<IF:$<CONFIG:Release>,-O3,-g>>
        )
    endif()
endmacro()
