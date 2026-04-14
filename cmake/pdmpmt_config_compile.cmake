cmake_minimum_required(VERSION 3.15)

##
# pdmpmt_config_compile.cmake
#
# Sets up project-wide compilation options.
#

include_guard(GLOBAL)

##
# Configure project-wide compilation options.
#
# This also defines a CUDA C++ compile group target with CUDA compile options,
# cuda_compile_options, that one can use in pdmpmt_cuda_link_libraries() to
# transitively provide compile options to a CUDA C++ compile group.
#
macro(pdmpmt_config_compile)
    # add MSVC-specific compile options
    if(MSVC)
        # CMake adds /O2 by default for release version
        # TODO: consider removing CUDA options now that we have custom rules
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
            # treat angle-bracket includes as system headers and suppress their
            # warnings unless we instantiate an included template
            $<$<COMPILE_LANGUAGE:C,CXX>:/external:anglebrackets>
            $<$<COMPILE_LANGUAGE:C,CXX>:/external:W0>
            $<$<COMPILE_LANGUAGE:C,CXX>:/external:templates->
            # enable standards-conformant handling of lambdas. this resolves
            # issues with MSVC require explicit capture of identifiers that
            # refer to constexpr or static variables
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:lambda>
            # also forward /Zc:lambda to the CUDA C++ frontend
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:lambda>
            # ensure CUDA compiler makes MSVC use /W3. we don't want to have
            # the warning level too high when using NVCC because those headers
            # tend to trigger MSVC a lot when using /Wall
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/W3>
            # cccl/cuda/std/__cccl/preprocessor.h(23) emits error if we don't
            # run MSVC preprocessor under standard-conformant mode
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:preprocessor>
            # 177-D: function declared but never referenced
            $<$<COMPILE_LANGUAGE:CUDA>:-diag-suppress=177>
            # give nvcc the correct /MD[d] flags since we want to use the
            # shared VC++ runtime libraries instead of the static ones
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/MD$<$<CONFIG:Debug>:d>>
        )
    # options are also accepted by Clang
    else()
        add_compile_options(
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:C,CXX>:$<IF:$<CONFIG:Release>,-O3,-g>>
        )
    endif()
    # cuda_compile_options: CUDA C++ compile group interface target for
    # shared options for most pdmpmt_cuda_compile_group() calls
    pdmpmt_cuda_compile_group(cuda_compile_options)
    pdmpmt_cuda_compile_options(
        cuda_compile_options INTERFACE
        # 177-D: function declared but never referenced
        -diag-suppress 177
    )
    # MSVC compiler options to pass from NVCC
    if(MSVC)
        # note: NVCC often misinterprets tokens starting with "/" as paths so
        # it's safest to use the -Xcompiler=<value> format. otherwise, you have
        # to specify explicit escaped quotes for each -Xcompiler value
        pdmpmt_cuda_compile_options(
            cuda_compile_options INTERFACE
            # cccl/cuda/std/__cccl/preprocessor.h(23) emits error if we don't
            # run MSVC preprocessor under standard-conformant mode
            -Xcompiler=/Zc:preprocessor
            # use standards-compliant lambda handling
            -Xcompiler=/Zc:lambda
            # warning level + disable for external headers
            -Xcompiler=/W4
            -Xcompiler=/external:anglebrackets
            -Xcompiler=/external:W0
            -Xcompiler=/external:templates-
            # ensure nvcc has the correct /MD[d] flags since we want to use the
            # shared VC++ runtime libraries instead of the static ones
            -Xcompiler=/MD$<$<CONFIG:Debug>:d>
        )
    endif()
endmacro()
