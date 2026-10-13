cmake_minimum_required(VERSION 3.22)
project(ClearMix VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # utile pour l'autocomplétion VS Code (clangd / cpptools)

option(CLEARMIX_BUILD_TESTS "Compiler les tests unitaires du moteur de masquage" ON)
option(CLEARMIX_BUILD_SMOKE_TEST "Compiler le test de bout en bout du processeur (plus long à compiler)" OFF)
# Pour utiliser une copie locale de JUCE (hors-ligne) : cmake -DCLEARMIX_JUCE_DIR=/chemin/vers/JUCE ..
set(CLEARMIX_JUCE_DIR "" CACHE PATH "Dossier JUCE local (optionnel)")

# ------------------------------------------------------------------ JUCE
if(CLEARMIX_JUCE_DIR)
    add_subdirectory(${CLEARMIX_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ------------------------------------------------------------------ Plugin
set(CLEARMIX_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND CLEARMIX_FORMATS AU)
endif()

juce_add_plugin(ClearMix
    COMPANY_NAME               "ClearMix Audio"
    BUNDLE_ID                  com.clearmixaudio.clearmix
    PLUGIN_MANUFACTURER_CODE   Clmx
    PLUGIN_CODE                Cmx1
    FORMATS                    ${CLEARMIX_FORMATS}
    PRODUCT_NAME               "ClearMix"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    VST3_CATEGORIES            Fx EQ
    AU_MAIN_TYPE               kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD    FALSE)

target_sources(ClearMix PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SpectrumAnalyzer.cpp
    Source/MaskingEngine.cpp
    Source/SharedBus.cpp)

target_compile_definitions(ClearMix PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ClearMix
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ------------------------------------------------------------------ Tests (sans JUCE)
if(CLEARMIX_BUILD_TESTS)
    enable_testing()
    add_executable(ClearMixTests tests/MaskingEngineTests.cpp Source/MaskingEngine.cpp Source/SharedBus.cpp)
    target_include_directories(ClearMixTests PRIVATE Source)
    add_test(NAME MaskingEngine COMMAND ClearMixTests)
endif()

# ------------------------------------------------------------------ Test de bout en bout (avec JUCE)
if(CLEARMIX_BUILD_SMOKE_TEST)
    juce_add_console_app(ClearMixSmokeTest PRODUCT_NAME "ClearMixSmokeTest")
    target_sources(ClearMixSmokeTest PRIVATE
        tests/ProcessorSmokeTest.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/SpectrumAnalyzer.cpp
        Source/MaskingEngine.cpp Source/SharedBus.cpp)
    target_include_directories(ClearMixSmokeTest PRIVATE Source)
    target_compile_definitions(ClearMixSmokeTest PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(ClearMixSmokeTest PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME ProcessorSmoke COMMAND ClearMixSmokeTest)
endif()
