cmake_minimum_required(VERSION 3.24)
project(RuSwitcherNative LANGUAGES CXX)

set(RUSWITCHER_VERSION "0.10.0-beta.2" CACHE STRING "Display version embedded in RuSwitcher.exe")
set(RUSWITCHER_VERSION_NUMERIC "0.10.0.2" CACHE STRING "Four-part numeric Windows version")

if(NOT RUSWITCHER_VERSION_NUMERIC MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "RUSWITCHER_VERSION_NUMERIC must contain four numeric components")
endif()

string(REPLACE "." "," RUSWITCHER_VERSION_COMMA "${RUSWITCHER_VERSION_NUMERIC}")
if(RUSWITCHER_VERSION MATCHES "-")
    set(RUSWITCHER_FILE_FLAGS "VS_FF_PRERELEASE")
else()
    set(RUSWITCHER_FILE_FLAGS "0x0L")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/RuSwitcher.ico")
    set(RUSWITCHER_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/RuSwitcher.ico")
else()
    set(RUSWITCHER_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src/RuSwitcher.Win/Assets/RuSwitcher.ico")
endif()
configure_file(src/resources.rc.in generated/resources.rc @ONLY)

add_executable(RuSwitcherNative WIN32
    src/main.cpp
    src/engine.cpp
    src/clipboard.cpp
    src/diagnostics.cpp
    src/dict.cpp
    src/input_safety.cpp
    src/settings.cpp
    src/settings_dialog.cpp
    src/text_fixes.cpp
    src/tray.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated/resources.rc
)

target_compile_features(RuSwitcherNative PRIVATE cxx_std_20)
target_compile_definitions(RuSwitcherNative PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(RuSwitcherNative PRIVATE user32 shell32 advapi32 ole32 comctl32 urlmon uiautomationcore)

if(MSVC)
    target_compile_options(RuSwitcherNative PRIVATE /O1 /Os /GL /Gy /Gw /utf-8 /permissive- /W4)
    target_link_options(RuSwitcherNative PRIVATE /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO)
endif()

set_target_properties(RuSwitcherNative PROPERTIES
    OUTPUT_NAME RuSwitcher
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

# Native Unit Tests
enable_testing()
add_executable(RuSwitcherNativeTests
    tests/test_main.cpp
    src/clipboard.cpp
    src/dict.cpp
    src/text_fixes.cpp
)
target_compile_features(RuSwitcherNativeTests PRIVATE cxx_std_20)
target_compile_definitions(RuSwitcherNativeTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(RuSwitcherNativeTests PRIVATE src)
target_link_libraries(RuSwitcherNativeTests PRIVATE user32 ole32 urlmon)
if(MSVC)
    target_compile_options(RuSwitcherNativeTests PRIVATE /utf-8 /permissive- /W4)
    set_target_properties(RuSwitcherNativeTests PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

add_test(NAME PureLogicTests COMMAND RuSwitcherNativeTests)
