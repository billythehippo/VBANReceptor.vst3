cmake_minimum_required(VERSION 3.15)

set(ARCHIVE_NAME "JuceLibraryCode.zip")
set(TARGET_DIR_NAME "JuceLibraryCode")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ARCHIVE_NAME}")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_DIR_NAME}")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_DIR_NAME}")
    endif()
    message(STATUS "Extracting ${ARCHIVE_NAME} into ${TARGET_DIR_NAME}...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_CURRENT_SOURCE_DIR}/${ARCHIVE_NAME}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_DIR_NAME}"
    )
endif()

set(PLUGIN_NAME "VBANReceptor")
set(CHANNELS 2 CACHE STRING "Number of audio channels")
project(${PLUGIN_NAME}_Project VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(JUCE CONFIG REQUIRED)
set(PRODUCT_NAME_STR "${PLUGIN_NAME}_${CHANNELS}ch")
set(LV2_URI_STR "https://github.com/billythehippo/{PLUGIN_NAME}-${CHANNELS}")
string(LENGTH "${CHANNELS}" CHAN_LEN)
if(CHAN_LEN EQUAL 1)
    set(PLUGIN_CODE_STR "V00${CHANNELS}")
else()
    set(PLUGIN_CODE_STR "V0${CHANNELS}")
endif()

juce_add_plugin(${PLUGIN_NAME}
    COMPANY_NAME "Hippo's Music Box"
    BUNDLE_ID "com.hipposmusicbox.${PLUGIN_NAME}"
    PRODUCT_NAME "${PRODUCT_NAME_STR}"
    PLUGIN_CODE "${PLUGIN_CODE_STR}"
    LV2_URI "${LV2_URI_STR}"
    FORMATS VST3 LV2 AU
    COPY_PLUGIN_AFTER_BUILD TRUE
)
juce_generate_juce_header(${PLUGIN_NAME})

set(GEN_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_config")
file(MAKE_DIRECTORY "${GEN_CONFIG_DIR}")
set(CONFIG_FILE "${GEN_CONFIG_DIR}/ChannelsConfig.h")

set(NEW_CONTENT "#pragma once\n#define NUMBER_OF_CHANNELS ${CHANNELS}\n")
if(EXISTS "${CONFIG_FILE}")
    file(READ "${CONFIG_FILE}" OLD_CONTENT)
endif()
if(NOT "${NEW_CONTENT}" STREQUAL "${OLD_CONTENT}")
    file(WRITE "${CONFIG_FILE}" "${NEW_CONTENT}")
endif()

target_include_directories(${PLUGIN_NAME} SYSTEM PRIVATE "${GEN_CONFIG_DIR}")
target_sources(${PLUGIN_NAME} PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/vban.h
    Source/zita-resampler/resampler-table.cc
    Source/zita-resampler/resampler-table.h
    Source/zita-resampler/vresampler.cc
    Source/zita-resampler/vresampler.h
)
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    target_include_directories(${PLUGIN_NAME} SYSTEM PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${GTK3_LIBRARIES})

    set_target_properties(${PLUGIN_NAME} PROPERTIES
        JUCE_VST3_COPY_DIR "$ENV{HOME}/.vst3"
        JUCE_LV2_COPY_DIR "$ENV{HOME}/.lv2"
    )
elseif(APPLE)
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        JUCE_VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        JUCE_LV2_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/LV2"
        JUCE_AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
    )
elseif(WIN32)
    file(TO_CMAKE_PATH "$ENV{CommonProgramFiles}" COMMON_FILES)
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        # VST3 на Windows по стандарту всегда здесь
        JUCE_VST3_COPY_DIR "${COMMON_FILES}/VST3"
        # LV2 на Windows менее стандартизирован, но чаще всего кладут рядом
        JUCE_LV2_COPY_DIR "${COMMON_FILES}/LV2"
    )
    target_compile_definitions(${PLUGIN_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# --- 8. ФЛАГИ И ЛИНКОВКА ---
target_compile_definitions(${PLUGIN_NAME} PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JucePlugin_LV2URI="${LV2_URI_STR}"
    JUCE_SILENCE_XCODE_DEPRECATION_WARNINGS=1
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# target_precompile_headers(${PLUGIN_NAME} PRIVATE <JuceHeader.h>)
