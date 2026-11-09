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
        test_coordinates.cc

        audio/test_audio.cc
        scene/test_base_scene.cc
        scene/test_scenes_manager.cc
        input/test_input.cc
        input/test_input_snapshot.cc
        video/test_bitmask.cc
        video/test_sprites.cc
        video/test_sprite_def.cc
        video/test_sprite_def_builder.cc
        video/test_sprite_set.cc
        video/test_sprite_cache.cc
        video/test_atlas_loader.cc
        video/test_sprite_animation.cc
        video/test_sprite_state.cc
        video/test_draw.cc
        video/test_color.cc
        video/test_palette.cc
        video/test_image_builder.cc
        video/test_image_bank_builder.cc
        video/test_shelf_packer.cc
        video/test_build_atlas.cc
        video/test_register_atlas.cc
        video/test_glyph_atlas.cc
        video/test_font.cc
        video/test_text_layout.cc
        video/test_texture.cc
        video/test_window_presentation.cc
        video/test_sprite_sheet.cc
        video/world/test_tileset_bundle.cc
        video/world/test_resource_cache.cc
        video/world/test_resource_cache_core.cc
        video/world/test_world_renderer.cc
        video/world/test_world_camera.cc
        video/world/test_world_draw.cc
        video/world/test_sprite_batch.cc
        video/world/test_world_compositor.cc

        world/test_tmx_loader.cc
        world/test_tile_resolution.cc

        detail/test_hash.cc
        detail/test_content_hash.cc
        detail/test_enum.cc

        utils/test_lru.cc
)

target_include_directories(neutrino_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/neutrino
)

# The engine's objects rather than the library: tests reach internals that a shared,
# hidden-visibility libneutrino does not export.
target_link_libraries(neutrino_tests
        PRIVATE
        neutrino_objects
        doctest::doctest
)

target_compile_definitions(neutrino_tests
        PRIVATE
        NEUTRINO_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/world/data"
        NEUTRINO_FONT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/video/data/fonts"
)

if(NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD)
        target_compile_definitions(neutrino_tests
                PRIVATE
                NEUTRINO_TMX_ENABLE_ZSTD)
endif()

add_test(NAME neutrino_tests COMMAND neutrino_tests)

# Game formats are tested with literal wire bytes, independently of the writer.
add_executable(game_format_tests test_main.cc test_game_formats.cc)
target_link_libraries(game_format_tests PRIVATE ke_format it_format doctest::doctest)
add_test(NAME game_format_tests COMMAND game_format_tests)

# Exercise the public entry macro separately from the main engine test process.
add_executable(simplex_entry_tests test_main.cc simplex/test_entry_point.cc)
target_link_libraries(simplex_entry_tests PRIVATE neutrino doctest::doctest)
add_test(NAME simplex_entry_tests COMMAND simplex_entry_tests)

# This target uses SDL's generated main, including the normal platform bootstrap.
add_executable(simplex_entry_smoke simplex/entry_point_smoke.cc)
target_link_libraries(simplex_entry_smoke PRIVATE neutrino)
add_test(NAME simplex_entry_smoke COMMAND simplex_entry_smoke)
set_tests_properties(simplex_entry_smoke PROPERTIES
        TIMEOUT 15
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")

# Client code never needs to spell sdlpp. Fails on any `sdlpp::` in a public header
# (outside alias definitions marked sdlpp-ok) or any sdlpp spelling in tutorials/examples/docs.
add_test(NAME sdlpp_seal
         COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DMODE=fail
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/check_sdlpp_seal.cmake)
