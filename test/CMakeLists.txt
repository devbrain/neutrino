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
        video/test_sprites.cc
        video/test_sprite_animation.cc
        video/test_sprite_state.cc
        video/test_cpu_texture_atlas.cc
        video/test_draw.cc
        video/test_sprite_sheet.cc
        video/test_atlas_packer.cc

        world/test_tmx_loader.cc
        world/test_tile_resolution.cc

        detail/test_hash.cc
        detail/test_content_hash.cc
)

target_include_directories(neutrino_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(neutrino_tests
        PRIVATE
        neutrino
        doctest::doctest
)

target_compile_definitions(neutrino_tests
        PRIVATE
        NEUTRINO_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/world/data"
)

if(NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD)
        target_compile_definitions(neutrino_tests
                PRIVATE
                NEUTRINO_TMX_ENABLE_ZSTD)
endif()

add_test(NAME neutrino_tests COMMAND neutrino_tests)
