cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# add MSVC-specific compile options
if(MSVC)
    # CMake adds /O2 by default for release version
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:/Wall>
        # pplwin.h: 'this' used in base member initializer list
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4355>
        # ignore removal of unused inline functions
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4514>
        # Google Test: implicitly deleted copy ctor, copy assignment
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4625>
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4626>
        # C4820: 'n' bytes of padding added after data member
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4820>
        # Google Test: implicitly deleted move ctor, move assignment
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd5026>
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd5027>
        # disable note on Spectre mitigiation insertion if using /QSpectre
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd5045>
        # pplwin.h: class with virtual functions has non-virtual dtor
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd5204>
    )
    # needed for GSL global variables that have to be exported
    add_compile_definitions(GSL_DLL)
# options are also accepted by Clang
else()
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:C,CXX>:$<IF:$<CONFIG:Release>,-O3,-g>>
    )
endif()
