cmake_minimum_required(VERSION 3.25)

# Binario universale (Apple Silicon + Intel) su macOS
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(PSM3 VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PSM3_BUILD_PLUGIN "Compila il plugin JUCE (VST3, AU, Standalone)" ON)
option(PSM3_BUILD_DEVTOOLS "Compila gli strumenti di sviluppo JUCE (test del processore, istantanea della GUI)" OFF)
include(CTest)

# ---------------------------------------------------------------------------
# Motore DSP: C++17 puro, nessuna dipendenza da JUCE
file(GLOB PSM3_DSP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/*.cpp")
add_library(PSM3_DSP STATIC ${PSM3_DSP_SOURCES})
target_include_directories(PSM3_DSP PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set_target_properties(PSM3_DSP PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(NOT MSVC)
    target_compile_options(PSM3_DSP PRIVATE -Wall -Wextra)
endif()

# Test nativi del motore
if(BUILD_TESTING)
    file(GLOB PSM3_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/test_*.cpp")
    foreach(test_source IN LISTS PSM3_TESTS)
        get_filename_component(test_name "${test_source}" NAME_WE)
        add_executable(${test_name} "${test_source}")
        target_include_directories(${test_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Tests")
        target_link_libraries(${test_name} PRIVATE PSM3_DSP)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# ---------------------------------------------------------------------------
if(PSM3_BUILD_PLUGIN)
    # JUCE: usa una copia locale se indicata (-DJUCE_DIR=...), altrimenti la scarica
    set(JUCE_DIR "" CACHE PATH "Cartella di JUCE (opzionale)")
    if(JUCE_DIR)
        add_subdirectory(${JUCE_DIR} JUCE)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.15
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    # caratteri incorporati (TeX Gyre Adventor, GUST Font License)
    juce_add_binary_data(PSM3Assets SOURCES
        resources/fonts/texgyreadventor-regular.otf
        resources/fonts/texgyreadventor-bold.otf)

    set(PSM3_PLUGIN_SOURCES
        Source/Plugin/PluginProcessor.cpp
        Source/GUI/Widgets.cpp
        Source/GUI/PluginEditor.cpp)

    juce_add_plugin(PSM3
        COMPANY_NAME "Collateral Effects"
        BUNDLE_ID com.collateraleffects.psm3
        PLUGIN_MANUFACTURER_CODE Coef
        PLUGIN_CODE Psm3
        PRODUCT_NAME "PSM-3"
        DESCRIPTION "PSM-3 (PsychoMatrix-3): sintetizzatore a matrice"
        FORMATS VST3 AU Standalone
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        VST3_CATEGORIES Instrument Synth
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(PSM3 PRIVATE ${PSM3_PLUGIN_SOURCES})
    target_include_directories(PSM3 PRIVATE Source)
    target_compile_definitions(PSM3 PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(PSM3
        PRIVATE PSM3_DSP PSM3Assets juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

    if(PSM3_BUILD_DEVTOOLS)
        foreach(tool processor_test gui_snapshot)
            juce_add_console_app(${tool} PRODUCT_NAME "${tool}")
            target_sources(${tool} PRIVATE Tests/plugin/${tool}.cpp ${PSM3_PLUGIN_SOURCES})
            target_include_directories(${tool} PRIVATE Source)
            target_compile_definitions(${tool} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
                JucePlugin_Name="PSM-3" JUCE_STANDALONE_APPLICATION=1)
            target_link_libraries(${tool} PRIVATE PSM3_DSP PSM3Assets juce::juce_audio_utils juce::juce_recommended_config_flags)
        endforeach()
    endif()
endif()
