cmake_minimum_required(VERSION 3.6)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-template-body>)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

project(game LANGUAGES C CXX)

set(OPENGL_VERSION "4.3")
add_compile_definitions($<$<CONFIG:Debug>:RLGL_ENABLE_OPENGL_DEBUG_CONTEXT=1>)

add_subdirectory(src/lib/raylib)
add_subdirectory(src/lib/imgui)
add_subdirectory(src/lib/rlImGui)

file(GLOB_RECURSE SYSTEM_SOURCES "src/system/*.cpp")
file(GLOB_RECURSE GAME_SOURCES "src/game/*.cpp")

add_executable(game 
main.cpp 
${SYSTEM_SOURCES}
${GAME_SOURCES}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-template-body)
endif()

target_include_directories(game PRIVATE 
		src/include
		src/lib/raygui
    src/include/gameobjects
		src/lib
		src/lib/lua/include
		src/lib/sol
)

add_definitions(-DHAS_GETADDRINFO)
target_link_libraries(game PRIVATE 
		raylib 
		imgui 
		rlImGui
		${CMAKE_SOURCE_DIR}/src/lib/enet/libenet.a
		${CMAKE_SOURCE_DIR}/src/lib/lua/liblua54.a
		winmm ws2_32
)

target_compile_options(game PRIVATE -g)

target_compile_definitions(game PRIVATE
	MAP_SOURCE_DIR="${CMAKE_SOURCE_DIR}/gamedata/maps"
	VS_PATH="${CMAKE_SOURCE_DIR}/gamedata/shaders/330/lighting.vert"
	FS_PATH="${CMAKE_SOURCE_DIR}/gamedata/shaders/330/lighting.frag"
	PP_VS_PATH="${CMAKE_SOURCE_DIR}/gamedata/shaders/330/postprocess.vert"
	BLOOM_EXTRACT_FS_PATH="${CMAKE_SOURCE_DIR}/gamedata/shaders/330/bloom_extract.frag"
	BLUR_FS_PATH="${CMAKE_SOURCE_DIR}/gamedata/shaders/330/blur.frag"
	COMPOSITE_FS_PATH="${CMAKE_SOURCE_DIR}/gamedata/shaders/330/composite.frag")