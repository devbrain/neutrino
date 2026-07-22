# -- ke_format: decode the on-disk KE files into plain data. Knows byte layouts and
#    images; knows nothing about neutrino's world/sprite systems. --
add_library(ke_format STATIC
        format/binary_reader.hh
        format/archive.cc
        format/archive.hh
        format/bob.cc
        format/bob.hh
        format/tab.cc
        format/tab.hh
        format/dig.cc
        format/dig.hh

)
target_include_directories(ke_format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ke_format PUBLIC neutrino)

add_executable(ke
        assets/backdrop.cc
        assets/backdrop.hh
        assets/sprites.cc
        assets/sprites.hh
        assets/registry.cc
        assets/registry.hh

        resources/resources.cc
        resources/resources.hh
        resources/cell.cc
        resources/cell.hh

        scenes/play_game_scene.cc
        scenes/play_game_scene.hh
        scenes/sprite_gallery_scene.cc
        scenes/sprite_gallery_scene.hh

        game/model.cc
        game/model.hh
        game/mechanics.cc
        game/mechanics.hh
        game/sfx.cc
        game/sfx.hh

        krypton_egg.cc
)
target_link_libraries(ke PRIVATE ke_format neutrino)

# Debug tool: labeled contact sheets of KE_SPELL / KE_NMY. Decodes the BOB sheets
# straight from the archive, so it links only ke_format (not the resource container
# or the world/sprite layers).
add_executable(ke_collage
        ke_collage.cc
)
target_link_libraries(ke_collage PRIVATE ke_format neutrino)
