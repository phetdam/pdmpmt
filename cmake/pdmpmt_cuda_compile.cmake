cmake_minimum_required(VERSION 3.21)

##
# pdmpmt_cuda_compile.cmake
#
# Ensure library or executable targets with CUDA files track dependencies.
#
# As of CMake 4.3.1 no CUDA dependency tracking is provided. For example, if
# you had a .cu file that included a header from your project, CMake would fail
# to rebuild the .cu file if you made a change to the header. This means that
# you end up needing to touch(1) the .cu file to trigger a rebuild manually
# which is annoying and defeats the purpose of a build system.
#
# Fortunately, NVCC provides the -M, -MD options for build system integration,
# which allows us to create our own .d dependency file for any compiled .cu
# file. Originally, the approach of intercepting the CUDA compilation stage was
# attempted by using custom rules with add_custom_command(), but of course, the
# produced depfile was incorrect. Instead, the approach of defining targets
# with custom properties, in this cause CUDA_* properties, and treating them as
# "compile groups" of .cu files to compile and link, with different compile
# options, include directories, and library targets/interfaces, we can follow
# the "modern CMake" method of target-based build configuration.
#
# For example, we can create an executable that is a mix of C++ and CUDA C++
# files as a fat binary using something like the following:
#
#   # executable specifying C++ sources
#   add_executable(myprog source1.cc source2.cc)
#   # CUDA C++ compile group with CUDA sources + compile options
#   pdmpmt_cuda_compile_group(myprog_cu source1.cu source2.cu)
#   pdmpmt_cuda_compile_options(myprog_cu PRIVATE -diag-suppress 177)
#   # consume other C++ library targets or other CUDA C++ compile groups for
#   # their include directories, compile options, and link libraries
#   pdmpmt_cuda_link_libraries(myprog_cu PRIVATE CUDA::cudart CURL::libcurl)
#   # add group to the host C++ executable to generate the custom build rules
#   pdmpmt_cuda_add_compile_groups(myprog myprog_cu)
#

include_guard(GLOBAL)

##
# Create a CUDA C++ compilation group.
#
# This defines a single custom target that represents several CUDA objects with
# shared compile and link options. Furthermore, by specifying a target instead
# of directly providing a generated file from a custom build rule to a library
# or executable target, we can accumulate compile + link options from other
# standard C++ targets, or even other CUDA compilation groups.
#
# The list of source files is stored in the CUDA_SOURCES target property. Note
# that an INTERFACE style target can be defined if no sources are specified.
#
# Arguments:
#   target          Target for the CUDA compile group
#   sources...      CUDA .cu source files
#
function(pdmpmt_cuda_compile_group target)
    # empty custom target to hang generated objects on
    add_custom_target(${target})
    # set sources
    set_target_properties(${target} PROPERTIES CUDA_SOURCES "${ARGN}")
    # set a property to indicate that this is a CUDA C++ compile group
    set_property(TARGET ${target} PROPERTY CUDA_CXX_COMPILE_GROUP TRUE)
endfunction()

##
# Indicate if a target is a CUDA C++ compilation group.
#
# This simply tests for the CUDA_CXX_COMPILE_GROUP property on the target.
#
# Arguments:
#   target          Target to check
#   res             Name of variable to set to TRUE or FALSE with the result
#
function(pdmpmt_cuda_is_compile_group target res)
    # if not a target immediately false
    if(NOT TARGET ${target})
        set(${res} FALSE PARENT_SCOPE)
        return()
    endif()
    # otherwise, get the CUDA_CXX_COMPILE_GROUP property, and check
    get_property(result TARGET ${target} PROPERTY CUDA_CXX_COMPILE_GROUP)
    if(result)
        set(${res} TRUE PARENT_SCOPE)
    else()
        set(${res} FALSE PARENT_SCOPE)
    endif()
endfunction()

##
# Add include directories to the CUDA C++ compilation group.
#
# Directories can have PRIVATE, INTERFACE, or PUBLIC visibility.
#
# The include directories will be stored in the CUDA_INCLUDE_DIRE, for PRIVATE
# and PUBLIC, and CUDA_INTERFACE_INCLUDE_DIRS, for INTERFACE and PUBLIC.
#
# Arguments:
#   target                  CUDA C++ compile gruop
#   [PRIVATE dirs...]       Private compilation include directories
#   [INTERFACE dirs...]     Interface include directories
#   [PUBLIC dirs...]        Public compilation + interface include directories
#
function(pdmpmt_cuda_include_directories target)
    cmake_parse_arguments(ARG "" "" "PRIVATE;INTERFACE;PUBLIC" ${ARGN})
    # get CUDA_INCLUDE_DIRS
    get_property(cuda_dirs TARGET ${target} PROPERTY CUDA_INCLUDE_DIRS)
    # append compile include directories, de-duplicate, and set
    list(APPEND cuda_dirs ${ARG_PRIVATE} ${ARG_PUBLIC})
    list(REMOVE_DUPLICATES cuda_dirs)
    set_property(TARGET ${target} PROPERTY CUDA_INCLUDE_DIRS "${cuda_dirs}")
    # get CUDA_INTERFACE_INCLUDE_DIRS
    get_property(
        cuda_int_dirs
        TARGET ${target}
        PROPERTY CUDA_INTERFACE_INCLUDE_DIRS
    )
    # append interface include directories, de-duplicate, and set
    list(APPEND cuda_int_dirs ${ARG_INTERFACE} ${ARG_PUBLIC})
    list(REMOVE_DUPLICATES cuda_int_dirs)
    set_property(
        TARGET ${target} PROPERTY
        CUDA_INTERFACE_INCLUDE_DIRS "${cuda_int_dirs}"
    )
endfunction()

##
# Add compilation options to the CUDA C++ compilation group.
#
# Option can have PRIVATE, INTERFACE, or PUBLIC visibility.
#
# The compile options will be stored in the CUDA_COMPILE_OPTS, for PRIVATE and
# PUB;IC, and CUDA_INTERFACE_COMPILE_OPTS, for INTERFACE and PUBLIC.
#
# Arguments:
#   target                  CUDA C++ compile gruop
#   [PRIVATE dirs...]       Private CUDA compilation options
#   [INTERFACE dirs...]     Interface CUDA compilation options
#   [PUBLIC dirs...]        Public compilation + interface CUDA options
#
function(pdmpmt_cuda_compile_options target)
    cmake_parse_arguments(ARG "" "" "PRIVATE;INTERFACE;PUBLIC" ${ARGN})
    # get CUDA_COMPILE_OPTS
    get_property(cuda_opts TARGET ${target} PROPERTY CUDA_COMPILE_OPTS)
    # append compile options, de-duplicate, and set
    list(APPEND cuda_opts ${ARG_PRIVATE} ${ARG_PUBLIC})
    list(REMOVE_DUPLICATES cuda_opts)
    set_property(TARGET ${target} PROPERTY CUDA_COMPILE_OPTS "${cuda_opts}")
    # get CUDA_INTERFACE_COMPILE_OPTS
    get_property(
        cuda_int_opts
        TARGET ${target}
        PROPERTY CUDA_INTERFACE_COMPILE_OPTS
    )
    # append interface compile options, de-duplicate, and set
    list(APPEND cuda_int_opts ${ARG_INTERFACE} ${ARG_PUBLIC})
    list(REMOVE_DUPLICATES cuda_int_opts)
    set_property(
        TARGET ${target} PROPERTY
        CUDA_INTERFACE_COMPILE_OPTS "${cuda_int_opts}"
    )
endfunction()


##
# Add libraries for linking to the CUDA C++ compilation group.
#
# Any include directories in the INTERFACE_INCLUDE_DIRECTORIES or
# CUDA_INTERFACE_INCLUDE_DIRS properties for each target will be added to the
# CUDA_INCLUDE_DIRS and CUDA_INTERFACE_INCLUDE_DIRS properties appropriately.
#
# Any compilation options in the INTERFACE_COMPILE_OPTIONS or the
# CUDA_INTERFACE_COMPILE_OPTS properties for each target will be added to the
# CUDA_COMPILE_OPTS and CUDA_INTERFACE_COMPILE_OPTS properties appropriately.
#
# Any libraries in INTERFACE_LINK_LIBRARIES or CUDA_INTERFACE_LINK_LIBS will
# be added to the CUDA_LINK_LIBS and CUDA_INTERFACE_LINK_LIBS properties.
#
# Arguments:
#   target                  CUDA C++ compile group
#   [PRIVATE libs...]       Private library targets/paths/names/CUDA groups
#   [INTERFACE libs...]     Interface library targets/paths/names/CUDA groups
#   [PUBLIC libs...]        Public library targets/paths/names/CUDA groups
#
function(pdmpmt_cuda_link_libraries target)
    cmake_parse_arguments(ARG "" "" "PRIVATE;INTERFACE;PUBLIC" ${ARGN})
    # get CUDA_LINK_LIBS
    get_property(cuda_libs TARGET ${target} PROPERTY CUDA_LINK_LIBS)
    # handle PRIVATE and PUBLIC libraries
    foreach(lib ${ARG_PRIVATE} ${ARG_PUBLIC})
        # if a target
        if(TARGET ${lib})
            # obtain all include dirs, compile options, libraries
            # note: libraries are directly used as targets to preserve any
            # additional usage requirements needed by C++ objects
            get_property(
                lib_include_dirs TARGET ${lib}
                PROPERTY INTERFACE_INCLUDE_DIRECTORIES
            )
            get_property(
                lib_cuda_dirs TARGET ${lib}
                PROPERTY CUDA_INTERFACE_INCLUDE_DIRS
            )
            get_property(
                lib_compile_opts TARGET ${lib}
                PROPERTY INTERFACE_COMPILE_OPTIONS
            )
            get_property(
                lib_cuda_compile_opts TARGET ${lib}
                PROPERTY CUDA_INTERFACE_COMPILE_OPTS
            )
            get_property(
                lib_cuda_libs TARGET ${lib}
                PROPERTY CUDA_INTERFACE_LINK_LIBS
            )
            # append appropriately
            if(lib_include_dirs)
                pdmpmt_cuda_include_directories(
                    ${target} PRIVATE
                    ${lib_include_dirs}
                )
            endif()
            if(lib_cuda_dirs)
                pdmpmt_cuda_include_directories(
                    ${target} PRIVATE
                    ${lib_cuda_dirs}
                )
            endif()
            if(lib_compile_opts)
                pdmpmt_cuda_compile_options(
                    ${target} PRIVATE
                    ${lib_compile_opts}
                )
            endif()
            if(lib_cuda_compile_opts)
                pdmpmt_cuda_compile_options(
                    ${target} PRIVATE
                    ${lib_cuda_compile_opts}
                )
            endif()
            # handle CUDA compile group libraries
            if(lib_cuda_libs)
                list(APPEND cuda_libs ${lib_cuda_libs})
            # otherwise copy over the target directly
            else()
                list(APPEND cuda_libs ${lib})
            endif()
            # ensure unset for next iteration
            unset(lib_include_dirs)
            unset(lib_cuda_dirs)
            unset(lib_compile_opts)
            unset(lib_cuda_compile_opts)
            unset(lib_cuda_libs)
        # otherwise just a library name so pass directly
        else()
            list(APPEND cuda_libs ${lib})
        endif()
    endforeach()
    # de-duplicate cuda_libs and update
    list(REMOVE_DUPLICATES cuda_libs)
    set_property(TARGET ${target} PROPERTY CUDA_LINK_LIBS "${cuda_libs}")
    # get CUDA_INTERFACE_LINK_LIBS
    get_property(
        cuda_int_libs TARGET ${target}
        PROPERTY CUDA_INTERFACE_LINK_LIBS
    )
    # handle INTERFACE and PUBLIC libraries
    foreach(lib ${ARG_INTERFACE} ${ARG_PUBLIC})
        # if a target
        if(TARGET ${lib})
            # obtain all include dirs + libraries
            # note: libraries are directly used as targets to preserve any
            # additional usage requirements needed by C++ objects
            get_property(
                lib_include_dirs TARGET ${lib}
                PROPERTY INTERFACE_INCLUDE_DIRECTORIES
            )
            get_property(
                lib_cuda_dirs TARGET ${lib}
                PROPERTY CUDA_INTERFACE_INCLUDE_DIRS
            )
            get_property(
                lib_compile_opts TARGET ${lib}
                PROPERTY INTERFACE_COMPILE_OPTIONS
            )
            get_property(
                lib_cuda_compile_opts TARGET ${lib}
                PROPERTY CUDA_INTERFACE_COMPILE_OPTS
            )
            get_property(
                lib_cuda_libs TARGET ${lib}
                PROPERTY CUDA_INTERFACE_LINK_LIBS
            )
            # append appropriately
            if(lib_include_dirs)
                pdmpmt_cuda_include_directories(
                    ${target} INTERFACE
                    ${lib_include_dirs}
                )
            endif()
            if(lib_cuda_dirs)
                pdmpmt_cuda_include_directories(
                    ${target} INTERFACE
                    ${lib_cuda_dirs}
                )
            endif()
            if(lib_compile_opts)
                pdmpmt_cuda_compile_options(
                    ${target} INTERFACE
                    ${lib_compile_opts}
                )
            endif()
            if(lib_cuda_compile_opts)
                pdmpmt_cuda_compile_options(
                    ${target} INTERFACE
                    ${lib_cuda_compile_opts}
                )
            endif()
            # handle CUDA compile group libraries
            if(lib_cuda_libs)
                list(APPEND cuda_int_libs ${lib_cuda_libs})
            # otherwise copy over the target directly
            else()
                list(APPEND cuda_int_libs ${lib})
            endif()
            # ensure unset for next iteration
            unset(lib_include_dirs)
            unset(lib_cuda_dirs)
            unset(lib_compile_opts)
            unset(lib_cuda_compile_opts)
            unset(lib_cuda_libs)
        # otherwise just a library name so pass directly
        else()
            list(APPEND cuda_int_libs ${lib})
        endif()
    endforeach()
    # de-duplicate cuda_int_libs and update
    list(REMOVE_DUPLICATES cuda_int_libs)
    set_property(
        TARGET ${target} PROPERTY
        CUDA_INTERFACE_LINK_LIBS "${cuda_int_libs}"
    )
endfunction()

##
# Add CUDA C++ compile groups as part of a target's sources.
#
# This function creates the actual add_custom_command() rules that ensure
# correct dependency tracking and generation of CUDA objects. The groups'
# CUDA_INCLUDE_DIRS will be used to add -I options to the CUDA driver command
# and the CUDA_COMPILE_OPTS will be used to add CUDA compiler options directly.
# The CUDA_LINK_LIBS libraries have some special treatment: they will be used
# with target_link_libraries() to ensure that the correct objects and libraries
# are linked as long as the given target is *not* a CUDA C++ compile group.
# This exception ensures that the CUDA options are passed to the host linker.
#
# Each added CUDA C++ compile group can be conceptualized as adding sources in
# a target-private manner, e.g. only one target would build one particular CUDA
# object from a .cu file. This could be relaxed in the future by adding a
# custom target to drive the .cu.o[bj] compilation to prevent races.
#
# Arguments:
#   target          Library or executable target to build
#   groups...       CUDA C++ compile groups to add
#
function(pdmpmt_cuda_add_groups target)
    # object extension
    if(MSVC)
        set(obj_ext "obj")
    else()
        set(obj_ext "o")
    endif()
    # CMake C++ flags generator expressions per configuration
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${config}" config_upper)
        list(
            APPEND cuda_flags_genex
            "$<$<CONFIG:${config}>:${CMAKE_CXX_FLAGS_${config_upper}}>"
        )
    endforeach()
    # CMake CUDA arch spec
    string(
        CONCAT cuda_arch_spec
        "--generate-code=arch=compute_${CMAKE_CUDA_ARCHITECTURES},"
        "code=[compute_${CMAKE_CUDA_ARCHITECTURES},sm_${CMAKE_CUDA_ARCHITECTURES}]"
    )
    # ensure linker language is C++
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
    # for each CUDA compile group
    foreach(cuda_group ${ARGN})
        # get CUDA_INCLUDE_DIRS, CUDA_COMPILE_OPTS, and CUDA_SOURCES
        get_property(cuda_dirs TARGET ${cuda_group} PROPERTY CUDA_INCLUDE_DIRS)
        get_property(cuda_opts TARGET ${cuda_group} PROPERTY CUDA_COMPILE_OPTS)
        get_property(cuda_sources TARGET ${cuda_group} PROPERTY CUDA_SOURCES)
        # prepend include directories with -I option and quote
        list(TRANSFORM cuda_dirs PREPEND "-I\"" OUTPUT_VARIABLE cuda_includes)
        list(TRANSFORM cuda_includes APPEND "\"")
        # remove any INSTALL_INTERFACE entries as those will expand to -I""
        list(FILTER cuda_includes EXCLUDE REGEX "-I\"\\$<INSTALL_INTERFACE:")
        # for each of the sources add a custom command + object for generation
        foreach(cuda_source ${cuda_sources})
            # get absolute path to source
            cmake_path(
                ABSOLUTE_PATH cuda_source
                OUTPUT_VARIABLE cuda_source_path
            )
            # add command
            add_custom_command(
                OUTPUT ${cuda_source}.${obj_ext}
                COMMAND ${CMAKE_CUDA_COMPILER}
                        $<${MSVC}:--use-local-env>
                        -c
                        ${cuda_includes}
                        -MD -MF ${cuda_source}.d
                        ${CMAKE_CUDA_FLAGS}
                        -std=c++${CMAKE_CXX_STANDARD}
                        "${cuda_arch_spec}"
                        # emit device debugging info for Debug config
                        $<$<CONFIG:Debug>:-G>
                        # options for host compiler
                        -Xcompiler "\"${CMAKE_CXX_FLAGS}\""
                        -Xcompiler ${cuda_flags_genex}
                        # ensure for MSVC that we write to separate PDB files
                        "$<${MSVC}:-Xcompiler \"/Fd${cuda_source}.pdb\">"
                        # additional CUDA compile options
                        ${cuda_opts}
                        -o ${cuda_source}.${obj_ext}
                        "${cuda_source_path}"
                COMMENT "CUDA C++ compile for ${cuda_source}"
                DEPFILE ${cuda_source}.d
                # note: don't use verbatim since some cache variables are typed
                # explicitly to STRING so CMake will automatically quote them.
                # this leads to NVCC complaining about unknown options, which
                # are formed from groups of valid compiler options within extra
                # quotes. where necessary we insert our own escaped quotes.
                COMMAND_EXPAND_LISTS
            )
            # update target sources
            target_sources(${target} PRIVATE ${cuda_source}.${obj_ext})
        endforeach()
        # ensure all libraries are linked
        get_property(cuda_libs TARGET ${cuda_group} PROPERTY CUDA_LINK_LIBS)
        foreach(cuda_lib ${cuda_libs})
            # check if compile group
            pdmpmt_cuda_is_compile_group(${cuda_lib} is_compile_group)
            # if not we can use target_link_libraries()
            if(NOT is_compile_group)
                target_link_libraries(${target} PRIVATE ${cuda_lib})
            endif()
        endforeach()
    endforeach()
endfunction()
