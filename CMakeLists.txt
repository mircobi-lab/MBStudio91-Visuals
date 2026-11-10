cmake_minimum_required(VERSION 3.22)

# MBStudio91 Visuals — plugin di visualizzazione audio-reattiva (VST3 / AU / Standalone)
project(MBStudio91Visuals VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    # Binario universale: Mac Intel (es. MacBook Pro 2019) e Apple Silicon
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    endif()
endif()

# JUCE: cartella locale (-DJUCE_DIR=...) oppure download automatico della versione fissata
set(JUCE_DIR "" CACHE PATH "Percorso di una copia locale di JUCE")
if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.5
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(MBV_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND MBV_FORMATS AU)
endif()

juce_add_plugin(MBStudio91Visuals
    COMPANY_NAME "MBStudio91"
    COMPANY_WEBSITE "https://mbstudio91.fun"
    BUNDLE_ID "fun.mbstudio91.visuals"
    PRODUCT_NAME "MBStudio91 Visuals"
    VERSION ${PROJECT_VERSION}
    PLUGIN_MANUFACTURER_CODE Mbst
    PLUGIN_CODE Mbvx
    FORMATS ${MBV_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer
    AU_MAIN_TYPE kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "MBStudio91 Visuals ascolta l'ingresso audio (es. BlackHole) per far reagire i visual alla musica."
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS com.apple.security.device.audio-input)

file(GLOB MBV_PRESETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Presets/*.glsl")
juce_add_binary_data(MBVPresets SOURCES ${MBV_PRESETS})

target_sources(MBStudio91Visuals PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Params.cpp
    Source/AudioFeed.cpp
    Source/VisualEngine.cpp
    Source/ShaderLibrary.cpp
    Source/Renderer.cpp
    Source/StageComponent.cpp
    Source/ProjectorWindow.cpp
    Source/ShaderEditorWindow.cpp)

if(TARGET MBStudio91Visuals_Standalone)
    target_sources(MBStudio91Visuals_Standalone PRIVATE Source/StandaloneApp.cpp)
    target_compile_definitions(MBStudio91Visuals_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
endif()

target_compile_definitions(MBStudio91Visuals PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_JACK=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(MBStudio91Visuals
    PRIVATE
        MBVPresets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
