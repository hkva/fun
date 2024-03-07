# SDL
# https://www.libsdl.org/
add_library(sdl INTERFACE)
if(WIN32)
    target_include_directories(sdl INTERFACE "${CMAKE_CURRENT_LIST_DIR}/SDL2-2.30.0-win64/include")
    target_link_libraries(sdl INTERFACE "${CMAKE_CURRENT_LIST_DIR}/SDL2-2.30.0-win64/lib/x64/SDL2.lib")
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/SDL2-2.30.0-win64/lib/x64/SDL2.dll"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COPYONLY
    )
    set(HAS_SDL TRUE)
elseif(UNIX AND NOT APPLE)
    find_package(SDL2 REQUIRED)
    include_directories(sdl INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(sdl INTERFACE ${SDL2_LIBRARIES} m)
    set(HAS_SDL TRUE)
elseif(APPLE)
    target_include_directories(sdl INTERFACE "/opt/homebrew/include/SDL2/")
    target_link_libraries(sdl INTERFACE "/opt/homebrew/lib/libSDL2.dylib")
    set(HAS_SDL TRUE)
endif()

# glad 3.3
# https://glad.dav1d.de/
add_library(glad33 STATIC "${CMAKE_CURRENT_LIST_DIR}/glad33/src/glad.c")
target_include_directories(glad33 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/glad33/include")

# Handmade math
# https://github.com/HandmadeMath/HandmadeMath
add_library(handmade-math INTERFACE)
target_include_directories(handmade-math INTERFACE "${CMAKE_CURRENT_LIST_DIR}/HandmadeMath-2.0.0")