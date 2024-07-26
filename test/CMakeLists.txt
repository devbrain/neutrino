add_executable(unittest
        main.cc
        ini/test_ini.cc
        ecs/test_sorted_array.cc
        ecs/test_bi_map.cc
        ecs/test_component_bucket.cc
)
target_link_libraries(unittest PRIVATE neutrino thirdparty::doctest)