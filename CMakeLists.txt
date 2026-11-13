cmake_minimum_required(VERSION 3.22)

project(SampleLab VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# macOS: build universale Intel + Apple Silicon
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Rubber Band (time-stretch / pitch-shift di alta qualita', motore R3). Si usa la build "single file" del progetto:
# nessuna dipendenza esterna (FFT interna, resampler interno). Licenza GPL: uso personale.
FetchContent_Declare(
    rubberband
    GIT_REPOSITORY https://github.com/breakfastquay/rubberband.git
    GIT_TAG        v4.0.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(rubberband)
add_library(rubberband_single STATIC ${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp)
target_include_directories(rubberband_single PUBLIC ${rubberband_SOURCE_DIR})
set_target_properties(rubberband_single PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(rubberband_single PRIVATE NOMINMAX _USE_MATH_DEFINES)

juce_add_plugin(SampleLab
    PRODUCT_NAME "Sample Lab"
    COMPANY_NAME "SampleLab"
    BUNDLE_ID "com.samplelab.samplelab"
    PLUGIN_MANUFACTURER_CODE SmpL
    PLUGIN_CODE SLab
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Sampler
)

# Codice del motore e del kit, condiviso tra plugin e test (compilato in entrambi i target)
set(SAMPLELAB_CORE_SOURCES
    Source/engine/SampleData.cpp
    Source/engine/LoopStretch.cpp
    Source/engine/SliceDetect.cpp
    Source/engine/Voice.cpp
    Source/engine/FxChain.cpp
    Source/params/ParamSet.cpp
    Source/params/MidiLearn.cpp
    Source/sequencer/SequencerExport.cpp
    Source/engine/Engine.cpp
    Source/kit/KitIO.cpp
)

target_sources(SampleLab PRIVATE
    ${SAMPLELAB_CORE_SOURCES}
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/PadGrid.cpp
    Source/ui/WaveformView.cpp
    Source/ui/KnobBox.cpp
    Source/ui/FxPanel.cpp
    Source/ui/SeqPanel.cpp
    Source/ui/ModulesBar.cpp
    Source/ui/WaveStrip.cpp
    Source/ui/CellPanel.cpp
    Source/ui/Widgets.cpp
    Source/ui/LookAndFeel.cpp
    Source/ui/SampleBrowser.cpp
    Source/ui/SlicePanel.cpp
)

target_include_directories(SampleLab PRIVATE Source)
target_link_libraries(SampleLab PRIVATE rubberband_single)

# Test automatici (motore + formato kit)
juce_add_console_app(SampleLabTests PRODUCT_NAME "SampleLabTests")
target_sources(SampleLabTests PRIVATE ${SAMPLELAB_CORE_SOURCES} Tests/Tests.cpp)
target_include_directories(SampleLabTests PRIVATE Source)
target_link_libraries(SampleLabTests PRIVATE rubberband_single)
if(MSVC)
    # Un solo grande runTest con molti oggetti Engine locali: serve piu' dello stack predefinito (1 MB)
    target_link_options(SampleLabTests PRIVATE /STACK:33554432)
endif()
target_compile_definitions(SampleLabTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SampleLabTests
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# ASIO (solo Windows): richiede l'SDK Steinberg in third_party/asiosdk (non incluso nel repository per
# via della licenza). Si scarica da https://www.steinberg.net/asiosdk e si estrae in quella cartella.
if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asiosdk/common/iasiodrv.h")
        message(STATUS "ASIO SDK trovato: driver ASIO abilitati")
        target_compile_definitions(SampleLab PUBLIC JUCE_ASIO=1)
        target_include_directories(SampleLab PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asiosdk/common")
    else()
        message(WARNING "ASIO SDK non trovato in third_party/asiosdk: lo standalone usera' solo WASAPI/DirectSound")
    endif()
endif()

target_compile_definitions(SampleLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(SampleLab
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
