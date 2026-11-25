# SPDX-License-Identifier: MIT
find_package(SDL2 2.0.18 REQUIRED CONFIG)

set(bfm_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_executable(musashi_native_boot
    player.c
    desktop_voice.c
    palace_voice.c
    save_menu.c
    save_view.c
    "${bfm_root}/pc_port/card/card_image.c"
    title_flow.c
    title_view.c
    opening_view.c
    chapter_card.c
    palace.c
    stair_actors.c
    palace_dialogue.c
    scene_tiles.c
    forest_blade.c
    forest_entry.c
    forest_field.c
    forest_manual.c
    forest_motion.c
    forest_pose.c
    forest_raster.c
    forest_terrain.c
    forest_speech_actor.c
    disc_voice.c
    forest_voice.c
    dialogue_contour.c
    forest_message_art.c
    forest_message.c
    forest_walk.c
    scene_payload.c
    "${bfm_root}/pc_port/forest_player_archive.c"
    "${bfm_root}/pc_port/forest_player_mesh.c"
    "${bfm_root}/src/overlays/sc01_0077/8012a018.c"
    "${bfm_root}/src/overlays/sc01_0077/8012a048.c"
    "${bfm_root}/src/overlays/sc01_0077/80134510.c"
    "${bfm_root}/src/overlays/sc01_0077/801345f8.c"
    "${bfm_root}/src/overlays/sc01_0077/801347a0.c"
    "${bfm_root}/src/overlays/sc01_0077/80134a28.c"
    "${bfm_root}/src/overlays/sc01_0077/801473ec.c"
    "${bfm_root}/src/overlays/sc01_0077/80174e9c.c"
    "${bfm_root}/src/overlays/sc01_0077/8017d900.c"
    "${bfm_root}/src/overlays/sc01_0077/8017d98c.c"
    "${bfm_root}/src/overlays/main_0003/800cf104.c"
    "${bfm_root}/pc_port/disc_media.c"
    "${bfm_root}/pc_port/platform/bfm_plat_sha256.c"
    "${bfm_root}/pc_port/platform/bfm_plat_font.c")
target_compile_features(musashi_native_boot PRIVATE c_std_99)
target_include_directories(musashi_native_boot PRIVATE
    "${bfm_root}/pc_port/card"
    "${bfm_root}/pc_port/include"
    "${bfm_root}/pc_port/platform"
    "${bfm_root}/include")
target_link_libraries(musashi_native_boot PRIVATE SDL2::SDL2)
target_compile_definitions(musashi_native_boot PRIVATE MUSASHI_STANDALONE_SHA256=1)

if(NOT MSVC)
    target_link_libraries(musashi_native_boot PRIVATE m)
endif()
