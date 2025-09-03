cmake_minimum_required(VERSION 3.14)
project(game_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES src/*.cpp)
file(GLOB TESTSOURCES test*.cpp)
file(GLOB BOX2DSRC
        Box2D/src/collision/*.cpp
        Box2D/src/common/*.cpp
        Box2D/src/dynamics/*.cpp
        Box2D/src/rope/*.cpp
)

include_directories(
        glm
        rapidsjon
        SDL_Image
        SDL_mixer
        SDL_TTF
        SDL/include
        Lua
        LuaBridge
        Box2D/include
        Box2D/src
)

# Build Box2D as a shared library on all platforms and place the result in Box2D/
add_library(box2d SHARED ${BOX2DSRC})
set_target_properties(box2d PROPERTIES
        OUTPUT_NAME box2d
        POSITION_INDEPENDENT_CODE ON
)
if(WIN32)
    # Export all symbols automatically on Windows if Box2D sources don't define dllexport
    set_target_properties(box2d PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
target_include_directories(box2d PUBLIC
        ${CMAKE_SOURCE_DIR}/Box2D/include
        ${CMAKE_SOURCE_DIR}/Box2D/src
)
add_custom_command(TARGET box2d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/Box2D"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:box2d>" "${CMAKE_SOURCE_DIR}/Box2D"
        COMMENT "Placing Box2D shared library into ${CMAKE_SOURCE_DIR}/Box2D"
)

if(WIN32)
    # Use local DLLs and copy them post-build
    set(SDL2_DLL         "${CMAKE_SOURCE_DIR}/SDL/SDL2.dll")
    set(SDL2_IMAGE_DLL   "${CMAKE_SOURCE_DIR}/SDL_Image/SDL2_image.dll")
    set(SDL2_MIXER_DLL   "${CMAKE_SOURCE_DIR}/SDL_mixer/SDL2_mixer.dll")
    set(SDL2_TTF_DLL     "${CMAKE_SOURCE_DIR}/SDL_TTF/SDL2_ttf.dll")
    set(BOX2D_DLL        "${CMAKE_SOURCE_DIR}/Box2D/box2d.dll")
    set(PLATFORM_LIBS    SDL2 SDL2main SDL2_image SDL2_ttf SDL2_mixer lua box2d)
    link_directories(
            SDL
            SDL_Image
            SDL_mixer
            SDL_TTF
            Lua
            Box2D
    )

elseif(APPLE)
    # Assume frameworks are present in analogous 'lib' folders, like ./SDL/lib/SDL2.framework
    set(SDL2_FRAMEWORK_DIR       "${CMAKE_SOURCE_DIR}/SDL/SDL2.framework")
    set(SDL2_IMAGE_FRAMEWORK_DIR "${CMAKE_SOURCE_DIR}/SDL_Image/SDL2_image.framework")
    set(SDL2_MIXER_FRAMEWORK_DIR "${CMAKE_SOURCE_DIR}/SDL_mixer/SDL2_mixer.framework")
    set(SDL2_TTF_FRAMEWORK_DIR   "${CMAKE_SOURCE_DIR}/SDL_TTF/SDL2_ttf.framework")
    set(FRAMEWORK_SEARCH_PATHS
            "${CMAKE_SOURCE_DIR}/SDL"
            "${CMAKE_SOURCE_DIR}/SDL_Image"
            "${CMAKE_SOURCE_DIR}/SDL_mixer"
            "${CMAKE_SOURCE_DIR}/SDL_TTF"
    )
    foreach(fw_path IN LISTS FRAMEWORK_SEARCH_PATHS)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -F${fw_path}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${fw_path}")
    endforeach()

    set(PLATFORM_LIBS
            "-framework SDL2"
            "-framework SDL2_image"
            "-framework SDL2_mixer"
            "-framework SDL2_ttf"
            lua
    )
else()
    # Linux & others, system libraries
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2   REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(LUA    REQUIRED lua5.3)

    include_directories(
            ${SDL2_INCLUDE_DIRS}
            ${SDL2_IMAGE_INCLUDE_DIRS}
            ${SDL2_MIXER_INCLUDE_DIRS}
            ${SDL2_TTF_INCLUDE_DIRS}
            ${LUA_INCLUDE_DIRS}
    )
    set(PLATFORM_LIBS
            ${SDL2_LIBRARIES}
            ${SDL2_IMAGE_LIBRARIES}
            ${SDL2_MIXER_LIBRARIES}
            ${SDL2_TTF_LIBRARIES}
            ${LUA_LIBRARIES}
            box2d
    )
    # Link to our local Box2D shared lib
    link_directories(${CMAKE_SOURCE_DIR}/Box2D)
endif()

# ---- The actual executable ----

if(APPLE)
    # On macOS, compile Box2D sources with the main code directly
    add_executable(game_engine_linux ${SOURCES} ${BOX2DSRC})
else()
    add_executable(game_engine_linux ${SOURCES})
endif()
target_compile_options(game_engine_linux PRIVATE -O3 -DNDEBUG -Wall -pedantic)
target_link_libraries(game_engine_linux PRIVATE ${PLATFORM_LIBS})

# ---- POST-BUILD: Platform-specific resource copying ----

if(WIN32)
    add_custom_command(TARGET game_engine_linux POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL}"
            "${SDL2_IMAGE_DLL}"
            "${SDL2_MIXER_DLL}"
            "${SDL2_TTF_DLL}"
            "${BOX2D_DLL}"
            $<TARGET_FILE_DIR:game_engine_linux>
            COMMENT "Copying SDL and Box2D DLLs to output directory"
    )
endif()

if(APPLE)
    # Copy frameworks to next to executable (or to bundle if using APP)
    set(APP_OUTPUT_DIR "$<TARGET_FILE_DIR:game_engine_linux>")
    foreach(fw_dir IN ITEMS
            ${SDL2_FRAMEWORK_DIR}
            ${SDL2_IMAGE_FRAMEWORK_DIR}
            ${SDL2_MIXER_FRAMEWORK_DIR}
            ${SDL2_TTF_FRAMEWORK_DIR})
        add_custom_command(TARGET game_engine_linux POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${fw_dir}" "${APP_OUTPUT_DIR}/$(basename ${fw_dir})"
                COMMENT "Copying framework: ${fw_dir}"
        )
    endforeach()
endif()

# ---- Additional targets (debug, valgrind, etc.) use the same logic as above,
#      just repeat the logic for each additional target as needed ----