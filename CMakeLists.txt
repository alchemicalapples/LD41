cmake_minimum_required(VERSION 3.5)
project(LD41)

add_subdirectory(ext/ginseng)
add_subdirectory(ext/lua)
add_subdirectory(ext/sol2)
add_subdirectory(ext/soloud)
add_subdirectory(ext/metastuff)
add_subdirectory(ext/glm)
add_subdirectory(ext/lodepng)
add_subdirectory(ext/sushi)
add_subdirectory(ext/msdfgen)

set(LD41_CXX_STANDARD 17)

add_custom_target(ld41)

if(EMSCRIPTEN)
    set(LD41_WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH "Client Output Directory")

    # Client Static
    file(GLOB_RECURSE LD41_CLIENT_STATIC static/*)
    add_custom_target(ld41_client_static
        COMMENT "Copying client static files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/static "${LD41_WWW_DIR}"
        SOURCES ${LD41_CLIENT_STATIC})

    # Client Shell
    set(LD41_CLIENT_SHELL "${CMAKE_SOURCE_DIR}/src/shell.html" CACHE PATH "Emscripten Shell")

    # Client JS
    file(GLOB_RECURSE LD41_CLIENT_JS "src/*.js")

    # Client Data
    set(LD41_CLIENT_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Directory")
    file(GLOB_RECURSE LD41_CLIENT_DATA_FILES "${LD41_CLIENT_DATA_DIR}/*")

    # Client C++
    file(GLOB_RECURSE LD41_CLIENT_SRCS src/*.cpp src/*.hpp)
    add_executable(ld41_client ${LD41_CLIENT_SRCS} ${LD41_CLIENT_JS} ${LD41_CLIENT_SHELL})
    string(CONCAT LD41_CLIENT_LINK_FLAGS
        " -s TOTAL_MEMORY=33554432"
        " -s DISABLE_EXCEPTION_CATCHING=0"
        " -s USE_SDL=2"
        " -s USE_SDL_IMAGE=2"
        " -s USE_SDL_NET=2"
        " -s USE_FREETYPE=1"
        " --preload-file ${LD41_CLIENT_DATA_DIR}@data"
        " --shell-file ${LD41_CLIENT_SHELL}")
    string(CONCAT LD41_CLIENT_LINK_FLAGS_DEBUG
        " -g4"
        " --cpuprofiler"
        " --memoryprofiler")
    set_target_properties(ld41_client PROPERTIES
        CXX_STANDARD ${LD41_CXX_STANDARD}
        SUFFIX .html
        LINK_FLAGS "${LD41_CLIENT_LINK_FLAGS}"
        LINK_FLAGS_DEBUG "${LD41_CLIENT_LINK_FLAGS_DEBUG}"
        LINK_DEPENDS "${LD41_CLIENT_SHELL};${LD41_CLIENT_DATA_FILES}"
        RUNTIME_OUTPUT_DIRECTORY "${LD41_WWW_DIR}")
    em_link_js_library(ld41_client ${LD41_CLIENT_JS})
    target_link_libraries(ld41_client
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud)
    add_dependencies(ld41_client ld41_client_static)

    add_dependencies(ld41 ld41_client)
endif()
