# MBStudio UI — kit grafico condiviso dei plugin MBStudio91.
# Uso nel CMakeLists.txt del plugin, dopo juce_add_plugin(...):
#   include(MBStudioUI/MBStudioUI.cmake)
#   mbstudio_ui_add(NomeTarget)
# poi nel codice:  #include "MBStudioLookAndFeel.h"
function(mbstudio_ui_add target)
    set(dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    juce_add_binary_data(${target}_MBStudioUIFonts
        NAMESPACE MBStudioUIFonts
        HEADER_NAME MBStudioUIFonts.h
        SOURCES
            "${dir}/fonts/BebasNeue-Regular.ttf"
            "${dir}/fonts/Antonio-Thin.ttf"
            "${dir}/fonts/Antonio-Light.ttf"
            "${dir}/fonts/Antonio-Bold.ttf"
            "${dir}/brand/mbstudio91-logo.png"
            "${dir}/brand/mbstudio91-wordmark.png")
    target_sources(${target} PRIVATE "${dir}/MBStudioLookAndFeel.cpp")
    target_include_directories(${target} PRIVATE "${dir}")
    target_link_libraries(${target} PRIVATE ${target}_MBStudioUIFonts)
endfunction()
