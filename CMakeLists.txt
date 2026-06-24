cmake_minimum_required(VERSION 3.14)
project(game_engine C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES src/*.cpp)
file(GLOB TESTSOURCES test*.cpp)
# Configure Box2D before adding it
set(BOX2D_BUILD_SHARED ON CACHE BOOL "Build Box2D shared libraries" FORCE)
set(BOX2D_BUILD_EXAMPLES OFF CACHE BOOL "Build Box2D examples" FORCE)
add_subdirectory(Box2D)
add_subdirectory(Lua)

include_directories(
        glm/glm
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



if(WIN32)
    # Use local DLLs and copy them post-build
    set(SDL2_DLL         "${CMAKE_SOURCE_DIR}/SDL/SDL2.dll")
    set(SDL2_IMAGE_DLL   "${CMAKE_SOURCE_DIR}/SDL_Image/SDL2_image.dll")
    set(SDL2_MIXER_DLL   "${CMAKE_SOURCE_DIR}/SDL_mixer/SDL2_mixer.dll")
    set(SDL2_TTF_DLL     "${CMAKE_SOURCE_DIR}/SDL_TTF/SDL2_ttf.dll")
    set(BOX2D_DLL        "$<TARGET_FILE:box2d>")

    link_directories(
            "${CMAKE_SOURCE_DIR}/SDL"
            "${CMAKE_SOURCE_DIR}/SDL_Image"
            "${CMAKE_SOURCE_DIR}/SDL_mixer"
            "${CMAKE_SOURCE_DIR}/SDL_TTF"
    )

    set(PLATFORM_LIBS    SDL2 SDL2main SDL2_image SDL2_ttf SDL2_mixer lua box2d)

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
    )
endif()

# ---- The actual executable ----

if(APPLE)
    # On macOS, compile Box2D sources with the main code directly
    add_executable(game_engine ${SOURCES} ${BOX2DSRC})
else()
    add_executable(game_engine ${SOURCES})
endif()
add_dependencies(game_engine box2d lua)
target_compile_options(game_engine PRIVATE -O3 -DNDEBUG -Wall -pedantic)
target_link_libraries(game_engine PRIVATE ${PLATFORM_LIBS})

# ---- POST-BUILD: Platform-specific resource copying ----

if(WIN32)
    add_custom_command(TARGET game_engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL}"
            "${SDL2_IMAGE_DLL}"
            "${SDL2_MIXER_DLL}"
            "${SDL2_TTF_DLL}"
            "${BOX2D_DLL}"
            $<TARGET_FILE_DIR:game_engine>
            COMMENT "Copying SDL and Box2D DLLs to output directory"
    )
endif()

if(APPLE)
    # Copy frameworks to next to executable (or to bundle if using APP)
    set(APP_OUTPUT_DIR "$<TARGET_FILE_DIR:game_engine>")
    foreach(fw_dir IN ITEMS
            ${SDL2_FRAMEWORK_DIR}
            ${SDL2_IMAGE_FRAMEWORK_DIR}
            ${SDL2_MIXER_FRAMEWORK_DIR}
            ${SDL2_TTF_FRAMEWORK_DIR})
        add_custom_command(TARGET game_engine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${fw_dir}" "${APP_OUTPUT_DIR}/$(basename ${fw_dir})"
                COMMENT "Copying framework: ${fw_dir}"
        )
    endforeach()
endif()

# ---- Additional targets (debug, valgrind, etc.) use the same logic as above,
#      just repeat the logic for each additional target as needed ----