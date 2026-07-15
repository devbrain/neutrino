add_executable(ke
        resources/binary_reader.hh
        resources/ke_cell.cc
        resources/ke_cell.hh
        resources/ke_bob.cc
        resources/ke_bob.hh
        resources/ke_loader.hh
        resources/ke_loader.cc
        resources/ke_world.cc
        resources/ke_world.hh
        resources/ke_assets.cc
        resources/ke_assets.hh
        resources/ke_sprites_def.cc
        resources/ke_sprites_def.hh
        resources/ke_tab.cc
        resources/ke_tab.hh
        krypton_egg.cc
        play_game_scene.cc
        play_game_scene.hh
        sprite_gallery_scene.cc
        sprite_gallery_scene.hh

)

target_link_libraries(ke PRIVATE neutrino)
target_include_directories(ke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Debug tool: labeled contact sheets of KE_SPELL / KE_NMY (needs only the loader chain).
add_executable(ke_collage
        ke_collage.cc
        resources/binary_reader.hh
        resources/ke_cell.cc
        resources/ke_cell.hh
        resources/ke_bob.cc
        resources/ke_bob.hh
        resources/ke_tab.cc
        resources/ke_tab.hh
        resources/ke_loader.cc
        resources/ke_loader.hh
)
target_link_libraries(ke_collage PRIVATE neutrino)
target_include_directories(ke_collage PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})