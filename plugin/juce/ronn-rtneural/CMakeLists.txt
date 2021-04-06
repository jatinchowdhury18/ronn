cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(ronn VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(modules)

# juce_set_vst2_sdk_path(C:/SDKs/VST_SDK/VST2_SDK/)

juce_add_plugin(ronn
    COMPANY_NAME Christian Steinmetz
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Sko4
    FORMATS AU VST3 Standalone #VST
    ProductName "ronn")

juce_generate_juce_header(ronn)
add_subdirectory(src)
include_directories(src)

target_compile_definitions(ronn PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ronn PRIVATE juce_plugin_modules)
