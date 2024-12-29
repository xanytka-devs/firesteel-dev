cmake_minimum_required(VERSION 3.12)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "FixFlags.cmake")
project(fse)
set(EDITOR_PROJECT_NAME fse)
add_subdirectory(engine)

file(GLOB_RECURSE EDITOR_INCLUDES "include/*.hpp")
file(GLOB_RECURSE FT_INCLUDES "engine/external/freetype/*.hpp engine/external/freetype/*.h")
file(GLOB_RECURSE FT_LIB "engine/external/freetype/freetype.lib")

set(EDITOR_ALL
	"main.cpp"
	${EDITOR_INCLUDES}
	${FT_INCLUDES}
	${FREETYPE_INCLUDE_DIRS}
)

add_executable(${EDITOR_PROJECT_NAME}
	${EDITOR_ALL}
	"build/app.rc"
)

target_link_libraries(${EDITOR_PROJECT_NAME}
	firesteel
	glad
	ImGui
	glm
	assimp
	OpenAL
	LuaBridge
)
target_compile_features(${EDITOR_PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${EDITOR_PROJECT_NAME} PUBLIC includes)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_PROJECT_NAME})
set_target_properties(${EDITOR_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)