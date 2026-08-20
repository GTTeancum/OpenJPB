cmake_minimum_required(VERSION 3.20)
project(jedi_power_battles_reconstruction LANGUAGES C CXX)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JPB_GAME_DATA_DIR "" CACHE PATH
    "Optional original game installation root for real-asset tests")
set(JPB_UFBX_SOURCE_DIR "" CACHE PATH
    "Optional exact ufbx 0.6.1 source tree for PC FBX level rendering and evidence probes")

file(GLOB_RECURSE JPB_RECONSTRUCTED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/reconstructed/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/reconstructed/*.cpp")
list(FILTER JPB_RECONSTRUCTED_SOURCES EXCLUDE REGEX
    "/src/reconstructed/scaffold_anchor\\.c$")

add_library(jpb_reconstructed STATIC
    src/reconstructed/scaffold_anchor.c
    ${JPB_RECONSTRUCTED_SOURCES})

target_include_directories(jpb_reconstructed PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(NOT MSVC)
    target_link_libraries(jpb_reconstructed PUBLIC m)
endif()

add_executable(jpb_host_scaffold src/port/bootstrap.c)
target_link_libraries(jpb_host_scaffold PRIVATE jpb_reconstructed)

add_executable(jpb_asset_probe src/port/asset_probe.c)
target_link_libraries(jpb_asset_probe PRIVATE jpb_reconstructed)

add_executable(jpb_cad_probe src/port/cad_probe.c)
target_link_libraries(jpb_cad_probe PRIVATE jpb_reconstructed)

add_executable(jpb_bmd_probe src/port/bmd_probe.c)
target_link_libraries(jpb_bmd_probe PRIVATE jpb_reconstructed)

add_executable(jpb_animation_probe src/port/animation_probe.c)
target_link_libraries(jpb_animation_probe PRIVATE jpb_reconstructed)

add_executable(jpb_jpx_probe src/port/jpx_probe.c)
target_link_libraries(jpb_jpx_probe PRIVATE jpb_reconstructed)

add_executable(jpb_jpx_preview src/port/jpx_preview.c)
target_link_libraries(jpb_jpx_preview PRIVATE jpb_reconstructed)

if(JPB_UFBX_SOURCE_DIR)
    if(NOT EXISTS "${JPB_UFBX_SOURCE_DIR}/ufbx.c" OR
       NOT EXISTS "${JPB_UFBX_SOURCE_DIR}/ufbx.h")
        message(FATAL_ERROR
            "JPB_UFBX_SOURCE_DIR must contain ufbx.c and ufbx.h")
    endif()
    add_executable(
        jpb_fbx_level_probe
        src/port/fbx_level_probe.c
        "${JPB_UFBX_SOURCE_DIR}/ufbx.c")
    target_include_directories(
        jpb_fbx_level_probe PRIVATE "${JPB_UFBX_SOURCE_DIR}")
    target_link_libraries(jpb_fbx_level_probe PRIVATE jpb_reconstructed)

    add_executable(
        jpb_fbx_jpx_match_probe
        src/port/fbx_jpx_match_probe.c
        "${JPB_UFBX_SOURCE_DIR}/ufbx.c")
    target_include_directories(
        jpb_fbx_jpx_match_probe PRIVATE "${JPB_UFBX_SOURCE_DIR}")
    target_link_libraries(
        jpb_fbx_jpx_match_probe PRIVATE jpb_reconstructed)
endif()

if(WIN32)
    add_executable(
        jpb_retail_camera_watch
        tools/retail_camera_watch.c)
    target_include_directories(
        jpb_retail_camera_watch PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(
        jpb_retail_camera_watch PRIVATE winmm)

    add_executable(
        jpb_pc_game
        src/port/pc_game_win32.c
        src/port/pc_audio_win32.c
        src/port/pc_image_wic.c
        src/port/pc_input_mapping.c
        src/port/pc_log_win32.c
        src/port/pc_present_d3d11.c
        src/port/pc_xinput_win32.c)
    target_link_libraries(
        jpb_pc_game PRIVATE jpb_reconstructed user32 gdi32 winmm
        ole32 windowscodecs d3d11 dxgi dxguid d3dcompiler)
    if(JPB_UFBX_SOURCE_DIR)
        target_sources(
            jpb_pc_game PRIVATE
            src/port/pc_level_fbx.c
            "${JPB_UFBX_SOURCE_DIR}/ufbx.c")
        target_include_directories(
            jpb_pc_game PRIVATE "${JPB_UFBX_SOURCE_DIR}")
        target_compile_definitions(
            jpb_pc_game PRIVATE JPB_PC_HAS_UFBX=1)
    endif()

    add_executable(
        jpb_pc_audio_probe
        src/port/pc_audio_probe.c
        src/port/pc_audio_win32.c
        src/port/pc_log_win32.c)
    target_link_libraries(
        jpb_pc_audio_probe PRIVATE jpb_reconstructed winmm)
endif()

if(BUILD_TESTING)
    target_compile_definitions(jpb_reconstructed PRIVATE JPB_FMATH_TESTING)

    if(WIN32)
        add_executable(
            jpb_pc_xinput_tests
            tests/test_pc_xinput_win32.c
            src/port/pc_input_mapping.c
            src/port/pc_xinput_win32.c)
        target_include_directories(
            jpb_pc_xinput_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/port")
        target_link_libraries(
            jpb_pc_xinput_tests PRIVATE jpb_reconstructed)
        add_test(NAME jpb_pc_xinput_tests COMMAND jpb_pc_xinput_tests)
        set_tests_properties(
            jpb_pc_xinput_tests
            PROPERTIES LABELS "pc_game;controls;xinput;input")

        add_executable(
            jpb_pc_log_tests
            tests/test_pc_log_win32.c
            src/port/pc_log_win32.c)
        target_include_directories(
            jpb_pc_log_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/port")
        add_test(NAME jpb_pc_log_tests COMMAND jpb_pc_log_tests)
        set_tests_properties(
            jpb_pc_log_tests
            PROPERTIES LABELS "pc_game;logging")

        add_executable(
            jpb_pc_audio_tests
            tests/test_pc_audio_win32.c
            src/port/pc_audio_win32.c
            src/port/pc_log_win32.c)
        target_link_libraries(
            jpb_pc_audio_tests PRIVATE jpb_reconstructed winmm)
        add_test(NAME jpb_pc_audio_tests COMMAND jpb_pc_audio_tests)
        if(JPB_GAME_DATA_DIR AND EXISTS
           "${JPB_GAME_DATA_DIR}/res/sound")
            add_test(
                NAME jpb_pc_audio_real_corpus
                COMMAND
                    jpb_pc_audio_tests
                    --corpus
                    "${JPB_GAME_DATA_DIR}/res/sound")
            set_tests_properties(
                jpb_pc_audio_real_corpus
                PROPERTIES LABELS "real_assets;pc_game;audio")
        endif()
    endif()

    add_executable(
        jpb_audio_stream_tests tests/test_audio_stream.c)
    target_link_libraries(
        jpb_audio_stream_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_audio_stream_tests COMMAND jpb_audio_stream_tests)

    add_executable(jpb_list_tests tests/test_list.c)
    target_link_libraries(jpb_list_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_list_tests COMMAND jpb_list_tests)

    add_executable(jpb_timer_tests tests/test_timer.c)
    target_link_libraries(jpb_timer_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_timer_tests COMMAND jpb_timer_tests)

    add_executable(jpb_alloc_tests tests/test_alloc.c)
    target_link_libraries(jpb_alloc_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_alloc_tests COMMAND jpb_alloc_tests)

    add_executable(jpb_bullet_tests tests/test_bullet.c)
    target_link_libraries(jpb_bullet_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_bullet_tests COMMAND jpb_bullet_tests)

    add_executable(jpb_memory_tests tests/test_memory.c)
    target_link_libraries(jpb_memory_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_memory_tests COMMAND jpb_memory_tests)

    add_executable(jpb_fmath_tests tests/test_fmath.c)
    target_compile_definitions(jpb_fmath_tests PRIVATE JPB_FMATH_TESTING)
    target_link_libraries(jpb_fmath_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_fmath_tests COMMAND jpb_fmath_tests)

    add_executable(
        jpb_force_activate_tests tests/test_force_activate.c)
    target_link_libraries(
        jpb_force_activate_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_force_activate_tests
        COMMAND jpb_force_activate_tests)

    add_executable(jpb_force_tests tests/test_force.c)
    target_link_libraries(
        jpb_force_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_force_tests COMMAND jpb_force_tests)

    add_executable(jpb_vectors_tests tests/test_vectors.c)
    target_link_libraries(jpb_vectors_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_vectors_tests COMMAND jpb_vectors_tests)

    add_executable(jpb_io_filesys_tests tests/test_io_filesys.c)
    target_link_libraries(jpb_io_filesys_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_io_filesys_tests COMMAND jpb_io_filesys_tests)

    add_executable(jpb_jpx_tests tests/test_jpx.c)
    target_link_libraries(jpb_jpx_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_jpx_tests COMMAND jpb_jpx_tests)

    add_executable(jpb_projection_tests tests/test_projection.c)
    target_link_libraries(jpb_projection_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_projection_tests COMMAND jpb_projection_tests)

    add_executable(
        jpb_portable_text_tests tests/test_portable_text.c)
    target_link_libraries(
        jpb_portable_text_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_portable_text_tests
        COMMAND jpb_portable_text_tests)
    if(JPB_GAME_DATA_DIR AND EXISTS
       "${JPB_GAME_DATA_DIR}/res/font/NotoSansSC-Bold.ttf")
        add_test(
            NAME jpb_portable_text_real_font
            COMMAND
                jpb_portable_text_tests
                "${JPB_GAME_DATA_DIR}")
        set_tests_properties(
            jpb_portable_text_real_font
            PROPERTIES LABELS "real_assets;pc_game;presentation")
    endif()

    add_executable(jpb_render_node_tests tests/test_render_nodes.c)
    target_link_libraries(jpb_render_node_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_render_node_tests COMMAND jpb_render_node_tests)

    add_executable(jpb_camera_tests tests/test_camera.c)
    target_link_libraries(jpb_camera_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_camera_tests COMMAND jpb_camera_tests)

    add_executable(jpb_material_tests tests/test_material.c)
    target_link_libraries(jpb_material_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_material_tests COMMAND jpb_material_tests)

    add_executable(jpb_texture_tests tests/test_texture.c)
    target_link_libraries(jpb_texture_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_texture_tests COMMAND jpb_texture_tests)

    add_executable(
        jpb_transparent_texture_database_tests
        tests/test_transparent_texture_database.c)
    target_link_libraries(
        jpb_transparent_texture_database_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_transparent_texture_database_tests
        COMMAND jpb_transparent_texture_database_tests)

    add_executable(jpb_combo_tests tests/test_combo.c)
    target_link_libraries(
        jpb_combo_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_combo_tests COMMAND jpb_combo_tests)

    add_executable(jpb_input_tests tests/test_input.c)
    target_link_libraries(jpb_input_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_input_tests COMMAND jpb_input_tests)
    set_tests_properties(
        jpb_input_tests PROPERTIES
        LABELS "pc_game;controls;input;haptics")

    add_executable(jpb_collision_tests tests/test_collision.c)
    target_link_libraries(jpb_collision_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_collision_tests COMMAND jpb_collision_tests)

    add_executable(jpb_player_tests tests/test_player.c)
    target_link_libraries(jpb_player_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_player_tests COMMAND jpb_player_tests)

    add_executable(jpb_game_tests tests/test_game.c)
    target_link_libraries(jpb_game_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_game_tests COMMAND jpb_game_tests)

    add_executable(jpb_menu_tests tests/test_menu.c)
    target_link_libraries(jpb_menu_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_menu_tests COMMAND jpb_menu_tests)

    add_executable(
        jpb_game_runtime_title_tests
        tests/test_game_runtime_title.c)
    target_link_libraries(
        jpb_game_runtime_title_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_game_runtime_title_tests
        COMMAND jpb_game_runtime_title_tests)

    add_executable(jpb_pwrup_tests tests/test_pwrup.c)
    target_link_libraries(jpb_pwrup_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_pwrup_tests COMMAND jpb_pwrup_tests)

    add_executable(
        jpb_boss_vehicle_tests tests/test_boss_vehicle.c)
    target_link_libraries(
        jpb_boss_vehicle_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_boss_vehicle_tests
        COMMAND jpb_boss_vehicle_tests)

    add_executable(jpb_sound_tests tests/test_sound.c)
    target_link_libraries(jpb_sound_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_sound_tests COMMAND jpb_sound_tests)

    add_executable(
        jpb_extracharacter_tests tests/test_extracharacters.c)
    target_link_libraries(
        jpb_extracharacter_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_extracharacter_tests
        COMMAND jpb_extracharacter_tests)

    add_executable(jpb_jonny_tests tests/test_jonny.c)
    target_link_libraries(jpb_jonny_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_jonny_tests COMMAND jpb_jonny_tests)

    add_executable(jpb_intersec_tests tests/test_intersec.c)
    target_link_libraries(jpb_intersec_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_intersec_tests COMMAND jpb_intersec_tests)

    add_executable(jpb_sprite_tests tests/test_sprite.c)
    target_link_libraries(jpb_sprite_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_sprite_tests COMMAND jpb_sprite_tests)

    add_executable(jpb_physics_tests tests/test_physics.c)
    target_link_libraries(jpb_physics_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_physics_tests COMMAND jpb_physics_tests)

    add_executable(jpb_anim_tests tests/test_anim.c)
    target_link_libraries(jpb_anim_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_anim_tests COMMAND jpb_anim_tests)

    add_executable(jpb_resource_tests tests/test_resources.c)
    target_link_libraries(
        jpb_resource_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_resource_tests COMMAND jpb_resource_tests)

    add_executable(jpb_brain_tests tests/test_brain.c)
    target_link_libraries(jpb_brain_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_brain_tests COMMAND jpb_brain_tests)

    add_executable(jpb_braindmg_tests tests/test_braindmg.c)
    target_link_libraries(
        jpb_braindmg_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_braindmg_tests
        COMMAND jpb_braindmg_tests)

    add_executable(
        jpb_brain_control_tests tests/test_brain_control.c)
    target_link_libraries(
        jpb_brain_control_tests PRIVATE jpb_reconstructed)
    add_test(
        NAME jpb_brain_control_tests
        COMMAND jpb_brain_control_tests)

    add_executable(jpb_enemy_tests tests/test_enemy.c)
    target_link_libraries(
        jpb_enemy_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_enemy_tests COMMAND jpb_enemy_tests)

    add_executable(jpb_shaolin_tests tests/test_shaolin.c)
    target_link_libraries(
        jpb_shaolin_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_shaolin_tests COMMAND jpb_shaolin_tests)

    add_executable(jpb_brainutl_tests tests/test_brainutl.c)
    target_link_libraries(jpb_brainutl_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_brainutl_tests COMMAND jpb_brainutl_tests)

    add_executable(jpb_cad_tests tests/test_cad.c)
    target_link_libraries(jpb_cad_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_cad_tests COMMAND jpb_cad_tests)

    add_executable(jpb_bmd_tests tests/test_bmd.c)
    target_link_libraries(jpb_bmd_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_bmd_tests COMMAND jpb_bmd_tests)

    add_executable(jpb_model_tests tests/test_model.c)
    target_link_libraries(jpb_model_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_model_tests COMMAND jpb_model_tests)

    add_executable(jpb_tga_tests tests/test_tga.c)
    target_link_libraries(jpb_tga_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_tga_tests COMMAND jpb_tga_tests)

    add_executable(jpb_object_scene_tests tests/test_object_scene.c)
    target_link_libraries(jpb_object_scene_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_object_scene_tests COMMAND jpb_object_scene_tests)

    add_executable(jpb_model_pose_tests tests/test_model_pose.c)
    target_link_libraries(jpb_model_pose_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_model_pose_tests COMMAND jpb_model_pose_tests)

    add_executable(jpb_level_world_tests tests/test_level_world.c)
    target_link_libraries(jpb_level_world_tests PRIVATE jpb_reconstructed)
    add_test(NAME jpb_level_world_tests COMMAND jpb_level_world_tests)

    if(JPB_GAME_DATA_DIR)
        file(GLOB JPB_POWERUP_ASSETS CONFIGURE_DEPENDS
            "${JPB_GAME_DATA_DIR}/res/level/powerups/*.pwr")
        foreach(JPB_POWERUP_ASSET IN LISTS JPB_POWERUP_ASSETS)
            get_filename_component(
                JPB_POWERUP_NAME "${JPB_POWERUP_ASSET}" NAME_WE)
            string(MAKE_C_IDENTIFIER
                "${JPB_POWERUP_NAME}" JPB_POWERUP_SUFFIX)
            add_test(
                NAME "jpb_real_powerup_${JPB_POWERUP_SUFFIX}"
                COMMAND
                    jpb_pwrup_tests --file "${JPB_POWERUP_ASSET}")
            set_tests_properties(
                "jpb_real_powerup_${JPB_POWERUP_SUFFIX}"
                PROPERTIES LABELS "real_assets;powerup_assets")
        endforeach()

        set(JPB_LEVEL_ARCHIVE_DIR
            "${JPB_GAME_DATA_DIR}/res/level/W3D")
        file(GLOB JPB_LEVEL_ARCHIVES CONFIGURE_DEPENDS
            "${JPB_LEVEL_ARCHIVE_DIR}/*.j3d")
        if(NOT JPB_LEVEL_ARCHIVES)
            message(WARNING
                "JPB_GAME_DATA_DIR contains no res/level/W3D/*.j3d archives")
        endif()
        foreach(JPB_LEVEL_ARCHIVE IN LISTS JPB_LEVEL_ARCHIVES)
            get_filename_component(
                JPB_LEVEL_ARCHIVE_NAME "${JPB_LEVEL_ARCHIVE}" NAME_WE)
            string(MAKE_C_IDENTIFIER
                "${JPB_LEVEL_ARCHIVE_NAME}" JPB_LEVEL_TEST_SUFFIX)
            add_test(
                NAME "jpb_real_asset_${JPB_LEVEL_TEST_SUFFIX}"
                COMMAND jpb_asset_probe "${JPB_LEVEL_ARCHIVE}")
            set_tests_properties(
                "jpb_real_asset_${JPB_LEVEL_TEST_SUFFIX}"
                PROPERTIES LABELS "real_assets")
        endforeach()

        file(GLOB_RECURSE JPB_WORLD_MESHES CONFIGURE_DEPENDS
            "${JPB_GAME_DATA_DIR}/res/level/jpx/*.jpx")
        foreach(JPB_WORLD_MESH IN LISTS JPB_WORLD_MESHES)
            get_filename_component(
                JPB_WORLD_MESH_NAME "${JPB_WORLD_MESH}" NAME_WE)
            get_filename_component(
                JPB_WORLD_MESH_DIR "${JPB_WORLD_MESH}" DIRECTORY)
            get_filename_component(
                JPB_WORLD_MESH_LEVEL "${JPB_WORLD_MESH_DIR}" NAME)
            string(MAKE_C_IDENTIFIER
                "${JPB_WORLD_MESH_LEVEL}_${JPB_WORLD_MESH_NAME}"
                JPB_WORLD_MESH_TEST_SUFFIX)
            add_test(
                NAME "jpb_real_mesh_${JPB_WORLD_MESH_TEST_SUFFIX}"
                COMMAND jpb_jpx_probe "${JPB_WORLD_MESH}")
            set_tests_properties(
                "jpb_real_mesh_${JPB_WORLD_MESH_TEST_SUFFIX}"
                PROPERTIES LABELS "real_assets")
        endforeach()

        file(GLOB JPB_ANIMATION_ASSETS CONFIGURE_DEPENDS
            "${JPB_GAME_DATA_DIR}/res/animation/*.cad")
        set(JPB_HUFFMAN_TABLE
            "${JPB_GAME_DATA_DIR}/res/animation/huffman.tab")
        set(JPB_HUFFMAN_VALUES
            "${JPB_GAME_DATA_DIR}/res/animation/huffman.val")
        set(JPB_HUFFMAN_OPTIONS
            "${JPB_GAME_DATA_DIR}/res/animation/huffman.opt")
        foreach(JPB_ANIMATION_ASSET IN LISTS JPB_ANIMATION_ASSETS)
            get_filename_component(
                JPB_ANIMATION_NAME
                "${JPB_ANIMATION_ASSET}" NAME_WE)
            string(MAKE_C_IDENTIFIER
                "${JPB_ANIMATION_NAME}" JPB_ANIMATION_SUFFIX)
            add_test(
                NAME "jpb_real_cad_${JPB_ANIMATION_SUFFIX}"
                COMMAND jpb_cad_probe "${JPB_ANIMATION_ASSET}")
            set_tests_properties(
                "jpb_real_cad_${JPB_ANIMATION_SUFFIX}"
                PROPERTIES LABELS "real_assets;animation_assets")
            if(EXISTS "${JPB_HUFFMAN_TABLE}" AND
               EXISTS "${JPB_HUFFMAN_VALUES}" AND
               EXISTS "${JPB_HUFFMAN_OPTIONS}")
                add_test(
                    NAME "jpb_real_animation_${JPB_ANIMATION_SUFFIX}"
                    COMMAND
                        jpb_animation_probe
                        "${JPB_ANIMATION_ASSET}"
                        "${JPB_HUFFMAN_TABLE}"
                        "${JPB_HUFFMAN_VALUES}"
                        "${JPB_HUFFMAN_OPTIONS}")
                set_tests_properties(
                    "jpb_real_animation_${JPB_ANIMATION_SUFFIX}"
                    PROPERTIES
                        LABELS
                        "real_assets;animation_assets;decoded_animation")
            endif()
        endforeach()

        set(JPB_KI_ADI_ANIMATION_ASSET
            "${JPB_GAME_DATA_DIR}/res/animation/ki_adi.cad")
        if(EXISTS "${JPB_KI_ADI_ANIMATION_ASSET}")
            add_test(
                NAME jpb_real_cad_ki_adi_stab_metadata
                COMMAND
                    jpb_cad_probe
                    "${JPB_KI_ADI_ANIMATION_ASSET}"
                    --motions)
            set_tests_properties(
                jpb_real_cad_ki_adi_stab_metadata
                PROPERTIES
                    LABELS
                        "real_assets;animation_assets;controls;combo"
                    PASS_REGULAR_EXPRESSION
                        "motion\\[130\\] name=jd_stab_r_base.*cut=\\(0,0\\).*disp=24 frames=\\(0,19\\) pre_roll=0")
        endif()

        file(GLOB JPB_MODEL_ASSETS CONFIGURE_DEPENDS
            "${JPB_GAME_DATA_DIR}/res/MODEL/*.bmd")
        foreach(JPB_MODEL_ASSET IN LISTS JPB_MODEL_ASSETS)
            get_filename_component(
                JPB_MODEL_NAME "${JPB_MODEL_ASSET}" NAME_WE)
            string(MAKE_C_IDENTIFIER
                "${JPB_MODEL_NAME}" JPB_MODEL_SUFFIX)
            if(JPB_MODEL_NAME STREQUAL "skin2")
                add_test(
                    NAME "jpb_real_model_${JPB_MODEL_SUFFIX}_known_truncated"
                    COMMAND
                        jpb_bmd_probe "${JPB_MODEL_ASSET}"
                        --expect-result -3)
                set(JPB_MODEL_TEST_NAME
                    "jpb_real_model_${JPB_MODEL_SUFFIX}_known_truncated")
            elseif(
                JPB_MODEL_NAME STREQUAL "weasel" OR
                JPB_MODEL_NAME MATCHES "^gate(_[123])?$")
                add_test(
                    NAME "jpb_real_model_${JPB_MODEL_SUFFIX}_known_out_of_range"
                    COMMAND
                        jpb_bmd_probe "${JPB_MODEL_ASSET}"
                        --expect-geometry-result -4)
                set(JPB_MODEL_TEST_NAME
                    "jpb_real_model_${JPB_MODEL_SUFFIX}_known_out_of_range")
            else()
                add_test(
                    NAME "jpb_real_model_${JPB_MODEL_SUFFIX}"
                    COMMAND
                        jpb_bmd_probe "${JPB_MODEL_ASSET}"
                        --validate-geometry)
                set(JPB_MODEL_TEST_NAME
                    "jpb_real_model_${JPB_MODEL_SUFFIX}")
            endif()
            set_tests_properties(
                "${JPB_MODEL_TEST_NAME}"
                PROPERTIES LABELS "real_assets;model_assets")
        endforeach()

        if(WIN32)
            set(JPB_PC_GAME_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/fed/fed.jpx")
            set(JPB_PC_GAME_CAD
                "${JPB_GAME_DATA_DIR}/res/animation/obi_wan.cad")
            set(JPB_PC_GAME_BMD
                "${JPB_GAME_DATA_DIR}/res/MODEL/obi_wan.bmd")
            set(JPB_PC_THEED_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/theed/theed.jpx")
            set(JPB_PC_MINI2_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/mini2/mini2.jpx")
            set(JPB_PC_PALACE_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/palace/palace.jpx")
            set(JPB_PC_MARSH_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/marsh/marsh.jpx")
            set(JPB_PC_CORUS1_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/corus1/corus1.jpx")
            set(JPB_PC_MINI3_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/mini3/mini3.jpx")
            set(JPB_PC_ARENA_MESH
                "${JPB_GAME_DATA_DIR}/res/level/jpx/arena/arena.jpx")
            set(JPB_PC_GAME_CMB
                "${JPB_GAME_DATA_DIR}/res/combo/obi_wan.cmb")
            set(JPB_PC_GAME_ENEMY_CAD
                "${JPB_GAME_DATA_DIR}/res/animation/battle_d.cad")
            set(JPB_PC_GAME_ENEMY_BMD
                "${JPB_GAME_DATA_DIR}/res/MODEL/battle_d.bmd")
            set(JPB_PC_PLAYER_TWO_CAD
                "${JPB_GAME_DATA_DIR}/res/animation/qui_gon.cad")
            set(JPB_PC_PLAYER_TWO_BMD
                "${JPB_GAME_DATA_DIR}/res/MODEL/qui_gon.bmd")
            set(JPB_PC_PLAYER_TWO_CMB
                "${JPB_GAME_DATA_DIR}/res/combo/qui_gon.cmb")
            set(JPB_PC_KI_ADI_CAD
                "${JPB_GAME_DATA_DIR}/res/animation/ki_adi.cad")
            set(JPB_PC_KI_ADI_BMD
                "${JPB_GAME_DATA_DIR}/res/MODEL/ki_adi.bmd")
            set(JPB_PC_KI_ADI_CMB
                "${JPB_GAME_DATA_DIR}/res/combo/ki_adi.cmb")
            if(EXISTS "${JPB_PC_GAME_MESH}" AND
               EXISTS "${JPB_PC_GAME_CAD}" AND
               EXISTS "${JPB_PC_GAME_BMD}" AND
               EXISTS "${JPB_PC_GAME_CMB}" AND
               EXISTS "${JPB_PC_GAME_ENEMY_CAD}" AND
               EXISTS "${JPB_PC_GAME_ENEMY_BMD}")
                add_test(
                    NAME jpb_pc_game_smoke
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --headless-move
                        # The original post-render player scheduler publishes
                        # locomotion for the following animation/physics tick.
                        --frames 3
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_game_smoke.ppm")
                set_tests_properties(
                    jpb_pc_game_smoke
                    PROPERTIES
                        LABELS "real_assets;pc_game;powerups"
                        PASS_REGULAR_EXPRESSION "powerups=55/")
                if(EXISTS "${JPB_PC_KI_ADI_CAD}" AND
                   EXISTS "${JPB_PC_KI_ADI_BMD}" AND
                   EXISTS "${JPB_PC_KI_ADI_CMB}")
                    # Exact executable scheduling exposes an authored
                    # incompatibility in Ki-Adi's first south-combo motion:
                    # Motion 130 enters combo lock 30, disp=24 clamps against
                    # Lframe=19, and recovery wins before a released/repressed
                    # second south edge can form s.s. Protect that provenance
                    # rather than adding a character-specific timing exception
                    # with no source evidence.
                    add_test(
                        NAME jpb_pc_ki_adi_stab_slack_provenance
                        COMMAND
                            jpb_pc_game "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_KI_ADI_CAD}"
                            --bmd "${JPB_PC_KI_ADI_BMD}"
                            --cmb "${JPB_PC_KI_ADI_CMB}"
                            --player-model 8
                            --headless --mute
                            --headless-maximum-progression
                            --headless-phase-p1 attack-south 1
                            --headless-phase-p1 none 1
                            --headless-phase-p1 attack-south 1
                            --headless-phase-p1 none 1
                            --frames 4)
                    set_tests_properties(
                        jpb_pc_ki_adi_stab_slack_provenance
                        PROPERTIES
                            LABELS
                                "real_assets;pc_game;controls;combo;roster;provenance"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=2.*combo_tally=2/1 pre=s\\..*queue=<empty> lock=30"
                            TIMEOUT 120)
                endif()
                if(EXISTS "${JPB_PC_ARENA_MESH}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_CAD}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_BMD}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_CMB}")
                    add_test(
                        NAME jpb_pc_versus_runtime
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --headless --frames 3)
                    set_tests_properties(
                        jpb_pc_versus_runtime
                        PROPERTIES
                            LABELS "real_assets;pc_game;menu;versus"
                            PASS_REGULAR_EXPRESSION
                                "second_player=\\(ready=1,triangles=[1-9][0-9]*,pixels=[1-9][0-9]*\\).*camera=\\(dolly=0,[^\r\n]*authored=0,[^\r\n]*collision=0\\.4170/3")
                endif()
                add_test(
                    NAME jpb_pc_authored_intro_camera
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --camera-diagnostics
                        --frames 3)
                set_tests_properties(
                    jpb_pc_authored_intro_camera
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera;enemy_ai"
                        PASS_REGULAR_EXPRESSION
                            "camera=\\(dolly=144,.*authored=3,collision=1\\.0000/0,.*\\).*player_flags=00000002")
                if(EXISTS "${JPB_PC_MINI2_MESH}")
                    add_test(
                        NAME jpb_pc_mini2_kaadu_camera
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_MINI2_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless
                            --camera-diagnostics
                            --validate-camera-follow
                            --frames 3)
                    set_tests_properties(
                        jpb_pc_mini2_kaadu_camera
                        PROPERTIES
                            LABELS "real_assets;pc_game;camera;mini2"
                            PASS_REGULAR_EXPRESSION
                                "model_pixels=[1-9][0-9]*.*target=visible.*camera_state=\\(type=6")

                    add_test(
                        NAME jpb_pc_mini2_solo_kadu_riders
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_MINI2_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                            --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                            --headless --mute --frames 120)
                    set_tests_properties(
                        jpb_pc_mini2_solo_kadu_riders
                        PROPERTIES
                            LABELS "real_assets;pc_game;mini2;enemy_ai"
                            PASS_REGULAR_EXPRESSION
                                "second_player=\\(ready=1,triangles=[1-9][0-9]*,pixels=[1-9][0-9]*\\)"
                            TIMEOUT 120)
                endif()
                if(EXISTS "${JPB_PC_PALACE_MESH}")
                    add_test(
                        NAME jpb_pc_palace_enemy_range_machinery
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_PALACE_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                            --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                            --headless --mute --frames 120)
                    set_tests_properties(
                        jpb_pc_palace_enemy_range_machinery
                        PROPERTIES
                            LABELS "real_assets;pc_game;palace;enemy_ai"
                            PASS_REGULAR_EXPRESSION
                                "frames=120 .*enemy=\\(actors=[1-9][0-9]*/"
                            TIMEOUT 120)
                endif()
                if(EXISTS "${JPB_PC_MARSH_MESH}")
                    add_test(
                        NAME jpb_pc_marsh_actor_classes
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_MARSH_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                            --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                            --headless --mute --frames 1)
                    set_tests_properties(
                        jpb_pc_marsh_actor_classes
                        PROPERTIES
                            LABELS "real_assets;pc_game;marsh;enemy_ai"
                            PASS_REGULAR_EXPRESSION
                                "classes=12/12/"
                            TIMEOUT 120)
                endif()
                if(EXISTS "${JPB_PC_MINI3_MESH}")
                    add_test(
                        NAME jpb_pc_mini3_actor_classes
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_MINI3_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                            --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                            --headless --mute --frames 1)
                    set_tests_properties(
                        jpb_pc_mini3_actor_classes
                        PROPERTIES
                            LABELS "real_assets;pc_game;mini3;enemy_ai"
                            PASS_REGULAR_EXPRESSION
                                "camera=\\(dolly=2,[^\r\n]*authored=1,[^\r\n]*collision=1\\.0000/0.*classes=5/5/"
                            TIMEOUT 120)
                endif()
                add_test(
                    NAME jpb_pc_authored_camera_director
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --camera-diagnostics
                        --framebuffer-size 960 540
                        --frames 180)
                set_tests_properties(
                    jpb_pc_authored_camera_director
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera;enemy_ai"
                        PASS_REGULAR_EXPRESSION
                            "camera=\\(dolly=145,.*authored=180,collision=1\\.0000/0,.*\\).*player_flags=00000002"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_authored_camera_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --camera-diagnostics
                        --framebuffer-size 960 540
                        --frames 360)
                set_tests_properties(
                    jpb_pc_authored_camera_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera;enemy_ai;fed;progression"
                        PASS_REGULAR_EXPRESSION
                            "camera=\\(dolly=56,flags=0000101a,initial=56,unique=4,transitions=4,authored=360,collision=1\\.0000/0,.*\\).*physics=\\(22448\\.6,3328\\.0,-14059\\.3,facing=2091\\).*player_flags=00000000.*player_ai=\\(attached=0,transitions=1/1,last=151/4\\)"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_authored_camera_settle
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --camera-diagnostics
                        --framebuffer-size 960 540
                        --frames 480)
                set_tests_properties(
                    jpb_pc_authored_camera_settle
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera;enemy_ai;fed;progression"
                        PASS_REGULAR_EXPRESSION
                            "camera=\\(dolly=56,flags=0000101a,initial=56,unique=4,transitions=4,authored=480,collision=1\\.0000/0,.*\\).*target=visible\\(459\\.7,276\\.3,0\\.104\\).*physics=\\(22395\\.2,3328\\.0,-13754\\.0,facing=2091\\).*player_ai=\\(attached=0,transitions=1/1,last=151/4\\)"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_authored_camera_gameplay_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --control-harness
                        --headless-phase none 360
                        --headless-phase up 59
                        --camera-diagnostics
                        --validate-camera-follow
                        --framebuffer-size 960 540
                        --frames 420)
                set_tests_properties(
                    jpb_pc_authored_camera_gameplay_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera;enemy_ai"
                        PASS_REGULAR_EXPRESSION
                            "camera=\\(dolly=56,flags=0000101a,initial=56,unique=4,transitions=4,authored=420,collision=1\\.0000/0,.*\\).*target=visible\\([^\\)]*\\).*player_ai=\\(attached=0,transitions=1/1,last=151/4\\)"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_authored_camera_spawn
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --camera-diagnostics
                        --validate-camera-follow
                        --framebuffer-size 960 540
                        --frames 1)
                set_tests_properties(
                    jpb_pc_authored_camera_spawn
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera"
                        PASS_REGULAR_EXPRESSION
                            "camera=\\(dolly=56,flags=0000111a,initial=56,unique=1,transitions=0,authored=1,collision=1\\.0000/0,.*\\) camera_world=\\(22388,3616,-14754\\).*camera_state=\\(type=1,view=0x00001901,angles=140,2048->140,2048")
                add_test(
                    NAME jpb_pc_authored_camera_follow
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --control-harness
                        --headless-phase up 120
                        --headless-phase right 30
                        --headless-phase none 53
                        --validate-camera-follow
                        --framebuffer-size 960 540
                        --frames 203)
                set_tests_properties(
                    jpb_pc_authored_camera_follow
                    PROPERTIES
                        LABELS "real_assets;pc_game;camera"
                        TIMEOUT 120)
                if(JPB_UFBX_SOURCE_DIR AND
                   EXISTS "${JPB_GAME_DATA_DIR}/res/level/jpx/fed/fed.fbx")
                    add_test(
                        NAME jpb_pc_fbx_level_smoke
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_PC_GAME_CAD}"
                            --bmd
                            "${JPB_PC_GAME_BMD}"
                            --headless
                            --require-fbx-level
                            --frames 1)
                    set_tests_properties(
                        jpb_pc_fbx_level_smoke
                        PROPERTIES
                            LABELS "real_assets;pc_game;rendering;fbx"
                            PASS_REGULAR_EXPRESSION
                                "level_fbx=.*batches=[1-9][0-9]* vertices=[1-9][0-9]* triangles=[1-9][0-9]*")
                endif()
                add_test(
                    NAME jpb_pc_title_smoke
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --frames 1
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title.ppm")
                set_tests_properties(
                    jpb_pc_title_smoke
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(0,music=1\\).*text=13/13/0/")
                add_test(
                    NAME jpb_pc_title_character_select
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title-character-select
                        --frames 3
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_character_select.ppm")
                set_tests_properties(
                    jpb_pc_title_character_select
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(14,music=1\\).*text=4/4/0/.*screen=19/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_character_select_keyboard_ignores_xinput
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title-character-select
                        --headless-keyboard-phase none 1
                        --headless-xinput-phase left 24
                        --frames 25)
                set_tests_properties(
                    jpb_pc_title_character_select_keyboard_ignores_xinput
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;controls"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(14,music=1\\).*character=\\(state=24,select=0,players=1,models=0/1")
                add_test(
                    NAME jpb_pc_title_character_select_keyboard_changes_once_with_xinput
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title-character-select
                        --headless-keyboard-phase none 1
                        --headless-xinput-phase left 12
                        --headless-keyboard-phase d 1
                        --headless-keyboard-phase none 12
                        --frames 26)
                set_tests_properties(
                    jpb_pc_title_character_select_keyboard_changes_once_with_xinput
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;controls"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(14,music=1\\).*character=\\(state=24,select=0,players=1,models=1/1")
                add_test(
                    NAME jpb_pc_title_character_select_completed
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title-character-select-completed
                        --frames 3
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_character_select_completed.ppm")
                set_tests_properties(
                    jpb_pc_title_character_select_completed
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;controls"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(14,music=1\\).*text=6/6/0/.*screen=21/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_character_select_two_player
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title-character-select-two-player
                        --frames 3
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_character_select_two_player.ppm")
                set_tests_properties(
                    jpb_pc_title_character_select_two_player
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(14,music=1\\).*text=8/8/0/.*screen=22/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_character_select_two_player_completed
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title-character-select-two-player-completed
                        --frames 3
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_character_select_two_player_completed.ppm")
                set_tests_properties(
                    jpb_pc_title_character_select_two_player_completed
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;controls"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(14,music=1\\).*text=12/12/0/.*screen=26/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_new_game
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --headless-phase select 1
                        --headless-phase none 1
                        --frames 3
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_new_game.ppm")
                set_tests_properties(
                    jpb_pc_title_new_game
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(144,music=1\\).*text=5/5/0/")
                add_test(
                    NAME jpb_pc_title_new_game_flow
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 1
                        --frames 9)
                set_tests_properties(
                    jpb_pc_title_new_game_flow
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(4,music=1\\).*text=4/4/0/")
                add_test(
                    NAME jpb_pc_title_continue_game_flow
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --headless
                        --mute
                        --front-end-flow
                        --title-valid-save
                        --title-main-select 0
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 20
                        --frames 23)
                set_tests_properties(
                    jpb_pc_title_continue_game_flow
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;flow"
                        PASS_REGULAR_EXPRESSION
                            "game_state=\\(level=1,mode=6,players=1,versus=0\\).*frames=23 mode=game-camera")
                add_test(
                    NAME jpb_pc_title_options
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --frames 54
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_options.ppm")
                set_tests_properties(
                    jpb_pc_title_options
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(11,music=1\\).*text=9/9/0/")
                add_test(
                    NAME jpb_pc_title_controls
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --frames 105
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_title_controls.ppm")
                set_tests_properties(
                    jpb_pc_title_controls
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;controls"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(35,music=1\\).*text=20/20/0/.*screen=16/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_audio_options
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase right 1
                        --headless-phase none 1
                        --validate-title-audio
                        --frames 76)
                set_tests_properties(
                    jpb_pc_title_audio_options
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(16,music=0\\)")
                add_test(
                    NAME jpb_pc_title_video_options
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase right 1
                        --headless-phase none 1
                        --frames 60)
                set_tests_properties(
                    jpb_pc_title_video_options
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        # Matched ReadKeyboardInput emits raw 0x2000 for
                        # physical D/right; the original menu modifier treats
                        # that bit as decrement, so the default 0 wraps to 2.
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(149,music=1\\).*video=\\(2,0\\)")
                add_test(
                    NAME jpb_pc_title_movie_resolution
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --mute --title
                        --validate-title-movie 7
                        --frames 1)
                set_tests_properties(
                    jpb_pc_title_movie_resolution
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;movie"
                        PASS_REGULAR_EXPRESSION
                            "movie_state=\\(requests=1,resolved=1,launched=1,decoded=[1-9][0-9]*,presented=[1-9][0-9]*,audio_bytes=[1-9][0-9]*,audio_samples=[1-9][0-9]*,audio_chunks=[1-9][0-9]*,failures=0,last=7,flags=0,path=.*End1080Flipped_converted\\.ogg,error=none\\)")
                add_test(
                    NAME jpb_pc_title_language
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --mute --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 16
                        --headless-phase right 1
                        --headless-phase none 3
                        --frames 141)
                set_tests_properties(
                    jpb_pc_title_language
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;localization"
                        # The same exact physical-right/decrement convention
                        # wraps the seven-value language selector from 0 to 6.
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(145,music=1\\).*language=6.*text=4/4/0/")
                add_test(
                    NAME jpb_pc_title_quit_cancel
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --mute --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 3
                        --frames 90)
                set_tests_properties(
                    jpb_pc_title_quit_cancel
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;flow"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(0,music=1\\) running=1")
                add_test(
                    NAME jpb_pc_title_quit_confirm
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --mute --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 3
                        --frames 107)
                set_tests_properties(
                    jpb_pc_title_quit_confirm
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;flow"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(146,music=1\\) running=0")
                add_test(
                    NAME jpb_pc_title_training
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --frames 20)
                set_tests_properties(
                    jpb_pc_title_training
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(12,music=1\\).*text=8/8/0/.*screen=20/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_level_select
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --title-level-select
                        --frames 3)
                set_tests_properties(
                    jpb_pc_title_level_select
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(26,music=1\\).*text=4/4/0/.*screen=42/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_character_to_level_select
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --title-character-select
                        --headless-phase none 2
                        --headless-phase select 1
                        --headless-phase none 3
                        --frames 7)
                set_tests_properties(
                    jpb_pc_title_character_to_level_select
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation;flow"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(26,music=1\\).*text=4/4/0/.*screen=42/0/[1-9][0-9]* menu_textures=209")
                add_test(
                    NAME jpb_pc_title_single_player_gameplay_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute --front-end-flow
                        --camera-diagnostics
                        --validate-camera-follow
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 3
                        --input-phase select 1
                        --input-phase none 3
                        --input-phase select 1
                        --input-phase none 20
                        --frames 36)
                set_tests_properties(
                    jpb_pc_title_single_player_gameplay_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;controls;flow"
                        PASS_REGULAR_EXPRESSION
                            "game_state=\\(level=1,mode=4,players=1,versus=0\\).*frames=36 mode=game-camera")
                add_test(
                    NAME jpb_pc_title_fed_intro_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless --mute --front-end-flow
                        --camera-diagnostics
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 3
                        --input-phase select 1
                        --input-phase none 3
                        --input-phase select 1
                        --input-phase none 3
                        --frames 19)
                set_tests_properties(
                    jpb_pc_title_fed_intro_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;controls;flow;camera;enemy_ai"
                        PASS_REGULAR_EXPRESSION
                            "camera_director=\\(override=144,current=144,enemy=0,mode=2,node=7,opcode=0x0200.*run_origin=\\(player=22528\\.0/3328\\.0/-13056\\.0/facing:2048\\).*camera=\\(dolly=144,authored=3,collision=1\\.0000/0\\).*player_flags=00000002"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_title_keyboard_gameplay_control_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute --front-end-flow
                        --validate-camera-follow
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 1
                        --input-phase select 1
                        --input-phase none 3
                        --input-phase select 1
                        --input-phase none 3
                        --input-phase select 1
                        --input-phase none 20
                        --input-phase up 10
                        --input-phase none 5
                        --frames 51)
                set_tests_properties(
                    jpb_pc_title_keyboard_gameplay_control_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;controls;flow;keyboard"
                        PASS_REGULAR_EXPRESSION
                            "game_state=\\(level=1,mode=4,players=1,versus=0\\).*control_sources=\\(p1=keyboard,p2=xinput,last=keyboard\\).*control_edges=\\(p1=.*held_frames:10.*locomotion:10/2.*frames=51 mode=game-camera"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_xinput_front_end_gameplay_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute --front-end-flow
                        # Exercise physical XInput A through every menu owner.
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 1
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 1
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 1
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 3
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 3
                        --headless-xinput-phase a 1
                        # Keep the final menu confirmation physically held
                        # across the transition.  The handoff guard must wait
                        # for neutral without publishing A as gameplay input.
                        --headless-xinput-phase a 7
                        # After neutral, scheme 1 maps physical B to north.
                        --headless-xinput-phase none 3
                        --headless-xinput-phase b 1
                        --headless-xinput-phase none 30
                        --frames 57)
                set_tests_properties(
                    jpb_pc_xinput_front_end_gameplay_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;controls;xinput;flow"
                        PASS_REGULAR_EXPRESSION
                            "game_state=\\(level=1,mode=4,players=1,versus=0\\).*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=.*pressed_bits:00000010.*frames=57 mode=game-camera.*damage_motion_frames=[1-9][0-9]* last_damage_motion=106"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_xinput_force_front_end_gameplay_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute --front-end-flow
                        --headless-maximum-progression
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 1
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 1
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 1
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 3
                        --headless-xinput-phase a 1
                        --headless-xinput-phase none 3
                        --headless-xinput-phase a 1
                        # The loading bridge consumes the final menu A and
                        # waits for neutral before this physical Force chord.
                        --headless-xinput-phase none 3
                        --headless-xinput-phase lt+b 1
                        --headless-xinput-phase none 44
                        --frames 64)
                set_tests_properties(
                    jpb_pc_xinput_force_front_end_gameplay_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;controls;xinput;force;flow"
                        PASS_REGULAR_EXPRESSION
                            "game_state=\\(level=1,mode=4,players=1,versus=0\\).*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=.*pressed_bits:00000011.*frames=64 mode=game-camera.*damage_motion_frames=[1-9][0-9]* last_damage_motion=99"
                        TIMEOUT 120)
                set(JPB_BASE_PLAYER_NAMES
                    obi_wan qui_gon mace adi plo maul_p amidala panaka ki_adi)
                set(JPB_BASE_ATTACK_NORTH_MOTIONS
                    106 100 124 133 105 138 107 101 115)
                set(JPB_BASE_ATTACK_NORTH_NAMES
                    jd_hack_h_base qg_hack_h_base jd_360jump_backhand_add
                    ag_hack_h_base pl_hack_h_base dm_backhand_2hit_custom
                    hu_blaster_pistol_base bd_360_backhand_add
                    jd_backhand_360_add)
                set(JPB_BASE_ATTACK_SOUTH_MOTIONS
                    107 124 106 103 108 134 106 112 130)
                set(JPB_BASE_ATTACK_SOUTH_NAMES
                    jd_hack_base qg_hack_base mw_hack_h_base ag_hack_base
                    pl_hack_base dm_stab_base hu_blaster_pistol_single
                    hu_blaster_pistol_single jd_stab_r_base)
                set(JPB_BASE_ATTACK_WEST_MOTIONS
                    105 99 126 102 107 132 99 99 109)
                set(JPB_BASE_ATTACK_WEST_NAMES
                    jd_slash_r_base qg_slash_r_base mw_slash_add
                    ag_slash_r_base pl_slash_l_base dm_slash_l_base
                    bd_jab_base bd_jab_base km_slash_r_base)
                set(JPB_BASE_RUNNING_ATTACK_NORTH_NAMES
                    qg_slash_run qg_slash_run qg_slash_run ag_hack_run
                    qg_slash_run qg_slash_run deadbeef deadbeef
                    qg_slash_run)
                set(JPB_BASE_RUNNING_ATTACK_SOUTH_NAMES
                    jd_stab_run qg_stab_run jd_stab_run jd_stab_run
                    jd_stab_run jd_stab_run deadbeef deadbeef
                    jd_stab_run)
                set(JPB_BASE_RUNNING_ATTACK_WEST_NAMES
                    jd_slash_run jd_slash_run jd_slash_run ag_slash_run
                    jd_slash_run dm_slash_run deadbeef deadbeef
                    jd_slash_run)
                set(JPB_BASE_LOCK_FORWARD_NAMES
                    jd_shuffle_f_core qg_shuffle_f_core mw_shuffle_f_core
                    ag_shuffle_f_core pl_shuffle_f_core jd_shuffle_f_core
                    hu_shuffle_f_pistol hu_shuffle_f_pistol
                    km_shuffle_f_core)
                set(JPB_BASE_LOCK_BACK_NAMES
                    jd_hop_b_core qg_hop_b_core qg_hop_b_core
                    ag_hop_b_core jd_hop_b_core jd_hop_b_core
                    jd_hop_b_core jd_hop_b_core km_hop_b_core)
                set(JPB_BASE_BLOCK_NAMES
                    jd_block_active_core qg_block_active_core
                    mw_block_active_core ag_block_active_core
                    pl_block_active_core dm_block_active_b_core
                    hu_block_active_unarmed hu_block_active_unarmed
                    jd_block_active_core)
                set(JPB_BASE_FORCE_NORTH_MOTIONS
                    99 67 96 101 100 67 65 65 101)
                set(JPB_BASE_FORCE_NORTH_NAMES
                    jd_charge_fwd_force jd_push_b_force mw_shield_a_force
                    ag_mesmerize_force pl_spin_a_force jd_push_b_force
                    jd_throw_item_custom jd_throw_item_custom deadbeef)
                set(JPB_BASE_FORCE_JUMP_MOTIONS
                    134 96 99 117 129 71 65 65 117)
                set(JPB_BASE_FORCE_JUMP_NAMES
                    jd_meditate_a_force jd_heal_a_force mw_absorb_a_force
                    ag_defend_a_force jd_meditate_a_force jd_defend_a_force
                    jd_throw_item_custom jd_throw_item_custom
                    jd_uppercut_add)
                set(JPB_BASE_FORCE_SOUTH_MOTIONS
                    66 66 102 67 99 125 65 65 141)
                set(JPB_BASE_FORCE_SOUTH_NAMES
                    jd_push_a_force jd_push_a_force jd_sabertoss_a_force
                    jd_push_b_force jd_attack_force jd_attack_force
                    jd_throw_item_custom jd_throw_item_custom
                    jd_sabertoss_a_force)
                set(JPB_BASE_FORCE_ITEM_MOTIONS
                    65 65 65 74 129 65 65 65 65)
                set(JPB_BASE_FORCE_ITEM_NAMES
                    jd_throw_item_custom jd_throw_item_custom
                    jd_throw_item_custom jd_meditate_a_force
                    jd_meditate_a_force jd_throw_item_custom
                    jd_throw_item_custom jd_throw_item_custom
                    jd_throw_item_custom)
                set(JPB_BASE_JUMP_NAMES
                    jd_jump_b_core jd_jump_b_core jd_jump_b_core
                    ag_jump_b_core jd_jump_b_core jd_jump_b_core
                    hu_jump_b_core hu_jump_b_core jd_jump_b_core)
                set(JPB_BASE_IDLE_NAMES
                    jd_idle_a_core qg_idle_a_core mw_idle_a_core
                    ag_idle_a_core pl_idle_a_core dm_idle_b_core
                    hu_idle_pistol_core hu_idle_pistol_core km_idle_a_core)
                # Unlike the headless menu regressions, this drives the real
                # Win32 window and D3D11 hardware presentation path through
                # title, character select, level select, and gameplay handoff.
                # The diagnostic window remains hidden. Audio banks/hooks are
                # rebuilt with output disabled, and persistence is redirected
                # to a build-tree directory so the desktop, speakers, and
                # installed saves remain untouched.
                add_test(
                    NAME jpb_pc_presentation_gameplay_handoff
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --player-model 0
                        --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                        --hidden-window --control-harness
                        --validate-presentation-handoff
                        --validate-neutral-handoff
                        --silent-audio --validate-audio-handoff
                        --persistence-directory
                            "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_presentation_persistence"
                        --validate-persistence-handoff
                        --front-end-flow
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase select 1
                        --headless-phase none 3
                        --headless-phase select 1
                        --headless-phase none 3
                        --headless-phase select 1
                        --headless-phase none 20
                        --frames 36)
                set_tests_properties(
                    jpb_pc_presentation_gameplay_handoff
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;controls;flow;handoff;presentation"
                        RUN_SERIAL TRUE
                        TIMEOUT 60
                        PASS_REGULAR_EXPRESSION
                            "presentation=\\(frames=36,handoffs=1,hidden=1,scripted=1\\)")
                foreach(JPB_BASE_PLAYER_ID RANGE 0 8)
                    list(GET JPB_BASE_PLAYER_NAMES
                        ${JPB_BASE_PLAYER_ID} JPB_BASE_PLAYER_NAME)
                    add_test(
                        NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_gameplay_handoff
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                            --player-model ${JPB_BASE_PLAYER_ID}
                            --validate-player-saber
                            --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                            --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                            --headless --mute --front-end-flow
                            --headless-phase select 1
                            --headless-phase none 1
                            --headless-phase select 1
                            --headless-phase none 1
                            --headless-phase select 1
                            --headless-phase none 1
                            --headless-phase select 1
                            --headless-phase none 3
                            --headless-phase select 1
                            --headless-phase none 3
                            --headless-phase select 1
                            --headless-phase none 20
                            --frames 36)
                    set_tests_properties(
                        jpb_pc_${JPB_BASE_PLAYER_NAME}_gameplay_handoff
                        PROPERTIES
                            LABELS "real_assets;pc_game;menu;controls;flow;handoff"
                            PASS_REGULAR_EXPRESSION
                                "player_selection=\\(models=${JPB_BASE_PLAYER_ID}/1\\).*frames=36 mode=game-camera")
                    foreach(JPB_BASE_ATTACK_DIRECTION IN ITEMS north south west)
                        if(JPB_BASE_ATTACK_DIRECTION STREQUAL "north")
                            list(GET JPB_BASE_ATTACK_NORTH_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_BASE_ATTACK_MOTION)
                            list(GET JPB_BASE_ATTACK_NORTH_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_BASE_ATTACK_NAME)
                        elseif(JPB_BASE_ATTACK_DIRECTION STREQUAL "south")
                            list(GET JPB_BASE_ATTACK_SOUTH_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_BASE_ATTACK_MOTION)
                            list(GET JPB_BASE_ATTACK_SOUTH_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_BASE_ATTACK_NAME)
                        else()
                            list(GET JPB_BASE_ATTACK_WEST_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_BASE_ATTACK_MOTION)
                            list(GET JPB_BASE_ATTACK_WEST_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_BASE_ATTACK_NAME)
                        endif()
                        set(JPB_BASE_ATTACK_PROJECTILE_ARGS)
                        set(JPB_BASE_ATTACK_PASS_REGEX
                            "phase_end=0.*motion=${JPB_BASE_ATTACK_MOTION}\\[${JPB_BASE_ATTACK_NAME}\\]")
                        # The installed CADs mark only the actual pistol
                        # motions with motionFlags bit 20 / callback slot 1.
                        # Amidala's west and Panaka's north/west actions are
                        # authored melee strikes and must not emit a shot.
                        if((JPB_BASE_PLAYER_ID EQUAL 6 AND
                            (JPB_BASE_ATTACK_DIRECTION STREQUAL "north" OR
                             JPB_BASE_ATTACK_DIRECTION STREQUAL "south")) OR
                           (JPB_BASE_PLAYER_ID EQUAL 7 AND
                            JPB_BASE_ATTACK_DIRECTION STREQUAL "south"))
                            list(APPEND JPB_BASE_ATTACK_PROJECTILE_ARGS
                                --validate-player-projectile)
                            if(JPB_BASE_PLAYER_ID EQUAL 6 AND
                               JPB_BASE_ATTACK_DIRECTION STREQUAL "north")
                                set(JPB_BASE_ATTACK_PROJECTILE_COUNT 3)
                            else()
                                set(JPB_BASE_ATTACK_PROJECTILE_COUNT 1)
                            endif()
                            set(JPB_BASE_ATTACK_PASS_REGEX
                                "phase_end=0.*motion=${JPB_BASE_ATTACK_MOTION}\\[${JPB_BASE_ATTACK_NAME}\\].*player_projectiles=\\(p1=${JPB_BASE_ATTACK_PROJECTILE_COUNT}/type:18")
                        elseif(JPB_BASE_PLAYER_ID EQUAL 6 OR
                               JPB_BASE_PLAYER_ID EQUAL 7)
                            set(JPB_BASE_ATTACK_PASS_REGEX
                                "phase_end=0.*motion=${JPB_BASE_ATTACK_MOTION}\\[${JPB_BASE_ATTACK_NAME}\\].*player_projectiles=\\(p1=0/type:0")
                        endif()
                        add_test(
                            NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_attack_${JPB_BASE_ATTACK_DIRECTION}
                            COMMAND
                                jpb_pc_game
                                "${JPB_PC_GAME_MESH}"
                                --cad
                                "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                                --bmd
                                "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                                --cmb
                                "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                                --player-model ${JPB_BASE_PLAYER_ID}
                                ${JPB_BASE_ATTACK_PROJECTILE_ARGS}
                                --validate-player-saber
                                --headless --mute
                                --headless-phase-p1 attack-${JPB_BASE_ATTACK_DIRECTION} 1
                                --headless-phase-p1 none 44
                                --frames 45)
                        set_tests_properties(
                            jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_attack_${JPB_BASE_ATTACK_DIRECTION}
                            PROPERTIES
                                LABELS "real_assets;pc_game;combat;controls;saber;roster"
                                PASS_REGULAR_EXPRESSION
                                    "${JPB_BASE_ATTACK_PASS_REGEX}")
                    endforeach()
                    foreach(JPB_RUNNING_ATTACK_DIRECTION IN ITEMS north south west)
                        if(JPB_RUNNING_ATTACK_DIRECTION STREQUAL "north")
                            set(JPB_RUNNING_ATTACK_MOTION 94)
                            list(GET JPB_BASE_RUNNING_ATTACK_NORTH_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_RUNNING_ATTACK_NAME)
                        elseif(JPB_RUNNING_ATTACK_DIRECTION STREQUAL "south")
                            set(JPB_RUNNING_ATTACK_MOTION 92)
                            list(GET JPB_BASE_RUNNING_ATTACK_SOUTH_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_RUNNING_ATTACK_NAME)
                        else()
                            set(JPB_RUNNING_ATTACK_MOTION 93)
                            list(GET JPB_BASE_RUNNING_ATTACK_WEST_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_RUNNING_ATTACK_NAME)
                        endif()
                        add_test(
                            NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_running_attack_${JPB_RUNNING_ATTACK_DIRECTION}
                            COMMAND
                                jpb_pc_game
                                "${JPB_PC_GAME_MESH}"
                                --cad
                                "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                                --bmd
                                "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                                --cmb
                                "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                                --player-model ${JPB_BASE_PLAYER_ID}
                                --headless --mute
                                --headless-phase-p1 up 21
                                --headless-phase-p1 up+attack-${JPB_RUNNING_ATTACK_DIRECTION} 1
                                --headless-phase-p1 none 30
                                --frames 53)
                        set_tests_properties(
                            jpb_pc_${JPB_BASE_PLAYER_NAME}_running_attack_${JPB_RUNNING_ATTACK_DIRECTION}
                            PROPERTIES
                                LABELS "real_assets;pc_game;combat;controls;context;roster"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=1.*motion=${JPB_RUNNING_ATTACK_MOTION}\\[${JPB_RUNNING_ATTACK_NAME}\\]"
                                    TIMEOUT 120)
                    endforeach()
                    list(GET JPB_BASE_BLOCK_NAMES
                        ${JPB_BASE_PLAYER_ID} JPB_BASE_BLOCK_NAME)
                    add_test(
                        NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_block
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                            --player-model ${JPB_BASE_PLAYER_ID}
                            --headless --mute
                            --headless-phase-p1 block 1
                            --headless-phase-p1 none 1
                            --frames 3)
                    set_tests_properties(
                        jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_block
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;block;roster"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*bits=00000004/00000000.*motion=15\\[${JPB_BASE_BLOCK_NAME}\\]")
                    foreach(JPB_ROSTER_FORCE_ACTION IN ITEMS north jump south item)
                        if(JPB_ROSTER_FORCE_ACTION STREQUAL "north")
                            set(JPB_ROSTER_FORCE_BUTTONS force+attack-north)
                            set(JPB_ROSTER_FORCE_BITS 00000011)
                            list(GET JPB_BASE_FORCE_NORTH_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_MOTION)
                            list(GET JPB_BASE_FORCE_NORTH_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_NAME)
                        elseif(JPB_ROSTER_FORCE_ACTION STREQUAL "jump")
                            set(JPB_ROSTER_FORCE_BUTTONS force+jump)
                            set(JPB_ROSTER_FORCE_BITS 00000021)
                            list(GET JPB_BASE_FORCE_JUMP_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_MOTION)
                            list(GET JPB_BASE_FORCE_JUMP_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_NAME)
                        elseif(JPB_ROSTER_FORCE_ACTION STREQUAL "south")
                            set(JPB_ROSTER_FORCE_BUTTONS force+attack-south)
                            set(JPB_ROSTER_FORCE_BITS 00000041)
                            list(GET JPB_BASE_FORCE_SOUTH_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_MOTION)
                            list(GET JPB_BASE_FORCE_SOUTH_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_NAME)
                        else()
                            set(JPB_ROSTER_FORCE_BUTTONS force+attack-west)
                            set(JPB_ROSTER_FORCE_BITS 00000081)
                            list(GET JPB_BASE_FORCE_ITEM_MOTIONS
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_MOTION)
                            list(GET JPB_BASE_FORCE_ITEM_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_NAME)
                        endif()

                        # Every authored ForceMap slot is launch-checked above.
                        # The exceptional owners below also prove the exact
                        # PDB callback/resource state on the following control
                        # tick.  Mace's absorb chain and Adi's shorter-lived
                        # mesmerize callback are followed through completion.
                        set(JPB_ROSTER_FORCE_TAIL_COMMAND
                            --headless-phase-p1 none 1)
                        set(JPB_ROSTER_FORCE_FRAMES 3)
                        set(JPB_ROSTER_FORCE_REGEX
                            "phase_end=0.*bits=${JPB_ROSTER_FORCE_BITS}/00000000.*motion=${JPB_ROSTER_FORCE_MOTION}\\[${JPB_ROSTER_FORCE_NAME}\\]")
                        if(JPB_BASE_PLAYER_ID EQUAL 2 AND
                           JPB_ROSTER_FORCE_ACTION STREQUAL "jump")
                            list(GET JPB_BASE_IDLE_NAMES
                                ${JPB_BASE_PLAYER_ID} JPB_ROSTER_FORCE_IDLE_NAME)
                            set(JPB_ROSTER_FORCE_TAIL_COMMAND
                                --headless-phase-p1 none 40
                                --headless-phase-p1 none 20
                                --headless-phase-p1 none 40)
                            set(JPB_ROSTER_FORCE_FRAMES 102)
                            set(JPB_ROSTER_FORCE_REGEX
                                "phase_end=0.*bits=00000021/00000000.*motion=99\\[mw_absorb_a_force\\].*queue=100\\[mw_absorb_b_force\\]>101\\[mw_absorb_c_force\\].*force_data=0/0/50/0/0/0.*phase_end=1.*motion=100\\[mw_absorb_b_force\\].*queue=101\\[mw_absorb_c_force\\].*phase_end=2.*motion=101\\[mw_absorb_c_force\\].*queue=<empty>.*phase_end=3.*motion=0\\[${JPB_ROSTER_FORCE_IDLE_NAME}\\].*queue=<empty>.*lock=0")
                        elseif(JPB_BASE_PLAYER_ID EQUAL 3 AND
                               JPB_ROSTER_FORCE_ACTION STREQUAL "north")
                            set(JPB_ROSTER_FORCE_TAIL_COMMAND
                                --headless-phase-p1 none 300
                                --headless-phase-p1 none 1)
                            set(JPB_ROSTER_FORCE_FRAMES 303)
                            set(JPB_ROSTER_FORCE_REGEX
                                "phase_end=0.*bits=00000011/00000000.*motion=${JPB_ROSTER_FORCE_MOTION}\\[${JPB_ROSTER_FORCE_NAME}\\].*force_owner=force_MesmerizeCallBack,resources=100/100/68/100/1,force_data=[^,]*/0/0/0.*phase_end=1.*force_owner=force_MesmerizeCallBack,resources=100/100/68/100/1,force_data=[^,]*/300/6/0.*phase_end=2.*callbacks=1/0/0,force_owner=none,resources=100/100/68/100/1,force_data=[^,]*/301/7/0")
                        elseif(JPB_BASE_PLAYER_ID EQUAL 8 AND
                               JPB_ROSTER_FORCE_ACTION STREQUAL "north")
                            set(JPB_ROSTER_FORCE_REGEX
                                "phase_end=0.*bits=00000011/00000000.*motion=${JPB_ROSTER_FORCE_MOTION}\\[${JPB_ROSTER_FORCE_NAME}\\].*force_owner=force_MesmerizeCallBack,resources=100/100/68/100/1,force_data=[^,]*/0/0/0.*phase_end=1.*force_owner=force_MesmerizeCallBack,resources=100/100/68/100/1,force_data=[^,]*/1/1/0")
                        elseif((JPB_BASE_PLAYER_ID EQUAL 3 OR
                                JPB_BASE_PLAYER_ID EQUAL 8) AND
                               JPB_ROSTER_FORCE_ACTION STREQUAL "jump")
                            set(JPB_ROSTER_FORCE_REGEX
                                "phase_end=0.*bits=00000021/00000000.*motion=${JPB_ROSTER_FORCE_MOTION}\\[${JPB_ROSTER_FORCE_NAME}\\].*force_owner=force_CloakCallBack,resources=100/100/100/100/1,force_data=0/0/0/0/0/0.*phase_end=1.*force_owner=force_CloakCallBack,resources=100/100/80/100/1,force_data=1/599/0/0/0/0")
                        elseif(JPB_BASE_PLAYER_ID EQUAL 3 AND
                               JPB_ROSTER_FORCE_ACTION STREQUAL "item")
                            set(JPB_ROSTER_FORCE_REGEX
                                "phase_end=0.*bits=00000081/00000000.*motion=${JPB_ROSTER_FORCE_MOTION}\\[${JPB_ROSTER_FORCE_NAME}\\].*force_owner=force_ShieldCallBack,resources=100/100/100/100/1,force_data=0/0/0/0/0/0.*phase_end=1.*force_owner=force_ShieldCallBack,resources=100/100/100/100/0,force_data=1/599/0/0/0/0")
                        elseif(JPB_BASE_PLAYER_ID EQUAL 4 AND
                               JPB_ROSTER_FORCE_ACTION STREQUAL "item")
                            set(JPB_ROSTER_FORCE_REGEX
                                "phase_end=0.*bits=00000081/00000000.*motion=${JPB_ROSTER_FORCE_MOTION}\\[${JPB_ROSTER_FORCE_NAME}\\].*force_owner=force_StarCallBack,resources=100/100/100/100/1,force_data=0/0/0/0/0/0.*phase_end=1.*force_owner=force_StarCallBack,resources=100/100/100/100/0,force_data=1/599/0/0/0/0")
                        endif()
                        add_test(
                            NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_force_${JPB_ROSTER_FORCE_ACTION}
                            COMMAND
                                jpb_pc_game
                                "${JPB_PC_GAME_MESH}"
                                --cad
                                "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                                --bmd
                                "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                                --cmb
                                "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                                --player-model ${JPB_BASE_PLAYER_ID}
                                --validate-player-saber
                                --headless --mute
                                --headless-maximum-progression
                                --headless-phase-p1 ${JPB_ROSTER_FORCE_BUTTONS} 1
                                ${JPB_ROSTER_FORCE_TAIL_COMMAND}
                                --frames ${JPB_ROSTER_FORCE_FRAMES})
                        set_tests_properties(
                            jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_force_${JPB_ROSTER_FORCE_ACTION}
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;force;roster"
                                PASS_REGULAR_EXPRESSION
                                    "${JPB_ROSTER_FORCE_REGEX}"
                                TIMEOUT 120)
                    endforeach()
                    list(GET JPB_BASE_JUMP_NAMES
                        ${JPB_BASE_PLAYER_ID} JPB_BASE_JUMP_NAME)
                    list(GET JPB_BASE_IDLE_NAMES
                        ${JPB_BASE_PLAYER_ID} JPB_BASE_IDLE_NAME)
                    foreach(JPB_ROSTER_JUMP_KIND IN ITEMS standing running)
                        if(JPB_ROSTER_JUMP_KIND STREQUAL "standing")
                            set(JPB_ROSTER_JUMP_COMMAND
                                --headless-phase-p1 jump 1
                                --headless-phase-p1 none 1)
                            set(JPB_ROSTER_JUMP_PHASE 0)
                            set(JPB_ROSTER_JUMP_BITS 00000020)
                        else()
                            set(JPB_ROSTER_JUMP_COMMAND
                                --headless-phase-p1 up 1
                                --headless-phase-p1 up+jump 1
                                --headless-phase-p1 none 1)
                            set(JPB_ROSTER_JUMP_PHASE 1)
                            set(JPB_ROSTER_JUMP_BITS 00001020)
                        endif()
                        add_test(
                            NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_jump_${JPB_ROSTER_JUMP_KIND}
                            COMMAND
                                jpb_pc_game
                                "${JPB_PC_GAME_MESH}"
                                --cad
                                "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                                --bmd
                                "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                                --cmb
                                "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                                --player-model ${JPB_BASE_PLAYER_ID}
                                --headless --mute
                                ${JPB_ROSTER_JUMP_COMMAND}
                                --validate-jump
                                --frames 4)
                        set_tests_properties(
                            jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_jump_${JPB_ROSTER_JUMP_KIND}
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;movement;jump;context;roster"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=${JPB_ROSTER_JUMP_PHASE}.*bits=${JPB_ROSTER_JUMP_BITS}/00000000.*motion=4\\[${JPB_BASE_JUMP_NAME}\\]")
                    endforeach()
                    if(EXISTS "${JPB_PC_ARENA_MESH}")
                        # A launch-only assertion missed a decoded-owner bug in
                        # WorldBlocking: model IDs 2-8 entered Motion 4 but
                        # looked for their landing nodes in the wrong collision
                        # row. Every roster member must complete the authored
                        # trajectory callback and return to its own idle.
                        add_test(
                            NAME jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_jump_recovery
                            COMMAND
                                jpb_pc_game
                                "${JPB_PC_ARENA_MESH}"
                                --cad
                                "${JPB_GAME_DATA_DIR}/res/animation/${JPB_BASE_PLAYER_NAME}.cad"
                                --bmd
                                "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_BASE_PLAYER_NAME}.bmd"
                                --cmb
                                "${JPB_GAME_DATA_DIR}/res/combo/${JPB_BASE_PLAYER_NAME}.cmb"
                                --player-model ${JPB_BASE_PLAYER_ID}
                                --headless --mute
                                --headless-phase-p1 jump 1
                                --headless-phase-p1 none 100
                                --frames 102)
                        set_tests_properties(
                            jpb_pc_${JPB_BASE_PLAYER_NAME}_roster_jump_recovery
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;movement;jump;recovery;roster;provenance"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=0.*motion=4\\[${JPB_BASE_JUMP_NAME}\\].*state=\\(flags=00000001,callbacks=1/1/0.*phase_end=1.*motion=0\\[${JPB_BASE_IDLE_NAME}\\].*lock=0.*state=\\(flags=00000000,callbacks=1/0/0"
                                TIMEOUT 120)
                    endif()
                endforeach()
                if(EXISTS "${JPB_PC_ARENA_MESH}")
                    # The matched executable keeps currentMotion at +0x1b4,
                    # clears ACTION_LOCK at +0x1bc, and owns jump trajectories
                    # through pMotionCallBack at +0x220.  Exercise all three
                    # facts with decoded retail animation/model/combo assets.
                    add_test(
                        NAME jpb_pc_obi_wan_double_jump
                        COMMAND
                            jpb_pc_game "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_GAME_DATA_DIR}/res/animation/obi_wan.cad"
                            --bmd "${JPB_GAME_DATA_DIR}/res/MODEL/obi_wan.bmd"
                            --cmb "${JPB_GAME_DATA_DIR}/res/combo/obi_wan.cmb"
                            --player-model 0
                            --headless --mute
                            --headless-phase-p1 jump 1
                            --headless-phase-p1 none 15
                            --headless-phase-p1 jump 1
                            --headless-phase-p1 none 15
                            --frames 33)
                    set_tests_properties(
                        jpb_pc_obi_wan_double_jump
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;movement;jump;double_jump;provenance"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=2.*bits=00000020/00000000.*motion=22\\[jd_jump_double_core\\].*callbacks=1/1/0.*control_edges=\\(p1=pressed_frames:2.*motion:22"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_obi_wan_directional_double_jump
                        COMMAND
                            jpb_pc_game "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_GAME_DATA_DIR}/res/animation/obi_wan.cad"
                            --bmd "${JPB_GAME_DATA_DIR}/res/MODEL/obi_wan.bmd"
                            --cmb "${JPB_GAME_DATA_DIR}/res/combo/obi_wan.cmb"
                            --player-model 0
                            --headless --mute
                            --headless-phase-p1 jump 1
                            --headless-phase-p1 none 15
                            --headless-phase-p1 up+jump 1
                            --headless-phase-p1 up 15
                            --frames 33)
                    set_tests_properties(
                        jpb_pc_obi_wan_directional_double_jump
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;movement;jump;double_jump;air_steering;provenance"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=2.*bits=00001020/00000000.*player=\\([^)]*facing=4080\\).*motion=22\\[jd_jump_double_core\\].*callbacks=1/1/0.*control_edges=\\(p1=.*motion:22.*direction:[1-9][0-9]*"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_obi_wan_air_steering
                        COMMAND
                            jpb_pc_game "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_GAME_DATA_DIR}/res/animation/obi_wan.cad"
                            --bmd "${JPB_GAME_DATA_DIR}/res/MODEL/obi_wan.bmd"
                            --cmb "${JPB_GAME_DATA_DIR}/res/combo/obi_wan.cmb"
                            --player-model 0
                            --headless --mute
                            --headless-phase-p1 jump 1
                            --headless-phase-p1 none 15
                            --headless-phase-p1 up 10
                            --headless-phase-p1 none 1
                            --frames 28)
                    set_tests_properties(
                        jpb_pc_obi_wan_air_steering
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;movement;jump;air_steering;provenance"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=2.*bits=00001000/00000000.*player=\\([^)]*facing=4080\\).*motion=4\\[jd_jump_b_core\\].*callbacks=1/1/0.*control_edges=\\(p1=.*motion:4.*direction:10"
                            TIMEOUT 120)
                endif()
                if(EXISTS "${JPB_PC_PALACE_MESH}")
                    add_test(
                        NAME jpb_pc_palace_mace_jump_recovery_attack
                        COMMAND
                            jpb_pc_game "${JPB_PC_PALACE_MESH}"
                            --cad "${JPB_GAME_DATA_DIR}/res/animation/mace.cad"
                            --bmd "${JPB_GAME_DATA_DIR}/res/MODEL/mace.bmd"
                            --cmb "${JPB_GAME_DATA_DIR}/res/combo/mace.cmb"
                            --player-model 2
                            --headless --mute
                            --headless-phase-p1 jump 1
                            --headless-phase-p1 none 100
                            --headless-phase-p1 attack-north 1
                            --headless-phase-p1 none 45
                            --frames 148)
                    set_tests_properties(
                        jpb_pc_palace_mace_jump_recovery_attack
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;movement;jump;recovery;combat;sequence;palace;provenance"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*motion=4\\[jd_jump_b_core\\].*callbacks=1/1/0.*phase_end=1.*motion=0\\[mw_idle_a_core\\].*callbacks=1/0/0.*phase_end=2.*motion=124\\[jd_360jump_backhand_add\\]"
                            TIMEOUT 120)
                endif()
                if(EXISTS "${JPB_PC_CORUS1_MESH}")
                    add_test(
                        NAME jpb_pc_corus1_amidala_jump_recovery_attack
                        COMMAND
                            jpb_pc_game "${JPB_PC_CORUS1_MESH}"
                            --cad "${JPB_GAME_DATA_DIR}/res/animation/amidala.cad"
                            --bmd "${JPB_GAME_DATA_DIR}/res/MODEL/amidala.bmd"
                            --cmb "${JPB_GAME_DATA_DIR}/res/combo/amidala.cmb"
                            --player-model 6
                            --headless --mute
                            --headless-phase-p1 jump 1
                            --headless-phase-p1 none 100
                            --headless-phase-p1 attack-west 1
                            --headless-phase-p1 none 45
                            --frames 148)
                    set_tests_properties(
                        jpb_pc_corus1_amidala_jump_recovery_attack
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;movement;jump;recovery;combat;sequence;corus1;provenance"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*motion=4\\[hu_jump_b_core\\].*callbacks=1/1/0.*phase_end=1.*motion=0\\[hu_idle_pistol_core\\].*callbacks=1/0/0.*phase_end=2.*motion=99\\[bd_jab_base\\]"
                            TIMEOUT 120)
                endif()
                foreach(JPB_SABER_TOSS_PLAYER_ID IN ITEMS 2 8)
                    list(GET JPB_BASE_PLAYER_NAMES
                        ${JPB_SABER_TOSS_PLAYER_ID}
                        JPB_SABER_TOSS_PLAYER_NAME)
                    list(GET JPB_BASE_FORCE_SOUTH_MOTIONS
                        ${JPB_SABER_TOSS_PLAYER_ID}
                        JPB_SABER_TOSS_MOTION)
                    list(GET JPB_BASE_FORCE_SOUTH_NAMES
                        ${JPB_SABER_TOSS_PLAYER_ID}
                        JPB_SABER_TOSS_NAME)
                    add_test(
                        NAME jpb_pc_${JPB_SABER_TOSS_PLAYER_NAME}_saber_toss_action
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_SABER_TOSS_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_SABER_TOSS_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_SABER_TOSS_PLAYER_NAME}.cmb"
                            --player-model ${JPB_SABER_TOSS_PLAYER_ID}
                            --validate-player-saber
                            --headless --mute
                            --headless-maximum-progression
                            --headless-phase-p1 force+attack-south 1
                            --headless-phase-p1 none 59
                            --frames 60)
                    set_tests_properties(
                        jpb_pc_${JPB_SABER_TOSS_PLAYER_NAME}_saber_toss_action
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls;force;saber;roster"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*motion=${JPB_SABER_TOSS_MOTION}\\[${JPB_SABER_TOSS_NAME}\\].*player_weapon_action=\\(model=${JPB_SABER_TOSS_PLAYER_ID},saber=1,frames=[1-9][0-9]*,valid=[1-9][0-9]*,invalid=0"
                            TIMEOUT 120)
                endforeach()
                if(EXISTS "${JPB_PC_ARENA_MESH}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_CAD}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_BMD}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_CMB}")
                    add_test(
                        NAME jpb_pc_title_versus_flow
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --headless --mute --front-end-flow
                            --headless-phase-p1 down 1
                            --headless-phase none 16
                            --headless-phase-p1 down 1
                            --headless-phase none 16
                            --headless-phase-p1 select 1
                            --headless-phase none 16
                            --headless-phase-p1 select 1
                            --headless-phase none 3
                            --headless-phase-p1 select 1
                            --headless-phase none 3
                            --headless-phase-p2 select 1
                            --headless-phase none 3
                            --headless-phase-p1 select 1
                            --headless-phase none 10
                            --frames 76)
                    set_tests_properties(
                        jpb_pc_title_versus_flow
                        PROPERTIES
                            LABELS "real_assets;pc_game;menu;presentation;flow;versus"
                            PASS_REGULAR_EXPRESSION
                                "game_state=\\(level=25,mode=2,players=2,versus=1\\).*second_player=\\(ready=1,triangles=[1-9][0-9]*,pixels=[1-9][0-9]*\\)")
                    add_test(
                        NAME jpb_pc_presentation_two_player_audio_handoff
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --hidden-window --control-harness
                            --silent-audio
                            --validate-presentation-handoff
                            --validate-neutral-handoff
                            --validate-audio-handoff
                            --validate-player-two-sound vjdie
                            --front-end-flow
                            --input-phase-p1 down 1
                            --input-phase none 16
                            --input-phase-p1 down 1
                            --input-phase none 16
                            --input-phase-p1 select 1
                            --input-phase none 16
                            --input-phase-p1 select 1
                            --input-phase none 3
                            --input-phase-p1 select 1
                            --input-phase none 18
                            --frames 76)
                    set_tests_properties(
                        jpb_pc_presentation_two_player_audio_handoff
                        PROPERTIES
                            LABELS "real_assets;pc_game;menu;controls;flow;handoff;presentation;audio;two_player;versus"
                            RUN_SERIAL TRUE
                            TIMEOUT 120
                            PASS_REGULAR_EXPRESSION
                                "player_two_audio=\\(requested=vjdie,resolved=1,path=.*qui_gon.vjdie\\.wav\\)")
                    add_test(
                        NAME jpb_pc_xinput_two_player_versus_handoff
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --headless --mute --front-end-flow
                            --headless-xinput-phase-pair down none 1
                            --headless-xinput-phase-pair none none 16
                            --headless-xinput-phase-pair down none 1
                            --headless-xinput-phase-pair none none 16
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 16
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 3
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 3
                            # The recovered versus owner transitions after P1
                            # accepts the pair; do not leak menu A into play.
                            --headless-xinput-phase-pair none none 25
                            # Both physical controllers retain ownership and
                            # scheme-1 B once their actors are idle.
                            --headless-xinput-phase-pair b b 1
                            --headless-xinput-phase-pair none none 30
                            --frames 110)
                    set_tests_properties(
                        jpb_pc_xinput_two_player_versus_handoff
                        PROPERTIES
                            LABELS "real_assets;pc_game;menu;controls;xinput;flow;versus;two_player"
                            PASS_REGULAR_EXPRESSION
                                "game_state=\\(level=25,mode=2,players=2,versus=1\\).*second_player=\\(ready=1,triangles=[1-9][0-9]*,pixels=[1-9][0-9]*\\).*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=.*pressed_bits:00000010.*p2=.*pressed_bits:00000010.*frames=110 mode=game-camera"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_xinput_two_player_force_handoff
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute --front-end-flow
                            --headless-maximum-progression
                            --headless-xinput-phase-pair down none 1
                            --headless-xinput-phase-pair none none 16
                            --headless-xinput-phase-pair down none 1
                            --headless-xinput-phase-pair none none 16
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 16
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 3
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 3
                            --headless-xinput-phase-pair none none 26
                            # P2's physical LT+B must remain P2-owned while
                            # gameplay applies the executable's shared scheme.
                            --headless-xinput-phase-pair none lt+b 1
                            --headless-xinput-phase-pair none none 44
                            --frames 124)
                    set_tests_properties(
                        jpb_pc_xinput_two_player_force_handoff
                        PROPERTIES
                            LABELS "real_assets;pc_game;menu;controls;xinput;force;flow;versus;two_player"
                            PASS_REGULAR_EXPRESSION
                                "game_state=\\(level=25,mode=2,players=2,versus=1\\).*second_player=\\(ready=1,triangles=[1-9][0-9]*,pixels=[1-9][0-9]*\\).*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=pressed_frames:0,held_frames:0,release_events:0,pressed_bits:00000000.*p2=.*pressed_bits:00000011.*motion:67.*frames=124 mode=game-camera"
                            TIMEOUT 120)
                endif()
                add_test(
                    NAME jpb_pc_title_credits
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 3
                        --frames 126)
                set_tests_properties(
                    jpb_pc_title_credits
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(19,music=1\\).*text=2/2/0/")
                add_test(
                    NAME jpb_pc_title_concept_art
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --headless --title
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase down 1
                        --headless-phase none 16
                        --headless-phase select 1
                        --headless-phase none 1
                        --frames 141)
                set_tests_properties(
                    jpb_pc_title_concept_art
                    PROPERTIES
                        LABELS "real_assets;pc_game;menu;presentation"
                        PASS_REGULAR_EXPRESSION
                            "title_menu=\\(27,music=1\\).*text=2/2/0/.*screen=3/")
                if(EXISTS "${JPB_PC_THEED_MESH}")
                    add_test(
                        NAME jpb_pc_theed_water
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_THEED_MESH}"
                            --cad
                            "${JPB_PC_GAME_CAD}"
                            --bmd
                            "${JPB_PC_GAME_BMD}"
                            --headless
                            --frames 3
                            --output
                            "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_pc_theed_water.ppm")
                    set_tests_properties(
                        jpb_pc_theed_water
                        PROPERTIES
                            LABELS "real_assets;pc_game;level;water"
                            PASS_REGULAR_EXPRESSION
                                "water_poly=[1-9][0-9]*/")
                endif()
                add_test(
                    NAME jpb_pc_enemy_radar
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --validate-radar
                        --frames 2)
                set_tests_properties(
                    jpb_pc_enemy_radar
                    PROPERTIES
                        LABELS "real_assets;pc_game;enemy;presentation")
                add_test(
                    NAME jpb_pc_enemy_teleport
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --headless
                        --validate-teleport
                        --frames 1)
                set_tests_properties(
                    jpb_pc_enemy_teleport
                    PROPERTIES
                        LABELS "real_assets;pc_game;enemy")
                add_test(
                    NAME jpb_pc_multi_enemy
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --validate-multi-enemy
                        --frames 9)
                set_tests_properties(
                    jpb_pc_multi_enemy
                    PROPERTIES
                        LABELS "real_assets;pc_game;enemy"
                        PASS_REGULAR_EXPRESSION
                            "classes=12/11/")
                add_test(
                    NAME jpb_pc_player_attack_smoke
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --headless-attack
                        --frames 16
                        --output
                        "$<TARGET_FILE_DIR:jpb_pc_game>/jpb_player_attack_smoke.ppm")
                set_tests_properties(
                    jpb_pc_player_attack_smoke
                    PROPERTIES LABELS "real_assets;pc_game;combat")
                foreach(JPB_ATTACK_DIRECTION IN ITEMS south west)
                    add_test(
                        NAME jpb_pc_player_attack_${JPB_ATTACK_DIRECTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_PC_GAME_CAD}"
                            --bmd
                            "${JPB_PC_GAME_BMD}"
                            --cmb
                            "${JPB_PC_GAME_CMB}"
                            --headless
                            --headless-phase-p1
                            attack-${JPB_ATTACK_DIRECTION}
                            1
                            --headless-phase-p1
                            none
                            44
                            --frames 45)
                    set_tests_properties(
                        jpb_pc_player_attack_${JPB_ATTACK_DIRECTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls")
                endforeach()
                foreach(JPB_QUI_GON_ATTACK_DIRECTION IN ITEMS north south west)
                    if(JPB_QUI_GON_ATTACK_DIRECTION STREQUAL "north")
                        set(JPB_QUI_GON_ATTACK_MOTION
                            "100\\[qg_hack_h_base\\]")
                    elseif(JPB_QUI_GON_ATTACK_DIRECTION STREQUAL "south")
                        set(JPB_QUI_GON_ATTACK_MOTION
                            "124\\[qg_hack_base\\]")
                    else()
                        set(JPB_QUI_GON_ATTACK_MOTION
                            "99\\[qg_slash_r_base\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_qui_gon_attack_${JPB_QUI_GON_ATTACK_DIRECTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_PC_PLAYER_TWO_CAD}"
                            --bmd
                            "${JPB_PC_PLAYER_TWO_BMD}"
                            --cmb
                            "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-model 1
                            --headless
                            --mute
                            --headless-phase-p1
                            attack-${JPB_QUI_GON_ATTACK_DIRECTION}
                            1
                            --headless-phase-p1
                            none
                            44
                            --frames 45)
                    set_tests_properties(
                        jpb_pc_qui_gon_attack_${JPB_QUI_GON_ATTACK_DIRECTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls"
                            PASS_REGULAR_EXPRESSION
                                "bits=000000(10|40|80)/00000000.*motion=${JPB_QUI_GON_ATTACK_MOTION}")
                endforeach()
                foreach(JPB_MOVE_DIRECTION IN ITEMS up down left right)
                    if(JPB_MOVE_DIRECTION STREQUAL "up")
                        set(JPB_MOVE_BITS "00001000")
                    elseif(JPB_MOVE_DIRECTION STREQUAL "down")
                        set(JPB_MOVE_BITS "00004000")
                    elseif(JPB_MOVE_DIRECTION STREQUAL "left")
                        set(JPB_MOVE_BITS "00008000")
                    else()
                        set(JPB_MOVE_BITS "00002000")
                    endif()
                    add_test(
                        NAME jpb_pc_player_move_${JPB_MOVE_DIRECTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless
                            --mute
                            --headless-phase-p1 ${JPB_MOVE_DIRECTION} 10
                            --headless-phase-p1 none 1
                            --frames 12)
                    set_tests_properties(
                        jpb_pc_player_move_${JPB_MOVE_DIRECTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;movement;controls"
                            PASS_REGULAR_EXPRESSION
                                "bits=${JPB_MOVE_BITS}/00000000.*motion=2\\[jd_run_core\\]")
                endforeach()
                add_test(
                    NAME jpb_pc_player_run_release_skid
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-phase-p1 up 21
                        --headless-phase-p1 none 20
                        --frames 42)
                set_tests_properties(
                    jpb_pc_player_run_release_skid
                    PROPERTIES
                        LABELS "real_assets;pc_game;movement;controls;context;release"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=1.*motion=25\\[jd_runstop_core\\]")
                add_test(
                    NAME jpb_pc_player_run_block_skid
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-phase-p1 up 21
                        --headless-phase-p1 block 1
                        --headless-phase-p1 none 20
                        --frames 43)
                set_tests_properties(
                    jpb_pc_player_run_block_skid
                    PROPERTIES
                        LABELS "real_assets;pc_game;movement;controls;context;block"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=1.*bits=00000004/00000000.*motion=25\\[jd_runstop_core\\]")
                add_test(
                    NAME jpb_pc_player_lock_on
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --mute
                        # The FED director owns P1 until its authored intro
                        # releases control. Exercise lock-on only afterward.
                        --headless-phase-p1 none 360
                        --headless-phase-p1 lock 1
                        --headless-phase-p1 none 10
                        --frames 372)
                set_tests_properties(
                    jpb_pc_player_lock_on
                    PROPERTIES
                        LABELS "real_assets;pc_game;combat;controls"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=1.*bits=00000008/00000000.*motion=20\\[jd_idle_b_core\\].*lock_target=\\(active=1,id=17\\)"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_player_lock_on_edges
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --mute
                        # A held edge toggles once; release re-arms it.
                        --headless-phase-p1 none 360
                        --headless-phase-p1 lock 3
                        --headless-phase-p1 none 3
                        --headless-phase-p1 lock 3
                        --headless-phase-p1 none 10
                        --frames 380)
                set_tests_properties(
                    jpb_pc_player_lock_on_edges
                    PROPERTIES
                        LABELS "real_assets;pc_game;combat;controls;edges"
                        PASS_REGULAR_EXPRESSION
                            "control_edges=\\(p1=pressed_frames:2,held_frames:6,release_events:2,pressed_bits:00000008,held_bits:00000008,released_bits:00000008,lock_toggles:2,lock:0"
                        TIMEOUT 120)
                if(EXISTS "${JPB_PC_ARENA_MESH}")
                    add_test(
                        NAME jpb_pc_player_analog_walk_threshold
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless
                            --mute
                            --headless-stick-pair 0 1000 0 0 10
                            --headless-stick-pair 0 0 0 0 1
                            --frames 12)
                    set_tests_properties(
                        jpb_pc_player_analog_walk_threshold
                        PROPERTIES
                            LABELS "real_assets;pc_game;movement;controls;analog"
                            PASS_REGULAR_EXPRESSION
                                "control_edges=\\(p1=.*pressed_bits:00001000.*locomotion:[1-9][0-9]*/1,direction:10,axis:0\\.00000/-0\\.03052,camera:-2032,desired:8176,facing:4080"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_player_analog_run_threshold
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless
                            --mute
                            --headless-stick-pair 0 3000 0 0 10
                            --headless-stick-pair 0 0 0 0 1
                            --frames 12)
                    set_tests_properties(
                        jpb_pc_player_analog_run_threshold
                        PROPERTIES
                            LABELS "real_assets;pc_game;movement;controls;analog"
                            PASS_REGULAR_EXPRESSION
                                "control_edges=\\(p1=.*pressed_bits:00001400.*locomotion:[1-9][0-9]*/2,direction:10,axis:0\\.00000/-0\\.09156,camera:-2032,desired:8176,facing:4080"
                            TIMEOUT 120)
                    foreach(JPB_ANALOG_PLAYER_ID RANGE 1 8)
                        list(GET JPB_BASE_PLAYER_NAMES
                            ${JPB_ANALOG_PLAYER_ID} JPB_ANALOG_PLAYER_NAME)
                        foreach(JPB_ANALOG_GAIT IN ITEMS walk run)
                            if(JPB_ANALOG_GAIT STREQUAL "walk")
                                set(JPB_ANALOG_LY 1000)
                                set(JPB_ANALOG_BITS 00001000)
                                set(JPB_ANALOG_MOTION 1)
                            else()
                                set(JPB_ANALOG_LY 3000)
                                set(JPB_ANALOG_BITS 00001400)
                                set(JPB_ANALOG_MOTION 2)
                            endif()
                            add_test(
                                NAME jpb_pc_${JPB_ANALOG_PLAYER_NAME}_analog_${JPB_ANALOG_GAIT}
                                COMMAND
                                    jpb_pc_game
                                    "${JPB_PC_ARENA_MESH}"
                                    --cad "${JPB_GAME_DATA_DIR}/res/animation/${JPB_ANALOG_PLAYER_NAME}.cad"
                                    --bmd "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_ANALOG_PLAYER_NAME}.bmd"
                                    --cmb "${JPB_GAME_DATA_DIR}/res/combo/${JPB_ANALOG_PLAYER_NAME}.cmb"
                                    --player-model ${JPB_ANALOG_PLAYER_ID}
                                    --headless
                                    --mute
                                    --headless-stick-pair 0 ${JPB_ANALOG_LY} 0 0 10
                                    --headless-stick-pair 0 0 0 0 1
                                    --frames 12)
                            set_tests_properties(
                                jpb_pc_${JPB_ANALOG_PLAYER_NAME}_analog_${JPB_ANALOG_GAIT}
                                PROPERTIES
                                    LABELS "real_assets;pc_game;movement;controls;analog;roster"
                                    PASS_REGULAR_EXPRESSION
                                        "control_edges=\\(p1=.*pressed_bits:${JPB_ANALOG_BITS}.*locomotion:[1-9][0-9]*/${JPB_ANALOG_MOTION},direction:10.*camera:-2032,desired:8176,facing:4080"
                                    TIMEOUT 120)
                        endforeach()
                    endforeach()
                endif()
                if(EXISTS "${JPB_PC_ARENA_MESH}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_CAD}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_BMD}" AND
                   EXISTS "${JPB_PC_PLAYER_TWO_CMB}")
                    add_test(
                        NAME jpb_pc_two_player_independent_controls
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --headless
                            --mute
                            --headless-phase-pair up down 10
                            --headless-phase-pair none none 1
                            --frames 12)
                    set_tests_properties(
                        jpb_pc_two_player_independent_controls
                        PROPERTIES
                            LABELS "real_assets;pc_game;movement;controls;versus"
                            PASS_REGULAR_EXPRESSION
                                "control_edges=\\(p1=pressed_frames:1,held_frames:10,release_events:1,pressed_bits:00001000,held_bits:00001000,released_bits:00001000.*locomotion:[1-9][0-9]*/2.*p2=pressed_frames:1,held_frames:10,release_events:1,pressed_bits:00004000,held_bits:00004000,released_bits:00004000.*locomotion:[1-9][0-9]*/1.*\\)"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_two_player_saber_ownership
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-model 0
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --validate-player-saber
                            --headless --mute
                            --headless-phase-pair attack-north attack-north 1
                            --headless-phase-pair none none 44
                            --frames 45)
                    set_tests_properties(
                        jpb_pc_two_player_saber_ownership
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls;saber;two_player;versus"
                            PASS_REGULAR_EXPRESSION
                                "player_weapon_action=\\(model=0,saber=1,frames=[1-9][0-9]*,valid=[1-9][0-9]*,invalid=0.*second_player_weapon_action=\\(model=1,saber=1,frames=[1-9][0-9]*,valid=[1-9][0-9]*,invalid=0"
                            TIMEOUT 120)
                    if(EXISTS "${JPB_GAME_DATA_DIR}/res/animation/mace.cad" AND
                       EXISTS "${JPB_GAME_DATA_DIR}/res/MODEL/mace.bmd" AND
                       EXISTS "${JPB_GAME_DATA_DIR}/res/combo/mace.cmb")
                        add_test(
                            NAME jpb_pc_two_player_p2_mace_jump_recovery
                            COMMAND
                                jpb_pc_game "${JPB_PC_ARENA_MESH}"
                                --cad "${JPB_PC_GAME_CAD}"
                                --bmd "${JPB_PC_GAME_BMD}"
                                --cmb "${JPB_PC_GAME_CMB}"
                                --player-model 0
                                --player-two-cad
                                    "${JPB_GAME_DATA_DIR}/res/animation/mace.cad"
                                --player-two-bmd
                                    "${JPB_GAME_DATA_DIR}/res/MODEL/mace.bmd"
                                --player-two-cmb
                                    "${JPB_GAME_DATA_DIR}/res/combo/mace.cmb"
                                --player-two-model 2
                                --headless --mute
                                --headless-phase-pair none jump 1
                                --headless-phase-pair none none 100
                                --frames 102)
                        set_tests_properties(
                            jpb_pc_two_player_p2_mace_jump_recovery
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;movement;jump;recovery;two_player;versus;provenance"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=0.*p2=\\(ready=1.*motion=4\\[jd_jump_b_core\\].*state=00000001,callbacks=1/1/0.*phase_end=1.*p2=\\(ready=1.*motion=0\\[mw_idle_a_core\\].*lock=0.*state=00000000,callbacks=1/0/0"
                                TIMEOUT 120)
                        add_test(
                            NAME jpb_pc_two_player_p2_mace_double_jump
                            COMMAND
                                jpb_pc_game "${JPB_PC_ARENA_MESH}"
                                --cad "${JPB_PC_GAME_CAD}"
                                --bmd "${JPB_PC_GAME_BMD}"
                                --cmb "${JPB_PC_GAME_CMB}"
                                --player-model 0
                                --player-two-cad
                                    "${JPB_GAME_DATA_DIR}/res/animation/mace.cad"
                                --player-two-bmd
                                    "${JPB_GAME_DATA_DIR}/res/MODEL/mace.bmd"
                                --player-two-cmb
                                    "${JPB_GAME_DATA_DIR}/res/combo/mace.cmb"
                                --player-two-model 2
                                --headless --mute
                                --headless-phase-pair none jump 1
                                --headless-phase-pair none none 15
                                --headless-phase-pair none jump 1
                                --headless-phase-pair none none 15
                                --frames 33)
                        set_tests_properties(
                            jpb_pc_two_player_p2_mace_double_jump
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;movement;jump;double_jump;two_player;versus;provenance"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=2.*p2=\\(ready=1.*motion=22\\[jd_jump_double_core\\].*state=00000001,callbacks=1/1/0.*control_edges=\\(p1=.*p2=pressed_frames:2.*motion:22"
                                TIMEOUT 120)
                    endif()
                    add_test(
                        NAME jpb_pc_xinput_two_player_p2_context_flags
                        COMMAND
                            jpb_pc_game "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --headless --mute
                            --headless-xinput-phase-pair none back 8
                            --headless-xinput-phase-pair none none 1
                            --headless-xinput-phase-pair none start 1
                            --headless-xinput-phase-pair none none 3
                            --frames 14)
                    set_tests_properties(
                        jpb_pc_xinput_two_player_p2_context_flags
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;context;zoom;pause;two_player;versus"
                            PASS_REGULAR_EXPRESSION
                                "control_context=\\(game_flags=00402000,orbit=4000\\.000,handoff_guard=00\\).*control_edges=\\(p1=pressed_frames:0,held_frames:0,release_events:0,pressed_bits:00000000.*p2=pressed_frames:2,held_frames:9,release_events:2,pressed_bits:00000900,held_bits:00000900,released_bits:00000900"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_xinput_two_player_p2_lock_direction
                        COMMAND
                            jpb_pc_game "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --headless --mute
                            --headless-xinput-phase-pair none rb 1
                            --headless-xinput-phase-pair none none 1
                            # Toward the P1 target in this authored Arena
                            # camera selects Qui-Gon's locked right hop.
                            --headless-stick-pair 0 0 0 -3000 10
                            --headless-stick-pair 0 0 0 0 1
                            --frames 14)
                    set_tests_properties(
                        jpb_pc_xinput_two_player_p2_lock_direction
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;lock_on;movement;two_player;versus"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*p2=\\(ready=1.*motion=20\\[qg_idle_b_core\\].*target=0,lock_flag=1.*phase_end=2.*p2=\\(ready=1.*motion=30\\[jd_hop_r_core\\].*target=0,lock_flag=1.*control_edges=\\(p1=pressed_frames:0,held_frames:0,release_events:0,pressed_bits:00000000.*p2=pressed_frames:2,held_frames:11,release_events:2,pressed_bits:00004408.*lock_toggles:1,lock:1,motion:30"
                            TIMEOUT 120)
                    foreach(JPB_P2_ATTACK IN ITEMS north west south)
                        if(JPB_P2_ATTACK STREQUAL "north")
                            set(JPB_P2_ATTACK_BUTTON b)
                            set(JPB_P2_ATTACK_BITS 00000010)
                            set(JPB_P2_ATTACK_MOTION
                                "100\\[qg_hack_h_base\\]")
                        elseif(JPB_P2_ATTACK STREQUAL "west")
                            set(JPB_P2_ATTACK_BUTTON x)
                            set(JPB_P2_ATTACK_BITS 00000080)
                            set(JPB_P2_ATTACK_MOTION
                                "99\\[qg_slash_r_base\\]")
                        else()
                            set(JPB_P2_ATTACK_BUTTON y)
                            set(JPB_P2_ATTACK_BITS 00000040)
                            set(JPB_P2_ATTACK_MOTION
                                "124\\[qg_hack_base\\]")
                        endif()
                        add_test(
                            NAME jpb_pc_xinput_p2_attack_${JPB_P2_ATTACK}
                            COMMAND
                                jpb_pc_game "${JPB_PC_ARENA_MESH}"
                                --cad "${JPB_PC_GAME_CAD}"
                                --bmd "${JPB_PC_GAME_BMD}"
                                --cmb "${JPB_PC_GAME_CMB}"
                                --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                                --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                                --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                                --player-two-model 1
                                --headless --mute
                                --headless-xinput-phase-pair
                                    none ${JPB_P2_ATTACK_BUTTON} 1
                                --headless-xinput-phase-pair none none 44
                                --frames 45)
                        set_tests_properties(
                            jpb_pc_xinput_p2_attack_${JPB_P2_ATTACK}
                            PROPERTIES
                                LABELS "real_assets;pc_game;combat;controls;xinput;two_player;versus"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=0.*p2=\\(ready=1.*pad=${JPB_P2_ATTACK_BITS}/${JPB_P2_ATTACK_BITS},motion=${JPB_P2_ATTACK_MOTION}.*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=pressed_frames:0,held_frames:0,release_events:0,pressed_bits:00000000.*p2=pressed_frames:1,held_frames:1,release_events:1,pressed_bits:${JPB_P2_ATTACK_BITS}"
                                TIMEOUT 120)
                    endforeach()
                    foreach(JPB_P2_FORCE IN ITEMS north jump south item)
                        if(JPB_P2_FORCE STREQUAL "north")
                            set(JPB_P2_FORCE_BUTTONS lt+b)
                            set(JPB_P2_FORCE_BITS 00000011)
                            set(JPB_P2_FORCE_MOTION
                                "67\\[jd_push_b_force\\]")
                        elseif(JPB_P2_FORCE STREQUAL "jump")
                            set(JPB_P2_FORCE_BUTTONS lt+a)
                            set(JPB_P2_FORCE_BITS 00000021)
                            set(JPB_P2_FORCE_MOTION
                                "96\\[jd_heal_a_force\\]")
                        elseif(JPB_P2_FORCE STREQUAL "south")
                            set(JPB_P2_FORCE_BUTTONS lt+y)
                            set(JPB_P2_FORCE_BITS 00000041)
                            set(JPB_P2_FORCE_MOTION
                                "66\\[jd_push_a_force\\]")
                        else()
                            set(JPB_P2_FORCE_BUTTONS lt+x)
                            set(JPB_P2_FORCE_BITS 00000081)
                            set(JPB_P2_FORCE_MOTION
                                "65\\[jd_throw_item_custom\\]")
                        endif()
                        add_test(
                            NAME jpb_pc_xinput_p2_force_${JPB_P2_FORCE}
                            COMMAND
                                jpb_pc_game "${JPB_PC_ARENA_MESH}"
                                --cad "${JPB_PC_GAME_CAD}"
                                --bmd "${JPB_PC_GAME_BMD}"
                                --cmb "${JPB_PC_GAME_CMB}"
                                --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                                --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                                --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                                --player-two-model 1
                                --headless --mute
                                --headless-maximum-progression
                                --headless-xinput-phase-pair
                                    none ${JPB_P2_FORCE_BUTTONS} 1
                                --headless-xinput-phase-pair none none 44
                                --frames 45)
                        set_tests_properties(
                            jpb_pc_xinput_p2_force_${JPB_P2_FORCE}
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;xinput;force;two_player;versus"
                                PASS_REGULAR_EXPRESSION
                                    "phase_end=0.*p2=\\(ready=1.*pad=${JPB_P2_FORCE_BITS}/${JPB_P2_FORCE_BITS},motion=${JPB_P2_FORCE_MOTION}.*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=pressed_frames:0,held_frames:0,release_events:0,pressed_bits:00000000.*p2=pressed_frames:1,held_frames:1,release_events:1,pressed_bits:${JPB_P2_FORCE_BITS}"
                                TIMEOUT 120)
                    endforeach()
                    add_test(
                        NAME jpb_pc_mixed_control_recovery_sequence
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-model 0
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --validate-player-saber
                            --headless --mute --headless-maximum-progression
                            --headless-phase-p1 up 21
                            --headless-phase-p1 up+attack-north 1
                            --headless-phase-p1 none 40
                            --headless-phase-p1 block 4
                            # Lock toggles before the block's lock-25 recovery.
                            --headless-phase-p1 lock 1
                            --headless-phase-p1 none 20
                            --headless-phase-p1 left 1
                            --headless-phase-p1 none 30
                            --headless-phase-p1 jump 1
                            # The trajectory callback owns input through the
                            # authored landing; Force begins only after idle.
                            --headless-phase-p1 none 80
                            --headless-phase-p1 force+attack-north 1
                            --headless-phase-p1 none 45
                            --frames 246)
                    set_tests_properties(
                        jpb_pc_mixed_control_recovery_sequence
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;combat;movement;block;lock_on;jump;force;sequence;recovery;saber;two_player;versus"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*motion=2\\[jd_run_core\\].*phase_end=1.*motion=94\\[qg_slash_run\\].*phase_end=4.*motion=15\\[jd_block_active_core\\].*lock_flag=1.*phase_end=5.*motion=20\\[jd_idle_b_core\\].*lock_flag=1.*phase_end=6.*motion=26\\[jd_shuffle_f_core\\].*phase_end=8.*motion=4\\[jd_jump_b_core\\].*phase_end=9.*motion=0\\[jd_idle_a_core\\].*lock_flag=0.*phase_end=10.*motion=99\\[jd_charge_fwd_force\\].*player_weapon_action=\\(model=0,saber=1,frames=77,valid=77,invalid=0"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_xinput_two_player_mixed_sequence
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-model 0
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --validate-player-saber
                            --headless --mute --headless-maximum-progression
                            --headless-xinput-phase-pair b none 1
                            --headless-xinput-phase-pair none none 45
                            --headless-xinput-phase-pair none b 1
                            --headless-xinput-phase-pair none none 50
                            --headless-xinput-phase-pair rb none 1
                            --headless-xinput-phase-pair none none 1
                            --headless-xinput-phase-pair a none 1
                            --headless-xinput-phase-pair none none 80
                            --headless-xinput-phase-pair none lt+b 1
                            --headless-xinput-phase-pair none none 80
                            --headless-xinput-phase-pair lb lb 4
                            --headless-xinput-phase-pair none none 25
                            --frames 291)
                    set_tests_properties(
                        jpb_pc_xinput_two_player_mixed_sequence
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;combat;block;lock_on;jump;force;sequence;recovery;saber;two_player;versus"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*motion=106\\[jd_hack_h_base\\].*p2=\\(ready=1.*motion=0\\[qg_idle_a_core\\].*phase_end=2.*p2=\\(ready=1.*motion=100\\[qg_hack_h_base\\].*phase_end=4.*motion=20\\[jd_idle_b_core\\].*lock_flag=1.*p2=\\(ready=1.*motion=100\\[qg_hack_h_base\\].*phase_end=6.*motion=4\\[jd_jump_b_core\\].*phase_end=7.*motion=0\\[jd_idle_a_core\\].*lock_flag=0.*p2=\\(ready=1.*motion=0\\[qg_idle_a_core\\].*phase_end=8.*p2=\\(ready=1.*motion=67\\[jd_push_b_force\\].*phase_end=10.*motion=15\\[jd_block_active_core\\].*p2=\\(ready=1.*motion=15\\[qg_block_active_core\\].*phase_end=11.*motion=0\\[jd_idle_a_core\\].*p2=\\(ready=1.*motion=0\\[qg_idle_a_core\\].*second_player_weapon_action=\\(model=1,saber=1,frames=55,valid=55,invalid=0.*control_sources=\\(p1=xinput,p2=xinput,last=xinput\\).*control_edges=\\(p1=pressed_frames:4,held_frames:7,release_events:4,pressed_bits:0000003c.*p2=pressed_frames:3,held_frames:6,release_events:3,pressed_bits:00000015"
                            TIMEOUT 120)
                    add_test(
                        NAME jpb_pc_two_player_camera_relative_analog
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_ARENA_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                            --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                            --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                            --player-two-model 1
                            --headless
                            --mute
                            --headless-stick-pair 0 3000 3000 0 10
                            --headless-stick-pair 0 0 0 0 1
                            --frames 12)
                    set_tests_properties(
                        jpb_pc_two_player_camera_relative_analog
                        PROPERTIES
                            LABELS "real_assets;pc_game;movement;controls;analog;versus"
                            PASS_REGULAR_EXPRESSION
                                "control_edges=\\(p1=.*pressed_bits:00001400.*locomotion:[1-9][0-9]*/2,direction:10,axis:0\\.00000/-0\\.09156,camera:475,desired:5669,facing:5669.*p2=.*pressed_bits:00002400.*locomotion:[1-9][0-9]*/2,direction:10,axis:0\\.09156/-0\\.00000,camera:475,desired:4645,facing:4645\\)"
                            TIMEOUT 120)
                    foreach(JPB_LOCK_PLAYER_ID RANGE 0 8)
                        list(GET JPB_BASE_PLAYER_NAMES
                            ${JPB_LOCK_PLAYER_ID} JPB_LOCK_PLAYER_NAME)
                        foreach(JPB_LOCK_DIRECTION IN ITEMS forward left right back)
                            if(JPB_LOCK_DIRECTION STREQUAL "forward")
                                set(JPB_LOCK_MOTION 26)
                                list(GET JPB_BASE_LOCK_FORWARD_NAMES
                                    ${JPB_LOCK_PLAYER_ID} JPB_LOCK_MOTION_NAME)
                                set(JPB_LOCK_DIRECTION_COMMAND
                                    --headless-stick-pair -2670 3000 0 0 1
                                    --headless-stick-pair 0 0 0 0 1)
                            elseif(JPB_LOCK_DIRECTION STREQUAL "left")
                                set(JPB_LOCK_MOTION 29)
                                set(JPB_LOCK_MOTION_NAME jd_hop_l_core)
                                set(JPB_LOCK_DIRECTION_COMMAND
                                    --headless-phase-p1 left 1
                                    --headless-phase-p1 none 1)
                            elseif(JPB_LOCK_DIRECTION STREQUAL "right")
                                set(JPB_LOCK_MOTION 30)
                                set(JPB_LOCK_MOTION_NAME jd_hop_r_core)
                                set(JPB_LOCK_DIRECTION_COMMAND
                                    --headless-phase-p1 up 1
                                    --headless-phase-p1 none 1)
                            else()
                                set(JPB_LOCK_MOTION 12)
                                list(GET JPB_BASE_LOCK_BACK_NAMES
                                    ${JPB_LOCK_PLAYER_ID} JPB_LOCK_MOTION_NAME)
                                set(JPB_LOCK_DIRECTION_COMMAND
                                    --headless-phase-p1 down 1
                                    --headless-phase-p1 none 1)
                            endif()
                            add_test(
                                NAME jpb_pc_${JPB_LOCK_PLAYER_NAME}_lock_${JPB_LOCK_DIRECTION}
                                COMMAND
                                    jpb_pc_game
                                    "${JPB_PC_ARENA_MESH}"
                                    --cad
                                    "${JPB_GAME_DATA_DIR}/res/animation/${JPB_LOCK_PLAYER_NAME}.cad"
                                    --bmd
                                    "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_LOCK_PLAYER_NAME}.bmd"
                                    --cmb
                                    "${JPB_GAME_DATA_DIR}/res/combo/${JPB_LOCK_PLAYER_NAME}.cmb"
                                    --player-model ${JPB_LOCK_PLAYER_ID}
                                    --player-two-cad "${JPB_PC_PLAYER_TWO_CAD}"
                                    --player-two-bmd "${JPB_PC_PLAYER_TWO_BMD}"
                                    --player-two-cmb "${JPB_PC_PLAYER_TWO_CMB}"
                                    --player-two-model 1
                                    --headless --mute
                                    --headless-phase-p1 lock 1
                                    --headless-phase-p1 none 1
                                    ${JPB_LOCK_DIRECTION_COMMAND}
                                    --frames 5)
                            set_tests_properties(
                                jpb_pc_${JPB_LOCK_PLAYER_NAME}_lock_${JPB_LOCK_DIRECTION}
                                PROPERTIES
                                    LABELS "real_assets;pc_game;movement;controls;lock_on;context;roster;versus"
                                    PASS_REGULAR_EXPRESSION
                                        "phase_end=2.*motion=${JPB_LOCK_MOTION}\\[${JPB_LOCK_MOTION_NAME}\\].*lock_target=\\(active=1,id=1\\)"
                                    TIMEOUT 120)
                        endforeach()
                    endforeach()
                endif()
                # One exact authored two-hit chain per base character.  The
                # timing values are intentionally character-owned: each CAD
                # has a different base-attack lock window and chain duration.
                set(JPB_BASE_COMBO_ACTIONS
                    south west west west west south west west west)
                set(JPB_BASE_COMBO_GAPS
                    15 8 8 8 8 8 8 8 8)
                set(JPB_BASE_COMBO_TAILS
                    20 30 50 30 60 30 20 12 30)
                set(JPB_BASE_COMBO_MOTIONS
                    119 112 118 123 122 124 105 100 122)
                set(JPB_BASE_COMBO_MOTION_NAMES
                    jd_hack_up_add jd_slashlow_l_add mw_hack_add
                    ag_slash_add pl_slash_r_add pl_slash_l_base
                    jd_sidekick_add bd_r_hook_add jd_slashlow_l_add)
                set(JPB_BASE_COMBO_RECORDS
                    4 6 4 5 6 3 4 4 4)
                set(JPB_BASE_COMBO_PREFIXES
                    s.s. w.w. w.w. w.w. w.w. s.s. w.w. w.w. w.w.)
                foreach(JPB_COMBO_PLAYER_ID RANGE 0 8)
                    list(GET JPB_BASE_PLAYER_NAMES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_PLAYER_NAME)
                    list(GET JPB_BASE_COMBO_ACTIONS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_ACTION)
                    list(GET JPB_BASE_COMBO_GAPS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_GAP)
                    list(GET JPB_BASE_COMBO_TAILS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_TAIL)
                    list(GET JPB_BASE_COMBO_MOTIONS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_MOTION)
                    list(GET JPB_BASE_COMBO_MOTION_NAMES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_MOTION_NAME)
                    list(GET JPB_BASE_COMBO_RECORDS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_RECORD)
                    list(GET JPB_BASE_COMBO_PREFIXES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_PREFIX)
                    math(EXPR JPB_COMBO_FRAMES
                        "${JPB_COMBO_GAP} + ${JPB_COMBO_TAIL} + 3")
                    add_test(
                        NAME jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_two_hit
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_COMBO_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_COMBO_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_COMBO_PLAYER_NAME}.cmb"
                            --player-model ${JPB_COMBO_PLAYER_ID}
                            --headless --mute
                            --headless-phase-p1
                            attack-${JPB_COMBO_ACTION} 1
                            --headless-phase-p1 none ${JPB_COMBO_GAP}
                            --headless-phase-p1
                            attack-${JPB_COMBO_ACTION} 1
                            --headless-phase-p1 none ${JPB_COMBO_TAIL}
                            --frames ${JPB_COMBO_FRAMES})
                    set_tests_properties(
                        jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_two_hit
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls;combo;roster"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=2.*combo_tally=2/${JPB_COMBO_RECORD} pre=${JPB_COMBO_PREFIX}.*queue=.*${JPB_COMBO_MOTION}\\[${JPB_COMBO_MOTION_NAME}\\]"
                            TIMEOUT 120)
                endforeach()
                # Exercise a third authored attack after the queued two-hit
                # motion.  These cases deliberately use maximum progression
                # because several third-stage CMB records are unlock rewards.
                set(JPB_BASE_COMBO_THREE_ACTIONS
                    north north west north north north west west north)
                set(JPB_BASE_COMBO_THREE_TAILS
                    40 60 70 60 100 80 40 30 50)
                set(JPB_BASE_COMBO_THREE_MOTIONS
                    106 123 115 126 119 135 103 104 114)
                set(JPB_BASE_COMBO_THREE_MOTION_NAMES
                    jd_hack_h_base qg_backhand_2hit_custom
                    jd_slash_360_add ag_cartwheel_custom pl_spin_l_add
                    dm_slash_h_base jd_kick_360_add bd_haymaker_l_base
                    jd_slash_360_add)
                set(JPB_BASE_COMBO_THREE_RECORDS
                    6 10 8 10 10 5 5 6 7)
                set(JPB_BASE_COMBO_THREE_PREFIXES
                    s.s.n. w.w.n. w.w.w. w.w.n. w.w.n.
                    s.s.n. w.w.w. w.w.w. w.w.n.)
                foreach(JPB_COMBO_PLAYER_ID RANGE 0 8)
                    list(GET JPB_BASE_PLAYER_NAMES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_PLAYER_NAME)
                    list(GET JPB_BASE_COMBO_ACTIONS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_ACTION)
                    list(GET JPB_BASE_COMBO_GAPS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_GAP)
                    list(GET JPB_BASE_COMBO_THREE_ACTIONS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_THREE_ACTION)
                    list(GET JPB_BASE_COMBO_THREE_TAILS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_THREE_TAIL)
                    list(GET JPB_BASE_COMBO_THREE_MOTIONS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_THREE_MOTION)
                    list(GET JPB_BASE_COMBO_THREE_MOTION_NAMES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_THREE_MOTION_NAME)
                    list(GET JPB_BASE_COMBO_THREE_RECORDS
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_THREE_RECORD)
                    list(GET JPB_BASE_COMBO_THREE_PREFIXES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_THREE_PREFIX)
                    math(EXPR JPB_COMBO_THREE_FRAMES
                        "${JPB_COMBO_GAP} + ${JPB_COMBO_THREE_TAIL} + 13")
                    add_test(
                        NAME jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_three_hit
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_COMBO_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_COMBO_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_COMBO_PLAYER_NAME}.cmb"
                            --player-model ${JPB_COMBO_PLAYER_ID}
                            --headless --mute --headless-maximum-progression
                            --headless-phase-p1
                            attack-${JPB_COMBO_ACTION} 1
                            --headless-phase-p1 none ${JPB_COMBO_GAP}
                            --headless-phase-p1
                            attack-${JPB_COMBO_ACTION} 1
                            --headless-phase-p1 none 8
                            --headless-phase-p1
                            attack-${JPB_COMBO_THREE_ACTION} 1
                            --headless-phase-p1
                            none ${JPB_COMBO_THREE_TAIL}
                            --frames ${JPB_COMBO_THREE_FRAMES})
                    set_tests_properties(
                        jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_three_hit
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls;combo;roster;progression"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=4.*combo_tally=3/${JPB_COMBO_THREE_RECORD} pre=${JPB_COMBO_THREE_PREFIX}.*queue=.*${JPB_COMBO_THREE_MOTION}\\[${JPB_COMBO_THREE_MOTION_NAME}\\]"
                            TIMEOUT 120)
                endforeach()
                # Amidala's authored CMB ends at three hits.  Every other
                # base-character CMB has a non-Force fourth-stage chain.
                set(JPB_BASE_COMBO_FOUR_PLAYER_IDS
                    0 1 2 3 4 5 7 8)
                set(JPB_BASE_COMBO_FOUR_ACTIONS
                    south west west south west south west west)
                set(JPB_BASE_COMBO_FOUR_GAPS
                    15 8 8 8 8 8 8 8)
                set(JPB_BASE_COMBO_FOUR_THIRD_ACTIONS
                    north north west west south south west west)
                set(JPB_BASE_COMBO_FOUR_FOURTH_ACTIONS
                    west north south west north west south west)
                set(JPB_BASE_COMBO_FOUR_TAILS
                    80 120 100 120 160 140 60 100)
                set(JPB_BASE_COMBO_FOUR_MOTIONS
                    124 110 133 103 114 93 108 115)
                set(JPB_BASE_COMBO_FOUR_MOTION_NAMES
                    jd_overhead_slash_add jd_uppercut_r_add
                    mw_backhand_2hit_custom ag_hack_base jd_slashes_l_add
                    dm_slash_run hu_blaster_pistol_single
                    jd_backhand_360_add)
                set(JPB_BASE_COMBO_FOUR_RECORDS
                    11 15 11 15 14 9 7 11)
                set(JPB_BASE_COMBO_FOUR_PREFIXES
                    s.s.n.w. w.w.n.n. w.w.w.s. s.s.w.w.
                    w.w.s.n. s.s.s.w. w.w.w.s. w.w.w.w.)
                foreach(JPB_COMBO_FOUR_INDEX RANGE 0 7)
                    list(GET JPB_BASE_COMBO_FOUR_PLAYER_IDS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_PLAYER_ID)
                    list(GET JPB_BASE_PLAYER_NAMES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_PLAYER_NAME)
                    list(GET JPB_BASE_COMBO_FOUR_ACTIONS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_ACTION)
                    list(GET JPB_BASE_COMBO_FOUR_GAPS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_GAP)
                    list(GET JPB_BASE_COMBO_FOUR_THIRD_ACTIONS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_THREE_ACTION)
                    list(GET JPB_BASE_COMBO_FOUR_FOURTH_ACTIONS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_FOUR_ACTION)
                    list(GET JPB_BASE_COMBO_FOUR_TAILS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_FOUR_TAIL)
                    list(GET JPB_BASE_COMBO_FOUR_MOTIONS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_FOUR_MOTION)
                    list(GET JPB_BASE_COMBO_FOUR_MOTION_NAMES
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_FOUR_MOTION_NAME)
                    list(GET JPB_BASE_COMBO_FOUR_RECORDS
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_FOUR_RECORD)
                    list(GET JPB_BASE_COMBO_FOUR_PREFIXES
                        ${JPB_COMBO_FOUR_INDEX} JPB_COMBO_FOUR_PREFIX)
                    math(EXPR JPB_COMBO_FOUR_FRAMES
                        "${JPB_COMBO_GAP} + ${JPB_COMBO_FOUR_TAIL} + 29")
                    add_test(
                        NAME jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_four_hit
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_COMBO_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_COMBO_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_COMBO_PLAYER_NAME}.cmb"
                            --player-model ${JPB_COMBO_PLAYER_ID}
                            --headless --mute --headless-maximum-progression
                            --headless-phase-p1
                            attack-${JPB_COMBO_ACTION} 1
                            --headless-phase-p1 none ${JPB_COMBO_GAP}
                            --headless-phase-p1
                            attack-${JPB_COMBO_ACTION} 1
                            --headless-phase-p1 none 8
                            --headless-phase-p1
                            attack-${JPB_COMBO_THREE_ACTION} 1
                            --headless-phase-p1 none 8
                            --headless-phase-p1
                            attack-${JPB_COMBO_FOUR_ACTION} 1
                            --headless-phase-p1 none ${JPB_COMBO_FOUR_TAIL}
                            --frames ${JPB_COMBO_FOUR_FRAMES})
                    set_tests_properties(
                        jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_four_hit
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls;combo;roster;progression"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=6.*combo_tally=4/${JPB_COMBO_FOUR_RECORD} pre=${JPB_COMBO_FOUR_PREFIX}.*queue=.*${JPB_COMBO_FOUR_MOTION}\\[${JPB_COMBO_FOUR_MOTION_NAME}\\]"
                            TIMEOUT 120)
                endforeach()
                # The four CMB sets below continue to a fifth non-Force
                # attack.  Panaka deliberately reuses Motion 108 for records
                # 7 and 8, so its exact comboTally record proves that the
                # fifth input was accepted even though Motion.combo is
                # rewritten through the authored prev/numHits relationship.
                set(JPB_BASE_COMBO_FIVE_PLAYER_IDS 0 3 5 7)
                set(JPB_BASE_COMBO_FIVE_ACTIONS south south west west)
                set(JPB_BASE_COMBO_FIVE_GAPS 15 8 2 8)
                set(JPB_BASE_COMBO_FIVE_THIRD_ACTIONS
                    north west west west)
                set(JPB_BASE_COMBO_FIVE_FOURTH_ACTIONS
                    west west south south)
                set(JPB_BASE_COMBO_FIVE_FIFTH_ACTIONS
                    west south north south)
                set(JPB_BASE_COMBO_FIVE_TAILS 110 160 80 80)
                set(JPB_BASE_COMBO_FIVE_MOTIONS 118 138 73 108)
                set(JPB_BASE_COMBO_FIVE_MOTION_NAMES
                    jd_slash_l_add ag_hack2_add jd_defend_c_force
                    hu_blaster_pistol_single)
                set(JPB_BASE_COMBO_FIVE_CURRENT_COMBOS 15 19 11 5)
                set(JPB_BASE_COMBO_FIVE_TALLY_LAST 15 19 11 8)
                set(JPB_BASE_COMBO_FIVE_PREFIXES
                    s.s.n.w.w. s.s.w.w.s. w.w.w.s.n. w.w.w.s.s.)
                foreach(JPB_COMBO_FIVE_INDEX RANGE 0 3)
                    list(GET JPB_BASE_COMBO_FIVE_PLAYER_IDS
                        ${JPB_COMBO_FIVE_INDEX} JPB_COMBO_PLAYER_ID)
                    list(GET JPB_BASE_PLAYER_NAMES
                        ${JPB_COMBO_PLAYER_ID} JPB_COMBO_PLAYER_NAME)
                    foreach(JPB_COMBO_FIVE_FIELD IN ITEMS
                        ACTIONS GAPS THIRD_ACTIONS FOURTH_ACTIONS
                        FIFTH_ACTIONS TAILS MOTIONS MOTION_NAMES
                        CURRENT_COMBOS TALLY_LAST PREFIXES)
                        list(GET JPB_BASE_COMBO_FIVE_${JPB_COMBO_FIVE_FIELD}
                            ${JPB_COMBO_FIVE_INDEX}
                            JPB_COMBO_FIVE_${JPB_COMBO_FIVE_FIELD})
                    endforeach()
                    math(EXPR JPB_COMBO_FIVE_FRAMES
                        "${JPB_COMBO_FIVE_GAPS} + ${JPB_COMBO_FIVE_TAILS} + 40")
                    add_test(
                        NAME jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_five_hit
                        COMMAND
                            jpb_pc_game "${JPB_PC_GAME_MESH}"
                            --cad
                            "${JPB_GAME_DATA_DIR}/res/animation/${JPB_COMBO_PLAYER_NAME}.cad"
                            --bmd
                            "${JPB_GAME_DATA_DIR}/res/MODEL/${JPB_COMBO_PLAYER_NAME}.bmd"
                            --cmb
                            "${JPB_GAME_DATA_DIR}/res/combo/${JPB_COMBO_PLAYER_NAME}.cmb"
                            --player-model ${JPB_COMBO_PLAYER_ID}
                            --headless --mute --headless-maximum-progression
                            --headless-phase-p1
                            attack-${JPB_COMBO_FIVE_ACTIONS} 1
                            --headless-phase-p1 none ${JPB_COMBO_FIVE_GAPS}
                            --headless-phase-p1
                            attack-${JPB_COMBO_FIVE_ACTIONS} 1
                            --headless-phase-p1 none 8
                            --headless-phase-p1
                            attack-${JPB_COMBO_FIVE_THIRD_ACTIONS} 1
                            --headless-phase-p1 none 8
                            --headless-phase-p1
                            attack-${JPB_COMBO_FIVE_FOURTH_ACTIONS} 1
                            --headless-phase-p1 none 8
                            --headless-phase-p1
                            attack-${JPB_COMBO_FIVE_FIFTH_ACTIONS} 1
                            --headless-phase-p1 none ${JPB_COMBO_FIVE_TAILS}
                            --frames ${JPB_COMBO_FIVE_FRAMES})
                    set_tests_properties(
                        jpb_pc_${JPB_COMBO_PLAYER_NAME}_combo_five_hit
                        PROPERTIES
                            LABELS "real_assets;pc_game;combat;controls;combo;roster;progression"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=8.*combo_tally=5/${JPB_COMBO_FIVE_TALLY_LAST} pre=${JPB_COMBO_FIVE_PREFIXES}.*queue=.*${JPB_COMBO_FIVE_MOTIONS}\\[${JPB_COMBO_FIVE_MOTION_NAMES}\\]"
                            TIMEOUT 120)
                endforeach()
                add_test(
                    NAME jpb_pc_obi_wan_combo_six_hit
                    COMMAND
                        jpb_pc_game "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --player-model 0
                        --headless --mute --headless-maximum-progression
                        --headless-phase-p1 attack-south 1
                        --headless-phase-p1 none 15
                        --headless-phase-p1 attack-south 1
                        --headless-phase-p1 none 8
                        --headless-phase-p1 attack-north 1
                        --headless-phase-p1 none 8
                        --headless-phase-p1 attack-west 1
                        --headless-phase-p1 none 8
                        --headless-phase-p1 attack-west 1
                        --headless-phase-p1 none 8
                        --headless-phase-p1 attack-north 1
                        --headless-phase-p1 none 130
                        --frames 197)
                set_tests_properties(
                    jpb_pc_obi_wan_combo_six_hit
                    PROPERTIES
                        LABELS "real_assets;pc_game;combat;controls;combo;roster;progression"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=10.*combo_tally=6/18 pre=s.s.n.w.w.n..*queue=.*114\\[jd_slash_360_add\\]"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_player_block
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --headless
                        --mute
                        --headless-phase block 8
                        --headless-phase none 12
                        --frames 20)
                set_tests_properties(
                    jpb_pc_player_block
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls"
                        PASS_REGULAR_EXPRESSION
                            "bits=00000004/00000004.*motion=15\\[jd_block_active_core\\]")
                add_test(
                    NAME jpb_pc_player_block_release_recovery
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-phase-p1 block 8
                        --headless-phase-p1 none 60
                        --frames 69)
                set_tests_properties(
                    jpb_pc_player_block_release_recovery
                    PROPERTIES
                        LABELS
                            "real_assets;pc_game;controls;block;release;recovery;provenance"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=0.*bits=00000004/00000000.*motion=15\\[jd_block_active_core\\].*queue=21\\[jd_idle_block_core\\].*lock=25.*phase_end=1.*bits=00000000/00000000.*motion=0\\[jd_idle_a_core\\].*queue=<empty>.*lock=0.*control_edges=\\(p1=pressed_frames:1,held_frames:8,release_events:1,pressed_bits:00000004,held_bits:00000004,released_bits:00000004"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_qui_gon_block
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_PLAYER_TWO_CAD}"
                        --bmd
                        "${JPB_PC_PLAYER_TWO_BMD}"
                        --cmb
                        "${JPB_PC_PLAYER_TWO_CMB}"
                        --player-model 1
                        --headless
                        --mute
                        --headless-phase block 8
                        --headless-phase none 12
                        --frames 20)
                set_tests_properties(
                    jpb_pc_qui_gon_block
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls"
                        PASS_REGULAR_EXPRESSION
                            "bits=00000004/00000004.*motion=15\\[qg_block_active_core\\]")
                foreach(JPB_FORCE_CHARACTER IN ITEMS obi_wan qui_gon)
                    if(JPB_FORCE_CHARACTER STREQUAL "obi_wan")
                        set(JPB_FORCE_CAD "${JPB_PC_GAME_CAD}")
                        set(JPB_FORCE_BMD "${JPB_PC_GAME_BMD}")
                        set(JPB_FORCE_CMB "${JPB_PC_GAME_CMB}")
                        set(JPB_FORCE_MODEL 0)
                    else()
                        set(JPB_FORCE_CAD "${JPB_PC_PLAYER_TWO_CAD}")
                        set(JPB_FORCE_BMD "${JPB_PC_PLAYER_TWO_BMD}")
                        set(JPB_FORCE_CMB "${JPB_PC_PLAYER_TWO_CMB}")
                        set(JPB_FORCE_MODEL 1)
                    endif()
                    foreach(JPB_FORCE_ACTION IN ITEMS north jump south item)
                        if(JPB_FORCE_ACTION STREQUAL "north")
                            set(JPB_FORCE_BUTTONS "force+attack-north")
                            set(JPB_FORCE_BITS "00000011")
                            if(JPB_FORCE_CHARACTER STREQUAL "obi_wan")
                                set(JPB_FORCE_MOTION
                                    "99\\[jd_charge_fwd_force\\]")
                            else()
                                set(JPB_FORCE_MOTION
                                    "67\\[jd_push_b_force\\]")
                            endif()
                        elseif(JPB_FORCE_ACTION STREQUAL "jump")
                            set(JPB_FORCE_BUTTONS "force+jump")
                            set(JPB_FORCE_BITS "00000021")
                            if(JPB_FORCE_CHARACTER STREQUAL "obi_wan")
                                set(JPB_FORCE_MOTION
                                    "134\\[jd_meditate_a_force\\]")
                            else()
                                set(JPB_FORCE_MOTION
                                    "96\\[jd_heal_a_force\\]")
                            endif()
                        elseif(JPB_FORCE_ACTION STREQUAL "south")
                            set(JPB_FORCE_BUTTONS "force+attack-south")
                            set(JPB_FORCE_BITS "00000041")
                            set(JPB_FORCE_MOTION
                                "66\\[jd_push_a_force\\]")
                        else()
                            set(JPB_FORCE_BUTTONS "force+attack-west")
                            set(JPB_FORCE_BITS "00000081")
                            set(JPB_FORCE_MOTION
                                "65\\[jd_throw_item_custom\\]")
                        endif()
                        add_test(
                            NAME jpb_pc_${JPB_FORCE_CHARACTER}_force_${JPB_FORCE_ACTION}
                            COMMAND
                                jpb_pc_game
                                "${JPB_PC_GAME_MESH}"
                                --cad "${JPB_FORCE_CAD}"
                                --bmd "${JPB_FORCE_BMD}"
                                --cmb "${JPB_FORCE_CMB}"
                                --player-model ${JPB_FORCE_MODEL}
                                --headless
                                --mute
                                --headless-maximum-progression
                                --headless-phase-p1 "${JPB_FORCE_BUTTONS}" 1
                                --headless-phase-p1 none 44
                                --frames 45)
                        set_tests_properties(
                            jpb_pc_${JPB_FORCE_CHARACTER}_force_${JPB_FORCE_ACTION}
                            PROPERTIES
                                LABELS "real_assets;pc_game;controls;force"
                                PASS_REGULAR_EXPRESSION
                                    "bits=${JPB_FORCE_BITS}/00000000.*motion=${JPB_FORCE_MOTION}")
                    endforeach()
                endforeach()
                foreach(JPB_XINPUT_ACTION IN ITEMS jump north west south)
                    if(JPB_XINPUT_ACTION STREQUAL "jump")
                        set(JPB_XINPUT_BUTTON "a")
                        set(JPB_XINPUT_BITS "00000020")
                        set(JPB_XINPUT_PRESSED_BITS "00000000")
                        set(JPB_XINPUT_HELD_FRAMES 8)
                        set(JPB_XINPUT_RELEASE_FRAMES 3)
                        set(JPB_XINPUT_TOTAL_FRAMES 12)
                        set(JPB_XINPUT_MOTION "4\\[jd_jump_b_core\\]")
                    elseif(JPB_XINPUT_ACTION STREQUAL "north")
                        set(JPB_XINPUT_BUTTON "b")
                        set(JPB_XINPUT_BITS "00000010")
                        set(JPB_XINPUT_PRESSED_BITS "00000010")
                        set(JPB_XINPUT_HELD_FRAMES 1)
                        set(JPB_XINPUT_RELEASE_FRAMES 30)
                        set(JPB_XINPUT_TOTAL_FRAMES 32)
                        set(JPB_XINPUT_MOTION "106\\[jd_hack_h_base\\]")
                    elseif(JPB_XINPUT_ACTION STREQUAL "west")
                        set(JPB_XINPUT_BUTTON "x")
                        set(JPB_XINPUT_BITS "00000080")
                        set(JPB_XINPUT_PRESSED_BITS "00000080")
                        set(JPB_XINPUT_HELD_FRAMES 1)
                        set(JPB_XINPUT_RELEASE_FRAMES 30)
                        set(JPB_XINPUT_TOTAL_FRAMES 32)
                        set(JPB_XINPUT_MOTION "105\\[jd_slash_r_base\\]")
                    else()
                        set(JPB_XINPUT_BUTTON "y")
                        set(JPB_XINPUT_BITS "00000040")
                        set(JPB_XINPUT_PRESSED_BITS "00000040")
                        set(JPB_XINPUT_HELD_FRAMES 1)
                        set(JPB_XINPUT_RELEASE_FRAMES 30)
                        set(JPB_XINPUT_TOTAL_FRAMES 32)
                        set(JPB_XINPUT_MOTION "107\\[jd_hack_base\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_xinput_${JPB_XINPUT_ACTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute
                            --headless-xinput-phase
                                "${JPB_XINPUT_BUTTON}"
                                ${JPB_XINPUT_HELD_FRAMES}
                            --headless-xinput-phase none
                                ${JPB_XINPUT_RELEASE_FRAMES}
                            --frames ${JPB_XINPUT_TOTAL_FRAMES})
                    set_tests_properties(
                        jpb_pc_xinput_${JPB_XINPUT_ACTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;context"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*pad=${JPB_XINPUT_PRESSED_BITS}/${JPB_XINPUT_BITS}.*motion=${JPB_XINPUT_MOTION}"
                            TIMEOUT 120)
                endforeach()
                foreach(JPB_XINPUT_FORCE_ACTION IN ITEMS north jump south item)
                    if(JPB_XINPUT_FORCE_ACTION STREQUAL "north")
                        set(JPB_XINPUT_FORCE_BUTTONS "lt+b")
                        set(JPB_XINPUT_FORCE_BITS "00000011")
                        set(JPB_XINPUT_FORCE_MOTION
                            "99\\[jd_charge_fwd_force\\]")
                    elseif(JPB_XINPUT_FORCE_ACTION STREQUAL "jump")
                        set(JPB_XINPUT_FORCE_BUTTONS "lt+a")
                        set(JPB_XINPUT_FORCE_BITS "00000021")
                        set(JPB_XINPUT_FORCE_MOTION
                            "134\\[jd_meditate_a_force\\]")
                    elseif(JPB_XINPUT_FORCE_ACTION STREQUAL "south")
                        set(JPB_XINPUT_FORCE_BUTTONS "lt+y")
                        set(JPB_XINPUT_FORCE_BITS "00000041")
                        set(JPB_XINPUT_FORCE_MOTION
                            "66\\[jd_push_a_force\\]")
                    else()
                        set(JPB_XINPUT_FORCE_BUTTONS "lt+x")
                        set(JPB_XINPUT_FORCE_BITS "00000081")
                        set(JPB_XINPUT_FORCE_MOTION
                            "65\\[jd_throw_item_custom\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_xinput_force_${JPB_XINPUT_FORCE_ACTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute
                            --headless-maximum-progression
                            --headless-xinput-phase
                                "${JPB_XINPUT_FORCE_BUTTONS}" 1
                            --headless-xinput-phase none 44
                            --frames 45)
                    set_tests_properties(
                        jpb_pc_xinput_force_${JPB_XINPUT_FORCE_ACTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;force;context"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*pad=${JPB_XINPUT_FORCE_BITS}/${JPB_XINPUT_FORCE_BITS}.*motion=${JPB_XINPUT_FORCE_MOTION}"
                            TIMEOUT 120)
                endforeach()
                # The executable's recovered Classic scheme (ControllerConfig
                # zero) uses the original face-button order and LB as its Force
                # modifier.  Exercise physical XInput all the way through the
                # mapper, player update, and authored FED animation resources.
                foreach(JPB_XINPUT_CLASSIC_ACTION IN ITEMS jump north west south)
                    if(JPB_XINPUT_CLASSIC_ACTION STREQUAL "jump")
                        set(JPB_XINPUT_CLASSIC_BUTTON "b")
                        set(JPB_XINPUT_CLASSIC_BITS "00000020")
                        set(JPB_XINPUT_CLASSIC_PRESSED_BITS "00000000")
                        set(JPB_XINPUT_CLASSIC_HELD_FRAMES 8)
                        set(JPB_XINPUT_CLASSIC_RELEASE_FRAMES 3)
                        set(JPB_XINPUT_CLASSIC_TOTAL_FRAMES 12)
                        set(JPB_XINPUT_CLASSIC_MOTION "4\\[jd_jump_b_core\\]")
                    elseif(JPB_XINPUT_CLASSIC_ACTION STREQUAL "north")
                        set(JPB_XINPUT_CLASSIC_BUTTON "y")
                        set(JPB_XINPUT_CLASSIC_BITS "00000010")
                        set(JPB_XINPUT_CLASSIC_PRESSED_BITS "00000010")
                        set(JPB_XINPUT_CLASSIC_HELD_FRAMES 1)
                        set(JPB_XINPUT_CLASSIC_RELEASE_FRAMES 30)
                        set(JPB_XINPUT_CLASSIC_TOTAL_FRAMES 32)
                        set(JPB_XINPUT_CLASSIC_MOTION "106\\[jd_hack_h_base\\]")
                    elseif(JPB_XINPUT_CLASSIC_ACTION STREQUAL "west")
                        set(JPB_XINPUT_CLASSIC_BUTTON "x")
                        set(JPB_XINPUT_CLASSIC_BITS "00000080")
                        set(JPB_XINPUT_CLASSIC_PRESSED_BITS "00000080")
                        set(JPB_XINPUT_CLASSIC_HELD_FRAMES 1)
                        set(JPB_XINPUT_CLASSIC_RELEASE_FRAMES 30)
                        set(JPB_XINPUT_CLASSIC_TOTAL_FRAMES 32)
                        set(JPB_XINPUT_CLASSIC_MOTION "105\\[jd_slash_r_base\\]")
                    else()
                        set(JPB_XINPUT_CLASSIC_BUTTON "a")
                        set(JPB_XINPUT_CLASSIC_BITS "00000040")
                        set(JPB_XINPUT_CLASSIC_PRESSED_BITS "00000040")
                        set(JPB_XINPUT_CLASSIC_HELD_FRAMES 1)
                        set(JPB_XINPUT_CLASSIC_RELEASE_FRAMES 30)
                        set(JPB_XINPUT_CLASSIC_TOTAL_FRAMES 32)
                        set(JPB_XINPUT_CLASSIC_MOTION "107\\[jd_hack_base\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_xinput_classic_${JPB_XINPUT_CLASSIC_ACTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute
                            --control-scheme p1 classic
                            --headless-xinput-phase
                                "${JPB_XINPUT_CLASSIC_BUTTON}"
                                ${JPB_XINPUT_CLASSIC_HELD_FRAMES}
                            --headless-xinput-phase none
                                ${JPB_XINPUT_CLASSIC_RELEASE_FRAMES}
                            --frames ${JPB_XINPUT_CLASSIC_TOTAL_FRAMES})
                    set_tests_properties(
                        jpb_pc_xinput_classic_${JPB_XINPUT_CLASSIC_ACTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;classic;context"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*pad=${JPB_XINPUT_CLASSIC_PRESSED_BITS}/${JPB_XINPUT_CLASSIC_BITS}.*motion=${JPB_XINPUT_CLASSIC_MOTION}"
                            TIMEOUT 120)
                endforeach()
                foreach(JPB_XINPUT_CLASSIC_FORCE_ACTION IN ITEMS north jump south item)
                    if(JPB_XINPUT_CLASSIC_FORCE_ACTION STREQUAL "north")
                        set(JPB_XINPUT_CLASSIC_FORCE_BUTTONS "lb+y")
                        set(JPB_XINPUT_CLASSIC_FORCE_BITS "00000014")
                        set(JPB_XINPUT_CLASSIC_FORCE_MOTION
                            "99\\[jd_charge_fwd_force\\]")
                    elseif(JPB_XINPUT_CLASSIC_FORCE_ACTION STREQUAL "jump")
                        set(JPB_XINPUT_CLASSIC_FORCE_BUTTONS "lb+b")
                        set(JPB_XINPUT_CLASSIC_FORCE_BITS "00000024")
                        set(JPB_XINPUT_CLASSIC_FORCE_MOTION
                            "134\\[jd_meditate_a_force\\]")
                    elseif(JPB_XINPUT_CLASSIC_FORCE_ACTION STREQUAL "south")
                        set(JPB_XINPUT_CLASSIC_FORCE_BUTTONS "lb+a")
                        set(JPB_XINPUT_CLASSIC_FORCE_BITS "00000044")
                        set(JPB_XINPUT_CLASSIC_FORCE_MOTION
                            "66\\[jd_push_a_force\\]")
                    else()
                        set(JPB_XINPUT_CLASSIC_FORCE_BUTTONS "lb+x")
                        set(JPB_XINPUT_CLASSIC_FORCE_BITS "00000084")
                        set(JPB_XINPUT_CLASSIC_FORCE_MOTION
                            "65\\[jd_throw_item_custom\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_xinput_classic_force_${JPB_XINPUT_CLASSIC_FORCE_ACTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute
                            --control-scheme p1 classic
                            --headless-maximum-progression
                            --headless-xinput-phase
                                "${JPB_XINPUT_CLASSIC_FORCE_BUTTONS}" 1
                            --headless-xinput-phase none 44
                            --frames 45)
                    set_tests_properties(
                        jpb_pc_xinput_classic_force_${JPB_XINPUT_CLASSIC_FORCE_ACTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;xinput;classic;force;context"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*pad=${JPB_XINPUT_CLASSIC_FORCE_BITS}/${JPB_XINPUT_CLASSIC_FORCE_BITS}.*motion=${JPB_XINPUT_CLASSIC_FORCE_MOTION}"
                            TIMEOUT 120)
                endforeach()
                add_test(
                    NAME jpb_pc_xinput_block
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-xinput-phase lb 8
                        --headless-xinput-phase none 12
                        --frames 20)
                set_tests_properties(
                    jpb_pc_xinput_block
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;xinput;block;context"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=0.*pad=00000000/00000004.*motion=15\\[jd_block_active_core\\]"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_xinput_lock_on
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless --mute
                        --headless-xinput-phase none 360
                        --headless-xinput-phase rb 1
                        --headless-xinput-phase none 10
                        --frames 372)
                set_tests_properties(
                    jpb_pc_xinput_lock_on
                    PROPERTIES
                        LABELS "real_assets;pc_game;combat;controls;xinput;lock_on;context"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=1.*pad=00000008/00000008.*motion=20\\[jd_idle_b_core\\].*lock_target=\\(active=1,id=17\\)"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_xinput_zoom_context
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-xinput-phase back 8
                        --headless-xinput-phase none 3
                        --frames 12)
                set_tests_properties(
                    jpb_pc_xinput_zoom_context
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;xinput;context;zoom"
                        PASS_REGULAR_EXPRESSION
                            "control_context=\\(game_flags=00200000,orbit=3496\\.763,handoff_guard=00\\).*control_edges=\\(p1=pressed_frames:1,held_frames:8,release_events:1,pressed_bits:00000100"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_xinput_pause_context
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-xinput-phase start 1
                        --headless-xinput-phase none 3
                        --frames 5)
                set_tests_properties(
                    jpb_pc_xinput_pause_context
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;xinput;context;pause"
                        PASS_REGULAR_EXPRESSION
                            "control_context=\\(game_flags=00001000,orbit=4000\\.000,handoff_guard=00\\).*control_edges=\\(p1=pressed_frames:1,held_frames:1,release_events:1,pressed_bits:00000800"
                        TIMEOUT 120)
                # Drive the physical PC key layout through ReadKeyboardInput's
                # recovered translation seam and the real gameplay brain. The
                # pure mapper tests protect its bits; these gates prove those
                # bits select the authored animations and context actions.
                foreach(JPB_KEYBOARD_ACTION IN ITEMS jump north west south)
                    if(JPB_KEYBOARD_ACTION STREQUAL "jump")
                        set(JPB_KEYBOARD_KEY "space")
                        set(JPB_KEYBOARD_BITS "00000120")
                        set(JPB_KEYBOARD_PRESSED_BITS "00000000")
                        set(JPB_KEYBOARD_HELD_FRAMES 8)
                        set(JPB_KEYBOARD_RELEASE_FRAMES 3)
                        set(JPB_KEYBOARD_TOTAL_FRAMES 12)
                        set(JPB_KEYBOARD_MOTION "4\\[jd_jump_b_core\\]")
                    elseif(JPB_KEYBOARD_ACTION STREQUAL "north")
                        set(JPB_KEYBOARD_KEY "l")
                        set(JPB_KEYBOARD_BITS "00000010")
                        set(JPB_KEYBOARD_PRESSED_BITS "00000010")
                        set(JPB_KEYBOARD_HELD_FRAMES 1)
                        set(JPB_KEYBOARD_RELEASE_FRAMES 30)
                        set(JPB_KEYBOARD_TOTAL_FRAMES 32)
                        set(JPB_KEYBOARD_MOTION "106\\[jd_hack_h_base\\]")
                    elseif(JPB_KEYBOARD_ACTION STREQUAL "west")
                        set(JPB_KEYBOARD_KEY "k")
                        set(JPB_KEYBOARD_BITS "00000080")
                        set(JPB_KEYBOARD_PRESSED_BITS "00000080")
                        set(JPB_KEYBOARD_HELD_FRAMES 1)
                        set(JPB_KEYBOARD_RELEASE_FRAMES 30)
                        set(JPB_KEYBOARD_TOTAL_FRAMES 32)
                        set(JPB_KEYBOARD_MOTION "105\\[jd_slash_r_base\\]")
                    else()
                        set(JPB_KEYBOARD_KEY "j")
                        set(JPB_KEYBOARD_BITS "00000040")
                        set(JPB_KEYBOARD_PRESSED_BITS "00000040")
                        set(JPB_KEYBOARD_HELD_FRAMES 1)
                        set(JPB_KEYBOARD_RELEASE_FRAMES 30)
                        set(JPB_KEYBOARD_TOTAL_FRAMES 32)
                        set(JPB_KEYBOARD_MOTION "107\\[jd_hack_base\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_keyboard_${JPB_KEYBOARD_ACTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute
                            --headless-keyboard-phase
                                "${JPB_KEYBOARD_KEY}"
                                ${JPB_KEYBOARD_HELD_FRAMES}
                            --headless-keyboard-phase none
                                ${JPB_KEYBOARD_RELEASE_FRAMES}
                            --frames ${JPB_KEYBOARD_TOTAL_FRAMES})
                    set_tests_properties(
                        jpb_pc_keyboard_${JPB_KEYBOARD_ACTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;keyboard;context"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*pad=${JPB_KEYBOARD_PRESSED_BITS}/${JPB_KEYBOARD_BITS}.*motion=${JPB_KEYBOARD_MOTION}.*control_sources=\\(p1=keyboard"
                            TIMEOUT 120)
                endforeach()
                foreach(JPB_KEYBOARD_FORCE_ACTION IN ITEMS north jump south item)
                    if(JPB_KEYBOARD_FORCE_ACTION STREQUAL "north")
                        set(JPB_KEYBOARD_FORCE_KEY "i")
                        set(JPB_KEYBOARD_FORCE_BITS "00000015")
                        set(JPB_KEYBOARD_FORCE_MOTION
                            "99\\[jd_charge_fwd_force\\]")
                    elseif(JPB_KEYBOARD_FORCE_ACTION STREQUAL "jump")
                        set(JPB_KEYBOARD_FORCE_KEY "u")
                        set(JPB_KEYBOARD_FORCE_BITS "00000025")
                        set(JPB_KEYBOARD_FORCE_MOTION
                            "134\\[jd_meditate_a_force\\]")
                    elseif(JPB_KEYBOARD_FORCE_ACTION STREQUAL "south")
                        set(JPB_KEYBOARD_FORCE_KEY "o")
                        set(JPB_KEYBOARD_FORCE_BITS "00000045")
                        set(JPB_KEYBOARD_FORCE_MOTION
                            "66\\[jd_push_a_force\\]")
                    else()
                        set(JPB_KEYBOARD_FORCE_KEY "y")
                        set(JPB_KEYBOARD_FORCE_BITS "00000085")
                        set(JPB_KEYBOARD_FORCE_MOTION
                            "65\\[jd_throw_item_custom\\]")
                    endif()
                    add_test(
                        NAME jpb_pc_keyboard_force_${JPB_KEYBOARD_FORCE_ACTION}
                        COMMAND
                            jpb_pc_game
                            "${JPB_PC_GAME_MESH}"
                            --cad "${JPB_PC_GAME_CAD}"
                            --bmd "${JPB_PC_GAME_BMD}"
                            --cmb "${JPB_PC_GAME_CMB}"
                            --headless --mute
                            --headless-maximum-progression
                            --headless-keyboard-phase
                                "${JPB_KEYBOARD_FORCE_KEY}" 1
                            --headless-keyboard-phase none 44
                            --frames 45)
                    set_tests_properties(
                        jpb_pc_keyboard_force_${JPB_KEYBOARD_FORCE_ACTION}
                        PROPERTIES
                            LABELS "real_assets;pc_game;controls;keyboard;force;context"
                            PASS_REGULAR_EXPRESSION
                                "phase_end=0.*pad=${JPB_KEYBOARD_FORCE_BITS}/${JPB_KEYBOARD_FORCE_BITS}.*motion=${JPB_KEYBOARD_FORCE_MOTION}.*control_sources=\\(p1=keyboard"
                            TIMEOUT 120)
                endforeach()
                add_test(
                    NAME jpb_pc_keyboard_block
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-keyboard-phase shift 8
                        --headless-keyboard-phase none 12
                        --frames 20)
                set_tests_properties(
                    jpb_pc_keyboard_block
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;keyboard;block;context"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=0.*pad=00000000/00000005.*motion=15\\[jd_block_active_core\\].*control_sources=\\(p1=keyboard"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_keyboard_run
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-keyboard-phase w 10
                        --headless-keyboard-phase none 1
                        --frames 12)
                set_tests_properties(
                    jpb_pc_keyboard_run
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;keyboard;movement"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=0.*pad=00000000/00001000.*motion=2\\[jd_run_core\\].*control_sources=\\(p1=keyboard"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_keyboard_walk_modifier
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-keyboard-phase w+lctrl 10
                        --headless-keyboard-phase none 1
                        --frames 12)
                set_tests_properties(
                    jpb_pc_keyboard_walk_modifier
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;keyboard;movement"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=0.*pad=00000000/00001400.*motion=1\\[jd_walk2_core\\].*control_sources=\\(p1=keyboard"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_keyboard_lock_on
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --enemy-cad "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless --mute
                        --headless-keyboard-phase none 360
                        --headless-keyboard-phase h 1
                        --headless-keyboard-phase none 10
                        --frames 372)
                set_tests_properties(
                    jpb_pc_keyboard_lock_on
                    PROPERTIES
                        LABELS "real_assets;pc_game;combat;controls;keyboard;lock_on;context"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=1.*pad=00000008/00000008.*motion=20\\[jd_idle_b_core\\].*lock_target=\\(active=1,id=17\\).*control_sources=\\(p1=keyboard"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_keyboard_zoom_context
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-keyboard-phase t 8
                        --headless-keyboard-phase none 3
                        --frames 12)
                set_tests_properties(
                    jpb_pc_keyboard_zoom_context
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;keyboard;context;zoom"
                        PASS_REGULAR_EXPRESSION
                            "control_context=\\(game_flags=00200000,orbit=3496\\.763,handoff_guard=00\\).*control_sources=\\(p1=keyboard.*control_edges=\\(p1=pressed_frames:1,held_frames:8,release_events:1,pressed_bits:00000100"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_keyboard_pause_context
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless --mute
                        --headless-keyboard-phase escape 1
                        --headless-keyboard-phase none 3
                        --frames 5)
                set_tests_properties(
                    jpb_pc_keyboard_pause_context
                    PROPERTIES
                        LABELS "real_assets;pc_game;controls;keyboard;context;pause"
                        PASS_REGULAR_EXPRESSION
                            "control_context=\\(game_flags=00001000,orbit=4000\\.000,handoff_guard=00\\).*control_sources=\\(p1=keyboard.*control_edges=\\(p1=pressed_frames:1,held_frames:1,release_events:1,pressed_bits:00000800"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_authored_combat
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --headless-phase none 2
                        --headless-phase attack 20
                        --headless-phase none 32
                        --validate-combat
                        --frames 55)
                set_tests_properties(
                    jpb_pc_authored_combat
                    PROPERTIES
                        LABELS "real_assets;pc_game;combat"
                        TIMEOUT 120)
                add_test(
                    NAME jpb_pc_authored_jump
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad
                        "${JPB_PC_GAME_CAD}"
                        --bmd
                        "${JPB_PC_GAME_BMD}"
                        --cmb
                        "${JPB_PC_GAME_CMB}"
                        --enemy-cad
                        "${JPB_PC_GAME_ENEMY_CAD}"
                        --enemy-bmd
                        "${JPB_PC_GAME_ENEMY_BMD}"
                        --headless
                        --headless-phase jump 8
                        --headless-phase none 1
                        --validate-jump
                        --frames 10)
                set_tests_properties(
                    jpb_pc_authored_jump
                    PROPERTIES
                        LABELS "real_assets;pc_game;movement")
                add_test(
                    NAME jpb_pc_authored_running_jump
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_GAME_CAD}"
                        --bmd "${JPB_PC_GAME_BMD}"
                        --cmb "${JPB_PC_GAME_CMB}"
                        --headless
                        --mute
                        --headless-phase-p1 up 1
                        --headless-phase-p1 up+jump 8
                        --headless-phase-p1 none 1
                        --validate-jump
                        --frames 11)
                set_tests_properties(
                    jpb_pc_authored_running_jump
                    PROPERTIES
                        LABELS "real_assets;pc_game;movement;controls;context"
                        PASS_REGULAR_EXPRESSION
                            "phase_end=1.*bits=00001020/00000000.*motion=4\\[jd_jump_b_core\\]")
                add_test(
                    NAME jpb_pc_qui_gon_authored_jump
                    COMMAND
                        jpb_pc_game
                        "${JPB_PC_GAME_MESH}"
                        --cad "${JPB_PC_PLAYER_TWO_CAD}"
                        --bmd "${JPB_PC_PLAYER_TWO_BMD}"
                        --cmb "${JPB_PC_PLAYER_TWO_CMB}"
                        --player-model 1
                        --headless
                        --mute
                        --headless-phase jump 8
                        --headless-phase none 1
                        --validate-jump
                        --frames 10)
                set_tests_properties(
                    jpb_pc_qui_gon_authored_jump
                    PROPERTIES
                        LABELS "real_assets;pc_game;movement;controls")
            endif()
        endif()
    endif()
endif()
