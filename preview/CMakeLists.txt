# SPDX-License-Identifier: MIT
find_package(SDL2 2.0.18 REQUIRED CONFIG)

set(bfm_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_executable(musashi_native_boot
    player.c
    desktop_voice.c
    palace_voice.c
    title_flow.c
    title_view.c
    opening_view.c
    palace.c
    stair_actors.c
    palace_dialogue.c
    scene_tiles.c
    "${bfm_root}/src/overlays/main_0003/800cf104.c"
    "${bfm_root}/pc_port/disc_media.c"
    "${bfm_root}/pc_port/platform/bfm_plat_sha256.c"
    "${bfm_root}/pc_port/platform/bfm_plat_font.c")
target_compile_features(musashi_native_boot PRIVATE c_std_99)
target_include_directories(musashi_native_boot PRIVATE
    "${bfm_root}/pc_port/include"
    "${bfm_root}/pc_port/platform"
    "${bfm_root}/include")
target_link_libraries(musashi_native_boot PRIVATE SDL2::SDL2)
target_compile_definitions(musashi_native_boot PRIVATE MUSASHI_STANDALONE_SHA256=1)

if(NOT MSVC)
    target_link_libraries(musashi_native_boot PRIVATE m)
endif()
