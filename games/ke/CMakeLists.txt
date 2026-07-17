add_library(ke_resources STATIC
        resources/binary_reader.hh
        resources/ke_cell.cc
        resources/ke_cell.hh
        resources/ke_bob.cc
        resources/ke_bob.hh
        resources/ke_loader.hh
        resources/ke_loader.cc
        resources/ke_world.cc
        resources/ke_world.hh
        resources/ke_sprites_def.cc
        resources/ke_sprites_def.hh
        resources/ke_tab.cc
        resources/ke_tab.hh
        resources/game_resources.cc
        resources/game_resources.hh
)

target_include_directories(ke_resources PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ke_resources PUBLIC neutrino)

add_executable(ke
        scenes/play_game_scene.cc
        scenes/play_game_scene.hh
        scenes/sprite_gallery_scene.cc
        scenes/sprite_gallery_scene.hh

        game/actors_layer.cc
        game/actors_layer.hh

        ke_assets.cc
        ke_assets.hh
        krypton_egg.cc
)

target_link_libraries(ke PRIVATE ke_resources neutrino)
target_include_directories(ke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Debug tool: labeled contact sheets of KE_SPELL / KE_NMY (needs only the loader chain).
add_executable(ke_collage
        ke_collage.cc
)
target_link_libraries(ke_collage PRIVATE neutrino ke_resources)
target_include_directories(ke_collage PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})