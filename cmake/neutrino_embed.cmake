# =============================================================================
# neutrino_embed.cmake — embed files into a target as C++ arrays (bin2c)
# =============================================================================
#
# Included by neutrino's top-level CMakeLists.txt, so every project that adds neutrino
# (FetchContent, add_subdirectory) can call neutrino_embed_files().
#
#   neutrino_embed_files(<target>
#       FILES <file>...              # relative to the calling CMakeLists.txt's directory
#       [NAMESPACE <ns>]             # e.g. game::assets
#       [SYMBOL <name>]              # only with one file; default: from the file name
#       [OUTPUT_DIR <dir>])          # default: <binary dir>/neutrino_embedded/<target>
#
# Each file becomes a header <symbol>.hh in OUTPUT_DIR, made by tools/bin2c/bin2c.cc:
#
#   constexpr std::array<std::uint8_t, N> <symbol> = {{ ... }};
#   inline std::span<const std::uint8_t> <symbol>_data();
#
# The symbol comes from the file name: logo.png is logo_png (string(MAKE_C_IDENTIFIER)).
# The headers are added to the target's sources, OUTPUT_DIR to its include path (PRIVATE),
# and the target is built after them; a header is made again when its file changes.
#
# bin2c is a build-time tool: it runs on the build machine. In a native build it is an
# ordinary target (neutrino_bin2c). When cross-compiling (Emscripten, Android, iOS),
# neutrino-cmake's neutrino_bootstrap_local_tool() compiles it with the host compiler at
# configure time and imports it, so the same rules run the host binary.
# =============================================================================

include_guard(GLOBAL)

include("${NEUTRINO_CMAKE_DIR}/NeutrinoHostTools.cmake")

get_filename_component(_NEUTRINO_BIN2C_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../tools/bin2c/bin2c.cc" ABSOLUTE)
set(NEUTRINO_BIN2C_SOURCE "${_NEUTRINO_BIN2C_SOURCE}" CACHE INTERNAL "neutrino's bin2c source")
unset(_NEUTRINO_BIN2C_SOURCE)

# Makes the neutrino_bin2c tool on first use, so projects that embed nothing do not build it.
function(_neutrino_ensure_bin2c)
    if(TARGET neutrino_bin2c)
        return()
    endif()
    neutrino_bootstrap_local_tool(neutrino_bin2c
        SOURCES "${NEUTRINO_BIN2C_SOURCE}"
        STD 20
    )
    get_target_property(_imported neutrino_bin2c IMPORTED)
    if(NOT _imported)
        target_compile_features(neutrino_bin2c PRIVATE cxx_std_20)
        set_target_properties(neutrino_bin2c PROPERTIES FOLDER "neutrino/tools")
    endif()
endfunction()

function(neutrino_embed_files TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "NAMESPACE;SYMBOL;OUTPUT_DIR" "FILES")
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "neutrino_embed_files(${TARGET}): unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "neutrino_embed_files(${TARGET}): no such target")
    endif()
    if(NOT arg_FILES)
        message(FATAL_ERROR "neutrino_embed_files(${TARGET}): FILES is empty")
    endif()
    list(LENGTH arg_FILES _count)
    if(DEFINED arg_SYMBOL AND NOT _count EQUAL 1)
        message(FATAL_ERROR "neutrino_embed_files(${TARGET}): SYMBOL names one file, but FILES has ${_count}")
    endif()
    if(DEFINED arg_SYMBOL AND NOT arg_SYMBOL MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "neutrino_embed_files(${TARGET}): SYMBOL '${arg_SYMBOL}' is not a C++ identifier")
    endif()
    if(DEFINED arg_NAMESPACE AND NOT arg_NAMESPACE MATCHES "^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*$")
        message(FATAL_ERROR "neutrino_embed_files(${TARGET}): NAMESPACE '${arg_NAMESPACE}' is not a C++ namespace name")
    endif()
    if(NOT arg_OUTPUT_DIR)
        set(arg_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/neutrino_embedded/${TARGET}")
    endif()

    _neutrino_ensure_bin2c()
    file(MAKE_DIRECTORY "${arg_OUTPUT_DIR}")

    get_property(_made GLOBAL PROPERTY _NEUTRINO_EMBEDDED_HEADERS)
    set(_headers)
    foreach(_file IN LISTS arg_FILES)
        get_filename_component(_input "${_file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${_input}" OR IS_DIRECTORY "${_input}")
            message(FATAL_ERROR "neutrino_embed_files(${TARGET}): no file ${_input}")
        endif()
        if(DEFINED arg_SYMBOL)
            set(_symbol "${arg_SYMBOL}")
        else()
            get_filename_component(_name "${_input}" NAME)
            string(MAKE_C_IDENTIFIER "${_name}" _symbol)
        endif()
        set(_header "${arg_OUTPUT_DIR}/${_symbol}.hh")
        if(_header IN_LIST _made OR _header IN_LIST _headers)
            message(FATAL_ERROR
                "neutrino_embed_files(${TARGET}): ${_file} would be embedded as ${_symbol}, which is "
                "already taken in ${arg_OUTPUT_DIR}; give it a SYMBOL of its own")
        endif()

        add_custom_command(
            OUTPUT "${_header}"
            COMMAND "$<TARGET_FILE:neutrino_bin2c>" "${_input}" "${_header}" "${_symbol}" ${arg_NAMESPACE}
            DEPENDS "${_input}" neutrino_bin2c
            COMMENT "Embedding ${_file} as ${_symbol}"
            VERBATIM
        )
        list(APPEND _headers "${_header}")
    endforeach()
    set_property(GLOBAL APPEND PROPERTY _NEUTRINO_EMBEDDED_HEADERS ${_headers})

    target_sources(${TARGET} PRIVATE ${_headers})
    target_include_directories(${TARGET} PRIVATE "${arg_OUTPUT_DIR}")

    # The headers are made by rules in this directory; a target here orders them before
    # <target>, whichever directory <target> was made in.
    get_property(_n GLOBAL PROPERTY _NEUTRINO_EMBED_COUNT)
    if(NOT _n)
        set(_n 0)
    endif()
    math(EXPR _n "${_n} + 1")
    set_property(GLOBAL PROPERTY _NEUTRINO_EMBED_COUNT ${_n})
    add_custom_target(${TARGET}_embedded_${_n} DEPENDS ${_headers})
    add_dependencies(${TARGET} ${TARGET}_embedded_${_n})
endfunction()
