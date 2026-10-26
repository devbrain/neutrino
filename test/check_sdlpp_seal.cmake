# Guard for roadmap P1.7: client code never needs to SPELL sdlpp.
#
#   cmake -DROOT=<repo> [-DMODE=warn|fail] -P check_sdlpp_seal.cmake
#
# Public headers (include/neutrino/**, except detail/): `sdlpp::` is flagged unless the line carries
# the marker `sdlpp-ok` (alias definitions, the get_renderer()/get_window() escape hatch, the entry
# point plumbing). Including <sdlpp/...> from a public header is fine: sdlpp stays reachable.
#
# Client-facing code (tutorials/, examples/, website/docs/): both `sdlpp::` and `<sdlpp/` are
# flagged -- these are what users copy. demos/ and games/ are not scanned: the VGA canvas and KE
# asset loading are genuine low-level code.
#
# MODE=warn (default) reports and passes; MODE=fail fails when anything is flagged.

if(NOT ROOT)
    message(FATAL_ERROR "check_sdlpp_seal: pass -DROOT=<repository root>")
endif()
if(NOT MODE)
    set(MODE warn)
endif()

set(total 0)
set(report "")

# scan(<files> <regex>): counts offending lines per file, skipping lines marked sdlpp-ok.
function(scan files regex)
    foreach(file IN LISTS files)
        # Read whole lines ourselves: file(STRINGS) returns a CMake list, which would split a C++
        # line at every ';' (and mis-split on unbalanced '[' / ']'), separating a code fragment from
        # its `// sdlpp-ok` marker. Neutralize those characters before splitting on newlines.
        file(READ "${file}" content)
        string(REPLACE ";" "<semi>" content "${content}")
        string(REPLACE "[" "<lb>" content "${content}")
        string(REPLACE "]" "<rb>" content "${content}")
        string(REPLACE "\n" ";" lines "${content}")
        set(count 0)
        foreach(line IN LISTS lines)
            if(line MATCHES "${regex}" AND NOT line MATCHES "sdlpp-ok")
                math(EXPR count "${count} + 1")
            endif()
        endforeach()
        if(count GREATER 0)
            file(RELATIVE_PATH rel "${ROOT}" "${file}")
            string(APPEND report "  ${count}\t${rel}\n")
            math(EXPR total "${total} + ${count}")
        endif()
    endforeach()
    set(report "${report}" PARENT_SCOPE)
    set(total ${total} PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE headers "${ROOT}/include/neutrino/*.hh")
list(FILTER headers EXCLUDE REGEX "/include/neutrino/detail/")
scan("${headers}" "sdlpp::")
set(header_total ${total})

file(GLOB_RECURSE clients
     "${ROOT}/tutorials/*.cc" "${ROOT}/tutorials/*.hh"
     "${ROOT}/examples/*.cc" "${ROOT}/examples/*.hh"
     "${ROOT}/website/docs/*.md" "${ROOT}/website/docs/*.mdx")
scan("${clients}" "sdlpp::|<sdlpp/")
math(EXPR client_total "${total} - ${header_total}")

if(total EQUAL 0)
    message(STATUS "sdlpp seal: clean")
    return()
endif()

set(summary "sdlpp seal: ${total} leak(s) -- ${header_total} in public headers, ${client_total} in tutorials/examples/docs\n${report}")
if(MODE STREQUAL "fail")
    message(FATAL_ERROR "${summary}")
else()
    message(WARNING "${summary}")
endif()
