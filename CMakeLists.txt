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
    find_package(PythonInterp 2 REQUIRED)

    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "Python 2 not found.")
    endif()

    set(LD41_WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH "Client Output Directory")

    # Client Static
    file(GLOB_RECURSE LD41_CLIENT_STATIC static/*)
    add_custom_target(ld41_client_static
        COMMENT "Copying client static files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/static "${LD41_WWW_DIR}"
        SOURCES ${LD41_CLIENT_STATIC})

    # Client JS
    file(GLOB_RECURSE LD41_CLIENT_JS "src/*.js")

    # Client Data
    set(LD41_CLIENT_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Directory")
    file(GLOB_RECURSE LD41_CLIENT_DATA_FILES ${LD41_CLIENT_DATA_DIR}/*)
    set(FILE_PACKAGER $ENV{EMSCRIPTEN}/tools/file_packager.py)
    set(LD41_DATA_FILE ${LD41_WWW_DIR}/ld41_client.data)
    set(LD41_DATA_LOADER ${LD41_WWW_DIR}/ld41_client.data.js)
    add_custom_command(
        OUTPUT ${LD41_DATA_FILE} ${LD41_DATA_LOADER}
        COMMAND ${PYTHON_EXECUTABLE} ${FILE_PACKAGER} ${LD41_DATA_FILE} --preload ${LD41_CLIENT_DATA_DIR}@data --js-output=${LD41_DATA_LOADER}
        COMMENT "Packaging data files"
        DEPENDS ${LD41_CLIENT_DATA_FILES})
    add_custom_target(ld41_data
        SOURCES ${LD41_CLIENT_DATA_FILES}
        DEPENDS ${LD41_DATA_FILE} ${LD41_DATA_LOADER})

    # Client C++
    file(GLOB_RECURSE LD41_CLIENT_SRCS src/*.cpp src/*.hpp)
    add_executable(ld41_client ${LD41_CLIENT_SRCS} ${LD41_CLIENT_JS})
    string(CONCAT LD41_CLIENT_LINK_FLAGS
        " -s TOTAL_MEMORY=33554432"
        " -s DISABLE_EXCEPTION_CATCHING=0"
        " -s ASSERTIONS=1"
        " -s FORCE_FILESYSTEM=1"
        " -s USE_SDL=2"
        " -s USE_SDL_IMAGE=2"
        " -s USE_SDL_NET=2"
        " -s USE_FREETYPE=1")
    option(LD41_USE_WASM "Use WASM" ON)
    if (LD41_USE_WASM)
        string(PREPEND LD41_CLIENT_LINK_FLAGS
            " -s BINARYEN=1"
            " -s \"BINARYEN_TRAP_MODE='js'\"")
    endif()
    string(CONCAT LD41_CLIENT_LINK_FLAGS_DEBUG
        " -g4"
        " --cpuprofiler"
        " --memoryprofiler")
    string(CONCAT LD41_CLIENT_LINK_FLAGS_RELWITHDEBINFO
        " --cpuprofiler"
        " --memoryprofiler")
    set_target_properties(ld41_client PROPERTIES
        CXX_STANDARD ${LD41_CXX_STANDARD}
        SUFFIX .js
        LINK_FLAGS "${LD41_CLIENT_LINK_FLAGS}"
        LINK_FLAGS_DEBUG "${LD41_CLIENT_LINK_FLAGS_DEBUG}"
        LINK_FLAGS_RELWITHDEBINFO "${LD41_CLIENT_LINK_FLAGS_RELWITHDEBINFO}"
        RUNTIME_OUTPUT_DIRECTORY "${LD41_WWW_DIR}")
    target_compile_definitions(ld41_client PUBLIC
        SOL_CHECK_ARGUMENTS
        SOL_PRINT_ERRORS)
    em_link_js_library(ld41_client ${LD41_CLIENT_JS})
    target_link_libraries(ld41_client
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud)
    add_dependencies(ld41_client ld41_client_static ld41_data)

    add_dependencies(ld41 ld41_client)
else()
    find_package(sdl2 REQUIRED)

    add_subdirectory(ext/glad)

    set(LD41_DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Client Output Directory")

    # Client Data
    set(LD41_CLIENT_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Directory")
    file(GLOB_RECURSE LD41_CLIENT_DATA_FILES ${LD41_CLIENT_DATA_DIR}/*)
    add_custom_target(ld41_data
        COMMENT "Packaging data files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${LD41_CLIENT_DATA_DIR} ${LD41_DIST_DIR}/data
        SOURCES ${LD41_CLIENT_DATA_FILES})

    # Emberjs Shim
    file(GLOB_RECURSE EMBERJS_SHIM_SRCS emberjs_shim_src/*.cpp emberjs_shim_src/*.hpp)
    add_library(emberjs_shim ${EMBERJS_SHIM_SRCS})
    set_target_properties(emberjs_shim PROPERTIES CXX_STANDARD 17)

    # Client C++
    file(GLOB_RECURSE LD41_CLIENT_SRCS src/*.cpp src/*.hpp)
    add_executable(ld41_client ${LD41_CLIENT_SRCS} ${LD41_CLIENT_JS})
    set_target_properties(ld41_client PROPERTIES
        CXX_STANDARD ${LD41_CXX_STANDARD}
        LINK_FLAGS "-static"
        RUNTIME_OUTPUT_DIRECTORY "${LD41_DIST_DIR}")
    target_compile_definitions(ld41_client PUBLIC
        GLM_ENABLE_EXPERIMENTAL
        SOL_CHECK_ARGUMENTS
        SOL_PRINT_ERRORS)
    target_include_directories(ld41_client PRIVATE
        ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ld41_client
        emberjs_shim
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud
        ${SDL2_LIBRARIES}
        glad
        png16
        z)
    if (WIN32)
        target_link_libraries(ld41_client
            ole32
            oleaut32
            imm32
            winmm
            version)
    endif()
    add_dependencies(ld41_client ld41_data)

    add_dependencies(ld41 ld41_client)
endif()
