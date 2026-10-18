cmake_minimum_required(VERSION 3.22)
project(ASAS_HW3 VERSION 1.0.0 LANGUAGES C CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "JUCE is missing. Run: git submodule update --init --recursive")
endif()
add_subdirectory(JUCE)

set(plugin_formats VST3 Standalone)
if(APPLE)
    list(APPEND plugin_formats AU)
endif()

juce_add_plugin(ASAS_HW3
    COMPANY_NAME "ASAS"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Asas
    PLUGIN_CODE Shpd
    FORMATS ${plugin_formats}
    VST3_CATEGORIES Instrument Synth
    PRODUCT_NAME "ASAS Shepard Keyboard")

target_sources(ASAS_HW3 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ShepardVoice.h)
target_compile_features(ASAS_HW3 PUBLIC cxx_std_17)
target_compile_definitions(ASAS_HW3 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ASAS_HW3 PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Renders the actual processor to WAV without opening an audio device.
option(ASAS_BUILD_RENDERER "Build the offline demo renderer" ON)
if(ASAS_BUILD_RENDERER)
    juce_add_console_app(ShepardRender PRODUCT_NAME "ShepardRender")
    target_sources(ShepardRender PRIVATE tools/RenderDemo.cpp)
    target_link_libraries(ShepardRender PRIVATE ASAS_HW3 juce::juce_audio_utils)
endif()
