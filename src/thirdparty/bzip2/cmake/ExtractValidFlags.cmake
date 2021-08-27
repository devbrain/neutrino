# Convenience function that checks the availability of certain
# C or C++ compiler flags and returns valid ones as a string.

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(extract_valid_c_flags varname)
    set(valid_flags)
    foreach (flag IN LISTS ARGN)
        string(REGEX REPLACE "[^a-zA-Z0-9_]+" "_" flag_var ${flag})
        set(flag_var "C_FLAG_${flag_var}")
        check_c_compiler_flag("${flag}" "${flag_var}")
        if (${flag_var})
            set(valid_flags "${valid_flags} ${flag}")
        endif ()
    endforeach ()
    set(${varname} "${valid_flags}" PARENT_SCOPE)
endfunction()
