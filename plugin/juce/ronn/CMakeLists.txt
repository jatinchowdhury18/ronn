cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(ronn VERSION 1.0.0)

# Using RTNeural with XSimd backend
if(APPLE)
    set(RTNEURAL_ACCELERATE ON CACHE BOOL "Use RTNeural with Accelerate backend" FORCE)
else()
    set(RTNEURAL_XSIMD ON CACHE BOOL "Use RTNeural with XSimd backend" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(juce)
add_subdirectory(RTNeural)
include_directories(RTNeural)

juce_add_plugin(ronn
    COMPANY_NAME "Christian Steinmetz"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Sko4
    FORMATS AU VST3 Standalone
    ProductName "ronn"
    MICROPHONE_PERMISSION_ENABLED TRUE
)

juce_generate_juce_header(ronn)

target_sources(ronn PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ronnlib.cpp
)

target_compile_definitions(ronn PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ronn PUBLIC
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    # juce::juce_recommended_warning_flags
    RTNeural
)
