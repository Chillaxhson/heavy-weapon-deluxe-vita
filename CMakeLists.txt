cmake_minimum_required(VERSION 3.20)
project(HeavyWeaponDeluxe LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES
    src/main.cpp
    src/platform/Vfs.cpp
    src/platform/AudioSystem.cpp
    src/platform/InputManager.cpp
    src/loader/XmlLoader.cpp
    src/render/TextureManager.cpp
    src/render/Renderer.cpp
    src/render/FontRenderer.cpp
    src/render/WorldRenderer.cpp
    src/core/GameEngine.cpp
    src/game/Gfx.cpp
    src/game/Board.cpp
    src/game/Crafts.cpp
    src/game/Hazards.cpp
    src/game/Supplies.cpp
    src/game/Weapons.cpp
    src/game/Bosses.cpp
)

# ---------------------------------------------------------------------------
# Desktop build (Linux): same game code on SDL2 + legacy OpenGL, for fast iteration
# and screenshots. See tools/desktop/hw.sh.
# ---------------------------------------------------------------------------
if(NOT DEFINED ENV{VITASDK} AND NOT DEFINED VITASDK)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2 SDL2_image SDL2_mixer)
    find_package(OpenGL REQUIRED)

    add_executable(heavyweapon ${SOURCES} 3rdparty/tinyxml2/tinyxml2.cpp)
    target_include_directories(heavyweapon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml2)
    target_compile_definitions(heavyweapon PRIVATE GL_GLEXT_PROTOTYPES)
    target_compile_options(heavyweapon PRIVATE -Wall -Wextra)
    target_link_libraries(heavyweapon PRIVATE PkgConfig::SDL2 OpenGL::GL)
    return()
endif()

# ---------------------------------------------------------------------------
# PS Vita build
# ---------------------------------------------------------------------------
if(NOT DEFINED VITASDK)
    set(VITASDK $ENV{VITASDK})
endif()
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Heavy Weapon Deluxe")
set(VITA_TITLEID "HWDL00001")
set(VITA_VERSION "01.00")

add_executable(heavyweapon ${SOURCES})

target_compile_definitions(heavyweapon PRIVATE
    __vita__
)

target_link_libraries(heavyweapon
    -Wl,--start-group
    SDL2_image
    SDL2_mixer
    SDL2
    vitaGL
    vitashark
    mathneon
    SceShaccCgExt
    SceShaccCg_stub
    taihen_stub
    SceKernelDmacMgr_stub
    vorbisfile
    vorbis
    ogg
    modplug
    opusfile
    opus
    mpg123
    xmp
    png
    jpeg
    webpdemux
    webp
    tinyxml2
    z
    m
    pthread
    SceLibKernel_stub
    SceDisplay_stub
    SceGxm_stub
    SceCtrl_stub
    SceTouch_stub
    SceAudio_stub
    SceAudioIn_stub
    SceMotion_stub
    SceHid_stub
    SceIme_stub
    ScePower_stub
    SceAppMgr_stub
    SceSysmodule_stub
    SceCommonDialog_stub
    -Wl,--end-group
)

# The signed SELF must not share the .velf name: vita.cmake writes the intermediate
# VELF to heavyweapon.velf, and a parallel build could then pack the unsigned file.
vita_create_self(heavyweapon.self heavyweapon UNSAFE)

vita_create_vpk(HeavyWeaponDeluxe.vpk ${VITA_TITLEID} heavyweapon.self
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
