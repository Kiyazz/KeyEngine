file(GLOB LUASRC sources/*.c)
add_library(lua STATIC ${LUASRC})
set_target_properties(lua PROPERTIES
    OUTPUT_NAME lua
    POSITION_INDEPENDENT_CODE ON
    LINKER_LANGUAGE C)

if (WIN32)
    set_target_properties(lua PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
