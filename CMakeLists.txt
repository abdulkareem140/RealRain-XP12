cmake_minimum_required(VERSION 3.21)
project(RealRain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime so users don't need a VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(XPLANE_SDK_DIR "${CMAKE_SOURCE_DIR}/third_party/XPSDK" CACHE PATH "Path to the X-Plane SDK (folder containing CHeaders/ and Libraries/)")

if(NOT EXISTS "${XPLANE_SDK_DIR}/CHeaders/XPLM/XPLMPlugin.h")
    message(FATAL_ERROR
        "X-Plane SDK not found at ${XPLANE_SDK_DIR}.\n"
        "Run scripts/get_sdk.ps1, or download the SDK from "
        "https://developer.x-plane.com/sdk/plugin-sdk-downloads/ and extract it so that "
        "third_party/XPSDK/CHeaders exists. Or pass -DXPLANE_SDK_DIR=<path>.")
endif()

set(SOURCES
    src/main.cpp
    src/Log.cpp
    src/Config.cpp
    src/SimData.cpp
    src/DebugWindow.cpp
    src/StreakControl.cpp
    src/DatarefScan.cpp
    src/TestQuad.cpp
)

add_library(RealRain MODULE ${SOURCES})

# Embed config/RealRain.ini so the plugin can write it out on first run.
# (The ini is the user's file: SkunkCrafts Updater never ships or overwrites it.)
file(READ "${CMAKE_SOURCE_DIR}/config/RealRain.ini" REALRAIN_DEFAULT_INI)
configure_file("${CMAKE_SOURCE_DIR}/src/DefaultIni.h.in" "${CMAKE_BINARY_DIR}/generated/DefaultIni.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/config/RealRain.ini")

target_include_directories(RealRain PRIVATE
    src
    "${CMAKE_BINARY_DIR}/generated"
    "${XPLANE_SDK_DIR}/CHeaders/XPLM"
    "${XPLANE_SDK_DIR}/CHeaders/Widgets"
)

# Target the XP12 API (SDK 4.x). Each define unlocks that API level.
target_compile_definitions(RealRain PRIVATE
    XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM302=1 XPLM303=1 XPLM400=1 XPLM410=1
    REALRAIN_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
    set(XP_PLATFORM_DIR "win_x64")
    target_compile_definitions(RealRain PRIVATE IBM=1 WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(RealRain PRIVATE
        "${XPLANE_SDK_DIR}/Libraries/Win/XPLM_64.lib"
        opengl32
    )
    if(MSVC)
        target_compile_options(RealRain PRIVATE /W4 /utf-8 /permissive-)
    endif()
elseif(APPLE)
    set(XP_PLATFORM_DIR "mac_x64")
    target_compile_definitions(RealRain PRIVATE APL=1)
    find_library(OPENGL_FRAMEWORK OpenGL)
    target_link_libraries(RealRain PRIVATE "-F${XPLANE_SDK_DIR}/Libraries/Mac" "-framework XPLM" ${OPENGL_FRAMEWORK})
    set_target_properties(RealRain PROPERTIES OSX_ARCHITECTURES "x86_64;arm64")
else()
    set(XP_PLATFORM_DIR "lin_x64")
    target_compile_definitions(RealRain PRIVATE LIN=1)
    target_compile_options(RealRain PRIVATE -fvisibility=hidden -Wall -Wextra)
    target_link_libraries(RealRain PRIVATE GL)
endif()

# X-Plane 12 "fat plugin" layout:  <dist>/RealRain/win_x64/RealRain.xpl
# The $<1:...> generator expression stops multi-config generators (Visual Studio)
# from appending Debug/ or Release/ to the output directory.
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist/RealRain")
set_target_properties(RealRain PROPERTIES
    PREFIX ""
    SUFFIX ".xpl"
    OUTPUT_NAME "RealRain"
    LIBRARY_OUTPUT_DIRECTORY "$<1:${DIST_DIR}/${XP_PLATFORM_DIR}>"
    RUNTIME_OUTPUT_DIRECTORY "$<1:${DIST_DIR}/${XP_PLATFORM_DIR}>"
)

# The SkunkCrafts Updater config travels with the plugin; its version tracks the project version.
configure_file("${CMAKE_SOURCE_DIR}/config/skunkcrafts_updater.cfg.in" "${CMAKE_BINARY_DIR}/skunkcrafts_updater.cfg" @ONLY NEWLINE_STYLE LF)
add_custom_command(TARGET RealRain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/skunkcrafts_updater.cfg"
        "${DIST_DIR}/skunkcrafts_updater.cfg"
)
