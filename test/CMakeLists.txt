# =============================================================================
# Neutrino Test Suite
# =============================================================================

add_executable(neutrino_tests
        physics/test_collide.cc
        physics/test_enclose.cc
        physics/test_triangle.cc

        physics/test_aabb_tree.cc
        physics/test_grid.cc
        physics/test_sorted_array.cc
        physics/test_units.cc
        physics/test_world.cc
        physics/test_world_carriers.cc
        physics/test_world_entity_dedup.cc
        physics/test_world_footing.cc
        physics/test_world_grid.cc
        physics/test_world_multihit.cc
        physics/test_world_queries.cc
        physics/test_world_run.cc
        physics/test_world_slide.cc
        physics/test_world_snap.cc
        physics/test_world_stepup.cc
        physics/test_world_swept_triggers.cc

        test_main.cc

        audio/test_audio.cc
        input/test_input.cc
        video/test_bitmask.cc
        video/test_cpu_sprites_atlas.cc
)

target_link_libraries(neutrino_tests
        PRIVATE
        neutrino
        doctest::doctest
)

add_test(NAME neutrino_tests COMMAND neutrino_tests)
