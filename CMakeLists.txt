cmake_minimum_required(VERSION 3.12)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "FixFlags.cmake")
project(fse)
set(EDITOR_PROJECT_NAME fse)
set(SCENE_VIEWER_PROJECT_NAME fsSceneViewer)
add_subdirectory(engine)

option(FS_BUILD_EDITOR "Build FSE (Firesteel Editor)" ON)
option(FS_BUILD_SCENE_VIEWER "Build Firesteel Scene Viewer (FSE without editing properties)" ON)
option(FS_BUILD_EXAMPLES "Build Firesteel examples (2d_ex, 3d_ex)" ON)

file(GLOB_RECURSE EDITOR_INCLUDES "include/*.hpp")
file(GLOB_RECURSE FT_INCLUDES "engine/external/freetype/*.hpp engine/external/freetype/*.h")
file(GLOB_RECURSE FT_LIB "engine/external/freetype/freetype.lib")

set(EDITOR_ALL
	"main.cpp"
	${EDITOR_INCLUDES}
	${FT_INCLUDES}
	${FREETYPE_INCLUDE_DIRS}
)

if(FS_BUILD_EDITOR)
	add_executable(${EDITOR_PROJECT_NAME}
		${EDITOR_ALL}
		"build/app.rc"
		"engine/external/IconsFontAwesome6.h"
		"engine/external/imgui_markdown.h"
		"engine/external/imgui-knobs.cpp"
		"engine/external/imgui-knobs.h"
		"engine/external/ImGuizmo.cpp"
		"engine/external/ImGuizmo.h"
	)
	target_link_libraries(${EDITOR_PROJECT_NAME}
		firesteel
		glad
		imgui
		glm
		assimp
		OpenAL
		LuaBridge
	)
	target_compile_features(${EDITOR_PROJECT_NAME} PUBLIC cxx_std_17)
	target_include_directories(${EDITOR_PROJECT_NAME} PUBLIC includes)
	target_include_directories(${EDITOR_PROJECT_NAME} PUBLIC engine/external/glfw)
	set_target_properties(${EDITOR_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
endif()
if(FS_BUILD_SCENE_VIEWER)
	add_executable(${SCENE_VIEWER_PROJECT_NAME}
		"include/atmosphere.hpp"
		"include/editor_object.hpp"
		"include/embeded.hpp"
		"include/light.hpp"
		"include/material.hpp"
		"include/scene.hpp"
		"include/utils.hpp"
		"include/imgui/base.hpp"
		"scene_viewer.cpp"
	)
	target_link_libraries(${SCENE_VIEWER_PROJECT_NAME}
		firesteel
		glad
		imgui
		glm
		assimp
		OpenAL
	)
	target_compile_features(${SCENE_VIEWER_PROJECT_NAME} PUBLIC cxx_std_17)
	target_include_directories(${SCENE_VIEWER_PROJECT_NAME} PUBLIC includes)
	set_target_properties(${SCENE_VIEWER_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
endif()
if(FS_BUILD_EXAMPLES)
	add_executable(fs2DExample
		"include/utils.hpp"
		"2d_ex.cpp"
		"build/example.rc"
	)
	target_link_libraries(fs2DExample
		firesteel
		glad
		glm
		assimp
		box2d
	)
	target_compile_features(fs2DExample PUBLIC cxx_std_17)
	target_include_directories(fs2DExample PUBLIC includes)
	set_target_properties(fs2DExample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
	
	add_executable(fs3DExample
		"include/atmosphere.hpp"
		"include/editor_object.hpp"
		"include/embeded.hpp"
		"include/light.hpp"
		"include/material.hpp"
		"include/scene.hpp"
		"include/utils.hpp"
		"include/imgui/base.hpp"
		"3d_ex.cpp"
		"build/example.rc"
	)
	target_link_libraries(fs3DExample
		firesteel
		glad
		imgui
		glm
		assimp
		OpenAL
		LuaBridge
	)
	target_compile_features(fs3DExample PUBLIC cxx_std_17)
	target_include_directories(fs3DExample PUBLIC includes)
	set_target_properties(fs3DExample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
	
	add_executable(fsAudioExample
		"include/utils.hpp"
		"include/imgui/base.hpp"
		"audio_ex.cpp"
		"build/example.rc"
	)
	target_link_libraries(fsAudioExample
		firesteel
		glad
		ImGui
		glm
		OpenAL
	)
	target_compile_features(fsAudioExample PUBLIC cxx_std_17)
	target_include_directories(fsAudioExample PUBLIC includes)
	set_target_properties(fsAudioExample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
endif()
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_PROJECT_NAME})