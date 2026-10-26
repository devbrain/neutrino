add_library(it_format STATIC
        lzss.hh
        lzss.cc
        it_blocks.hh
        it_blocks.cc
        it_vm.hh
        it_vm.cc
)
target_include_directories(it_format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(it_format PUBLIC neutrino::bstream)

add_executable(it_res_parser it_res_parser.cc)
target_link_libraries(it_res_parser PRIVATE it_format)
