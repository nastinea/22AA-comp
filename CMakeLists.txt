cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) and macOS 10.13+ by default
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(22AA_COMP VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE checkout with -DJUCE_DIR=/path/to/JUCE, otherwise download it.
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(22AAComp
    PRODUCT_NAME "22AA comp"
    COMPANY_NAME "nastinea"
    BUNDLE_ID "com.nastinea.22aacomp"
    PLUGIN_MANUFACTURER_CODE Nsta
    PLUGIN_CODE A22c
    FORMATS VST3
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(22AAComp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(22AAComp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(22AAComp
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional: small tool that renders the UI to a PNG (used for checks, not shipped)
option(AA22_BUILD_SNAPSHOT "Build the UI snapshot tool" OFF)
if(AA22_BUILD_SNAPSHOT)
    juce_add_console_app(UISnapshot PRODUCT_NAME "UISnapshot")
    target_sources(UISnapshot PRIVATE tools/UISnapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(UISnapshot PRIVATE
        JucePlugin_Name="22AA comp" JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(UISnapshot PRIVATE juce::juce_audio_utils juce::juce_gui_extra
        PUBLIC juce::juce_recommended_config_flags)
endif()
