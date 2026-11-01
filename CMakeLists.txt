cmake_minimum_required(VERSION 3.20)
project(tenshi_engine)

set(EXTERNAL_DIR "external")
include(GNUInstallDirs)

# ============================================================
# Options
# ============================================================

option(TENSHI_PACKED
    "Pack game resources into TPK"
    OFF
)

option(TENSHI_BUILD_PACKER_ONLY
    "Build only the native resource packer"
    OFF
)

option(TENSHI_INSTALL
    "enable install rules for tenshi"
    OFF
)

set(TENSHI_RESOURCE_DIR
    "${CMAKE_SOURCE_DIR}/resources"
    CACHE PATH
    "Directory containing game resources"
)

set(TENSHI_PAK_FILE
    "${CMAKE_BINARY_DIR}/game_data/resource0.tpk"
    CACHE FILEPATH
    "First output resource archive chunk"
)

set(TENSHI_PAK_MAX_SIZE
    "67108864"
    CACHE STRING
    "Maximum TPK chunk size in bytes"
)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PakCrypto.cmake")
tenshi_configure_pak_crypto()

# --------------------------------------------------------
# Packer-only build
# Used by the Emscripten build to create a native packer
# --------------------------------------------------------
if(TENSHI_BUILD_PACKER_ONLY)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(liblha_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/liblha"
        liblha
    )
    add_executable(packer_tool
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/pak/main.cpp"
    )
    target_link_libraries(packer_tool 
        PRIVATE 
            liblha::liblha
            tenshi_pak_crypto
    )
    target_include_directories(packer_tool
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    return()
endif()

# ============================================================
# External dependencies
# ============================================================

add_library(miniaudio STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/miniaudio/miniaudio.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/miniaudio/miniaudio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/stb_vorbis.c"
)

target_include_directories(miniaudio
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/miniaudio>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(liblha_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/liblha"
    liblha
)

add_library(fkyaml INTERFACE)
target_include_directories(fkyaml
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/fkyaml/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(stb_image INTERFACE)
target_include_directories(stb_image
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/stb_image>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(stb_truetype INTERFACE)
target_include_directories(stb_truetype
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/stb_truetype>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# ============================================================
# Tenshi engine
# ============================================================

set(TENSHI_INCLUDE_DIR "include")
set(TENSHI_SOURCE_DIR  "src")
set(TENSHI_COPY_PAK_CHUNKS_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyPakChunks.cmake"
    CACHE INTERNAL
    "Path to the generated PAK chunk staging script"
    FORCE
)

add_library(tenshi STATIC)
add_library(tenshi::tenshi ALIAS tenshi)

target_sources(tenshi
    PRIVATE
        # Public includes
        "${TENSHI_INCLUDE_DIR}/Tenshi/Tenshi.h"
        
        "${TENSHI_INCLUDE_DIR}/Tenshi/Assets/AssetLoading.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Assets/AssetManager.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Assets/AssetRegistry.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Assets/Font.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Assets/ResourceLoading.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Audio/Audio.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Audio/Sound.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Audio/SoundInstance.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Character/Character.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Components/Components.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Core/Game.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Core/Log.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Core/Subroutine.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Entity/ComponentPool.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Entity/Entity.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Entity/World.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Files/FileSystem.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Files/Yaml.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/Sprite.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/SpriteBatch.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/Renderer.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/Text.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/TextureAtlas.h" 
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/Tilemap.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/Tileset.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Graphics/TileTypes.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/GUI/Element.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/GUI/Frame.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/GUI/Manager.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Input/Input.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Input/InputEvent.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Input/Key.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Platform/Platform.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Scene/Background.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Scene/Camera.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Scene/Collider.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Scene/Scene.h"
        "${TENSHI_INCLUDE_DIR}/Tenshi/Utility/Types.h"

        # Private
        "${TENSHI_SOURCE_DIR}/Assets/AssetLoading.cpp"
        "${TENSHI_SOURCE_DIR}/Assets/Font.cpp"
        "${TENSHI_SOURCE_DIR}/Assets/Image.cpp"
        "${TENSHI_SOURCE_DIR}/Assets/Image.h"
        "${TENSHI_SOURCE_DIR}/Assets/ResourceLoading.cpp"
        "${TENSHI_SOURCE_DIR}/Audio/Audio.cpp"
        "${TENSHI_SOURCE_DIR}/Audio/Sound.cpp"
        "${TENSHI_SOURCE_DIR}/Audio/SoundInstance.cpp"
        "${TENSHI_SOURCE_DIR}/Character/Character.cpp"
        "${TENSHI_SOURCE_DIR}/Core/CrashHandler.h"
        "${TENSHI_SOURCE_DIR}/Core/Game.cpp"
        "${TENSHI_SOURCE_DIR}/Core/GameHelper.h"
        "${TENSHI_SOURCE_DIR}/Core/Log.cpp"
        "${TENSHI_SOURCE_DIR}/Entity/World.cpp"
        "${TENSHI_SOURCE_DIR}/Files/FileSystem.cpp"
        "${TENSHI_SOURCE_DIR}/Files/PakArchive.cpp"
        "${TENSHI_SOURCE_DIR}/Files/PakArchive.h"
        "${TENSHI_SOURCE_DIR}/Files/PakFile.cpp"
        "${TENSHI_SOURCE_DIR}/Files/PakFile.h"
        "${TENSHI_SOURCE_DIR}/Files/Yaml.cpp"
        "${TENSHI_SOURCE_DIR}/Graphics/Sprite.cpp"
        "${TENSHI_SOURCE_DIR}/Graphics/SpriteBatch.cpp"
        "${TENSHI_SOURCE_DIR}/Graphics/Text.cpp"
        "${TENSHI_SOURCE_DIR}/Graphics/TextureAtlas.cpp"          
        "${TENSHI_SOURCE_DIR}/Graphics/Tilemap.cpp"
        "${TENSHI_SOURCE_DIR}/Graphics/TilemapFormat.h"
        "${TENSHI_SOURCE_DIR}/Graphics/Tileset.cpp"
        "${TENSHI_SOURCE_DIR}/GUI/Frame.cpp"
        "${TENSHI_SOURCE_DIR}/GUI/Manager.cpp"
        "${TENSHI_SOURCE_DIR}/Input/Input.cpp"
        "${TENSHI_SOURCE_DIR}/Scene/Background.cpp"
        "${TENSHI_SOURCE_DIR}/Scene/Camera.cpp"
        "${TENSHI_SOURCE_DIR}/Scene/Scene.cpp"
)

target_link_libraries(tenshi
    PUBLIC
        miniaudio
        liblha::liblha
        fkyaml
        stb_image
        stb_truetype
    PRIVATE
        tenshi_pak_crypto
)

add_library(tenshi_sdl3 STATIC
    "${TENSHI_SOURCE_DIR}/Backends/SDL3/SDLGraphicsBackend.cpp"
    "${TENSHI_SOURCE_DIR}/Backends/SDL3/SDLPlatform.cpp"
    "${TENSHI_SOURCE_DIR}/Backends/SDL3/SDLPlatform.h"
)
add_library(tenshi::tenshi_sdl3 ALIAS tenshi_sdl3)

target_link_libraries(tenshi_sdl3
    PUBLIC
        tenshi::tenshi
    PRIVATE
        "$<BUILD_INTERFACE:SDL3::SDL3>"
)
target_include_directories(tenshi_sdl3
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_include_directories(tenshi
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# TODO: Add more tests
include(CTest)
if(BUILD_TESTING)
    add_executable(tenshi_ecs_tests
        tests/ecs_tests.cpp
    )
    target_compile_features(tenshi_ecs_tests PRIVATE cxx_std_20)
    target_include_directories(tenshi_ecs_tests
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(tenshi_ecs_tests PRIVATE tenshi::tenshi)
    add_test(NAME tenshi_ecs_tests COMMAND tenshi_ecs_tests)

    if(TENSHI_PAK_ENCRYPTION AND NOT EMSCRIPTEN)
        add_executable(tenshi_pak_crypto_tests
            tests/pak_crypto_tests.cpp
        )
        target_link_libraries(tenshi_pak_crypto_tests
            PRIVATE tenshi_pak_crypto
        )
        target_include_directories(tenshi_pak_crypto_tests
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
        )
        add_test(NAME tenshi_pak_crypto_tests COMMAND tenshi_pak_crypto_tests)
    endif()
endif()

# ============================================================
# Resource packer
# ============================================================

if(TENSHI_PACKED)
    file(GLOB_RECURSE TENSHI_RESOURCE_FILES CONFIGURE_DEPENDS
        "${TENSHI_RESOURCE_DIR}/*"
    )

    # --------------------------------------------------------
    # Native build
    # --------------------------------------------------------
    if(NOT EMSCRIPTEN)
        add_executable(packer_tool
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pak/main.cpp"
        )

        target_link_libraries(packer_tool
            PRIVATE
                liblha::liblha
                tenshi_pak_crypto
        )
        target_include_directories(packer_tool
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
        )

        add_custom_command(
            OUTPUT
                "${TENSHI_PAK_FILE}"
            COMMAND
                "${CMAKE_COMMAND}"
                -E
                make_directory
                "${CMAKE_BINARY_DIR}/game_data"
            COMMAND
                "$<TARGET_FILE:packer_tool>"
                "${TENSHI_RESOURCE_DIR}"
                "${TENSHI_PAK_FILE}"
                "${TENSHI_PAK_MAX_SIZE}"
            DEPENDS
                packer_tool
                ${TENSHI_RESOURCE_FILES}
            COMMENT
                "Packing game resources"
            VERBATIM
        )

    # --------------------------------------------------------
    # Emscripten build
    # --------------------------------------------------------
    else()
        # --------------------------------------------------------
        # Emscripten host packer
        # --------------------------------------------------------

        include(ExternalProject)

        set(TENSHI_PACKER_BUILD_DIR
            "${CMAKE_BINARY_DIR}/tenshi-packer-build"
        )

        set(TENSHI_PACKER_INSTALL_DIR
            "${CMAKE_BINARY_DIR}/tenshi-packer-install"
        )

        if(CMAKE_HOST_WIN32)
            set(TENSHI_PACKER_SUFFIX ".exe")
        else()
            set(TENSHI_PACKER_SUFFIX "")
        endif()

        ExternalProject_Add(tenshi_packer
            SOURCE_DIR
                "${CMAKE_CURRENT_SOURCE_DIR}"
            BINARY_DIR
                "${TENSHI_PACKER_BUILD_DIR}"
            INSTALL_DIR
                "${TENSHI_PACKER_INSTALL_DIR}"
            CMAKE_ARGS
                "-DCMAKE_BUILD_TYPE=Release"
                "-DTENSHI_PACKED=ON"
                "-DTENSHI_BUILD_PACKER_ONLY=ON"
                "-DTENSHI_PAK_ENCRYPTION=${TENSHI_PAK_ENCRYPTION}"
                "-DTENSHI_PAK_KEY_SOURCE_DIR=${TENSHI_PAK_KEY_SOURCE_DIR}"
            BUILD_COMMAND
                "${CMAKE_COMMAND}"
                --build .
                --target packer_tool
                --config Release
            INSTALL_COMMAND
                ""
            BUILD_BYPRODUCTS
                "${TENSHI_PACKER_BUILD_DIR}/packer_tool${TENSHI_PACKER_SUFFIX}"
        )

        # this executable runs on the build host, not emscripten
        set(TENSHI_PACKER
            "${TENSHI_PACKER_BUILD_DIR}/packer_tool${TENSHI_PACKER_SUFFIX}"
        )

        add_custom_command(
            OUTPUT
                "${TENSHI_PAK_FILE}"
            COMMAND
                "${CMAKE_COMMAND}"
                -E
                make_directory
                "${CMAKE_BINARY_DIR}/game_data"
            COMMAND
                "${TENSHI_PACKER}"
                "${TENSHI_RESOURCE_DIR}"
                "${TENSHI_PAK_FILE}"
                "${TENSHI_PAK_MAX_SIZE}"
            DEPENDS
                tenshi_packer
                ${TENSHI_RESOURCE_FILES}
            COMMENT
                "Packing game resources"
            VERBATIM
        )
    endif()

    add_custom_target(pack_resources
        DEPENDS
            "${TENSHI_PAK_FILE}"
    )
endif()

if(BUILD_TESTING AND TENSHI_PACKED AND NOT EMSCRIPTEN)
    add_executable(tenshi_pak_archive_tests
        tests/pak_archive_tests.cpp
    )
    target_include_directories(tenshi_pak_archive_tests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(tenshi_pak_archive_tests PRIVATE tenshi::tenshi)
    add_dependencies(tenshi_pak_archive_tests pack_resources)
    add_test(
        NAME tenshi_pak_archive_tests
        COMMAND tenshi_pak_archive_tests "${TENSHI_PAK_FILE}"
    )
endif()

# ============================================================
#   Game resource setup
# ============================================================

function(tenshi_setup_game TARGET)
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR
            "tenshi_setup_game(): target '${TARGET}' does not exist"
        )
    endif()

    if(TENSHI_PACKED)
        # Tell game code that resources are packed
        target_compile_definitions(
            ${TARGET}
            PRIVATE
                TENSHI_PACKED
        )

        # ensure the archive exists before game is built
        add_dependencies(${TARGET} pack_resources)

        if(NOT EMSCRIPTEN)
            # copy generated chunks next to the game executable
            add_custom_command(
                TARGET 
                    ${TARGET}
                    POST_BUILD
                COMMAND
                    "${CMAKE_COMMAND}"
                    "-DSOURCE_DIR=${CMAKE_BINARY_DIR}/game_data"
                    "-DDESTINATION_DIR=$<TARGET_FILE_DIR:${TARGET}>"
                    -P
                    "${TENSHI_COPY_PAK_CHUNKS_SCRIPT}"
                VERBATIM
            )
        else()
            # put the packed directory (all chunks) into the virtual filesystem
            target_link_options(${TARGET}
                PRIVATE
                    "--preload-file"
                    "${CMAKE_BINARY_DIR}/game_data@/"
            )
        endif()
    else()
        # --------------------------------------------------------
        # Resource directory
        # --------------------------------------------------------

        if(NOT EMSCRIPTEN)
            add_custom_command(
                TARGET 
                    ${TARGET}
                    POST_BUILD
                COMMAND
                    "${CMAKE_COMMAND}"
                    -E
                    copy_directory
                    "${TENSHI_RESOURCE_DIR}"
                    "$<TARGET_FILE_DIR:${TARGET}>/resources"
                VERBATIM
            )
        endif()
    endif()
endfunction()

# ============================================================
# Installation
# ============================================================

if(TENSHI_INSTALL)
    include(CMakePackageConfigHelpers)

    set(TENSHI_INSTALL_TARGETS
        tenshi
        tenshi_sdl3
        miniaudio
        liblha
        fkyaml
        stb_image
        stb_truetype
        tenshi_pak_crypto
    )
    if(TARGET sodium)
        list(APPEND TENSHI_INSTALL_TARGETS sodium)
    endif()

    install(
        TARGETS ${TENSHI_INSTALL_TARGETS}
        EXPORT tenshiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Development
    )

    install(
        DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION
            "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT Development
    )

    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/miniaudio/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT Development
        FILES_MATCHING PATTERN "miniaudio.h"
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/liblha/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT Development
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/fkyaml/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT Development
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/stb_image/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT Development
        FILES_MATCHING PATTERN "*.h"
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/stb_truetype/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT Development
        FILES_MATCHING PATTERN "*.h"
    )
    if(TARGET sodium)
        install(
            DIRECTORY
                "${CMAKE_CURRENT_SOURCE_DIR}/${EXTERNAL_DIR}/libsodium-cmake/libsodium/src/libsodium/include/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT Development
            FILES_MATCHING PATTERN "*.h"
        )
    endif()

    install(
        EXPORT tenshiTargets
        FILE tenshiTargets.cmake
        NAMESPACE tenshi::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tenshi"
        COMPONENT Development
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tenshiConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tenshiConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tenshi"
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/tenshiConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tenshi"
        COMPONENT Development
    )
endif()