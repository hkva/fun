# SDL
# https://www.libsdl.org/
add_library(sdl INTERFACE)
if(WIN32)
    # @@
elseif(UNIX AND NOT APPLE)
    # @@
elseif(APPLE)
    target_include_directories(sdl INTERFACE "/opt/homebrew/include/SDL2/")
    target_link_libraries(sdl INTERFACE "/opt/homebrew/lib/libSDL2.dylib")
    # target_link_libraries(sdl INTERFACE "-framework Foundation" "-framework Cocoa" "-framework CoreAudio")
    set(HAS_SDL TRUE)
endif()

# glad 3.3
# https://glad.dav1d.de/
add_library(glad33 STATIC "${CMAKE_CURRENT_LIST_DIR}/glad33/src/glad.c")
target_include_directories(glad33 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/glad33/include")
set(HAS_GLAD33 TRUE)