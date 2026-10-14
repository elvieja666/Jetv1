cmake_minimum_required(VERSION 3.22)
project(Jet VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# En Mac: binario universal (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(JET_FORMATS AU VST3)
else()
    set(JET_FORMATS VST3)
endif()

juce_add_plugin(Jet
    COMPANY_NAME "0600"
    PLUGIN_MANUFACTURER_CODE Ox06
    PLUGIN_CODE JetF
    FORMATS ${JET_FORMATS}
    PRODUCT_NAME "Jet"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(Jet PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Jet PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Jet
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Prueba offline (solo para desarrollo, no se instala) ---
option(JET_BUILD_TEST "Build offline render test" OFF)
if(JET_BUILD_TEST)
    juce_add_console_app(JetRender PRODUCT_NAME "JetRender")
    target_sources(JetRender PRIVATE test/RenderTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(JetRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_Name="Jet" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0 JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0 JucePlugin_EditorRequiresKeyboardFocus=0 JucePlugin_ManufacturerCode=0x4f783036 JucePlugin_PluginCode=0x4a657446)
    target_link_libraries(JetRender PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats
        juce::juce_gui_basics PUBLIC juce::juce_recommended_config_flags)
endif()
