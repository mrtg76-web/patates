cmake_minimum_required(VERSION 3.22)
project(ORGU VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ORGU_BUILD_PLUGIN "Build the JUCE plugin" ON)
option(ORGU_BUILD_TESTS "Build the headless core tests" ON)
set(ORGU_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (fetched automatically when empty)")

# ---------------------------------------------------------------- Plaits DSP (MIT, Emilie Gillet)
file(GLOB_RECURSE PLAITS_SOURCES CONFIGURE_DEPENDS
    third_party/eurorack/plaits/*.cc
    third_party/eurorack/stmlib/*.cc)
add_library(plaits_dsp STATIC ${PLAITS_SOURCES})
target_include_directories(plaits_dsp PUBLIC third_party/eurorack)
# TEST selects the portable (non-ARM) code paths of stmlib
target_compile_definitions(plaits_dsp PUBLIC TEST)
if(MSVC)
    target_compile_options(plaits_dsp PRIVATE /W0)
else()
    target_compile_options(plaits_dsp PRIVATE -w -O3)
endif()

# ---------------------------------------------------------------- core (framework independent)
add_library(orgu_core STATIC core/OrguSynth.cpp)
target_include_directories(orgu_core PUBLIC core)
target_link_libraries(orgu_core PUBLIC plaits_dsp)

if(ORGU_BUILD_TESTS)
    enable_testing()
    add_executable(orgu_tests tests/test_core.cpp)
    target_link_libraries(orgu_tests PRIVATE orgu_core)
    add_test(NAME orgu_tests COMMAND orgu_tests)
endif()

# ---------------------------------------------------------------- plugin
if(ORGU_BUILD_PLUGIN)
    if(ORGU_JUCE_DIR)
        add_subdirectory(${ORGU_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
        FetchContent_MakeAvailable(JUCE)
    endif()

    if(APPLE)
        set(ORGU_FORMATS VST3 AU Standalone)
    elseif(WIN32)
        set(ORGU_FORMATS VST3 Standalone)
    else()
        set(ORGU_FORMATS VST3)
    endif()

    juce_add_plugin(ORGU
        COMPANY_NAME "Orgu"
        BUNDLE_ID "com.orgu.macrooscillator"
        PLUGIN_MANUFACTURER_CODE Orgu
        PLUGIN_CODE Org1
        FORMATS ${ORGU_FORMATS}
        PRODUCT_NAME "Orgu"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Synth
        AU_MAIN_TYPE kAudioUnitType_MusicDevice)

    target_sources(ORGU PRIVATE
        plugin/PluginProcessor.cpp
        plugin/PluginEditor.cpp)

    target_compile_definitions(ORGU PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    if(UNIX AND NOT APPLE)
        target_compile_definitions(ORGU PUBLIC JUCE_ALSA=0 JUCE_JACK=0)
    endif()

    target_link_libraries(ORGU
        PRIVATE
            orgu_core
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_gui_basics
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    # Headless test of the real processor + editor (Linux CI / development only)
    option(ORGU_BUILD_PLUGIN_TEST "Build the processor smoke test" OFF)
    if(ORGU_BUILD_PLUGIN_TEST)
        juce_add_console_app(orgu_plugin_test PRODUCT_NAME "orgu_plugin_test")
        target_sources(orgu_plugin_test PRIVATE tests/test_plugin.cpp plugin/PluginProcessor.cpp plugin/PluginEditor.cpp)
        target_compile_definitions(orgu_plugin_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Orgu"
            JucePlugin_IsSynth=1 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
        if(UNIX AND NOT APPLE)
            target_compile_definitions(orgu_plugin_test PRIVATE JUCE_ALSA=0 JUCE_JACK=0)
        endif()
        target_link_libraries(orgu_plugin_test PRIVATE orgu_core juce::juce_audio_processors
            juce::juce_gui_basics juce::juce_recommended_config_flags)
    endif()
endif()
