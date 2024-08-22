cmake_minimum_required(VERSION 3.12)
project(LearningOpenGL)
set(PRJ_NAME LearningOpenGL)

file(GLOB_RECURSE ENGINE_INCLUDES "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")
file(GLOB_RECURSE FT_INCLUDES "external/freetype/*.hpp external/freetype/*.h")

set(PRJ_ALL
	${ENGINE_INCLUDES}
	${ENGINE_SOURCES}
	${FT_INCLUDES}
)

add_executable(${PRJ_NAME}
	${PRJ_ALL}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES
	${PRJ_ALL}
)

target_include_directories(${PRJ_NAME} PUBLIC includes)
target_include_directories(${PRJ_NAME} PUBLIC stb)
target_include_directories(${PRJ_NAME} PRIVATE src)
target_compile_features(${PRJ_NAME} PUBLIC cxx_std_17)

add_subdirectory(external/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
target_link_libraries(${PRJ_NAME} PRIVATE glfw)

add_subdirectory(external/glad ${CMAKE_CURRENT_BINARY_DIR}/glad)
target_link_libraries(${PRJ_NAME} PRIVATE glad)

add_subdirectory(external/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)
target_link_libraries(${PRJ_NAME} PRIVATE glm)

add_subdirectory(external/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp)
target_link_libraries(${PRJ_NAME} PRIVATE assimp)

find_package(Freetype REQUIRED)
target_include_directories(${PRJ_NAME} PRIVATE external/freetype)
target_link_libraries(${PRJ_NAME} PRIVATE "G:/! New/! EXPERIMENTS/RE_OGL/external/freetype/freetype.lib")

set_target_properties(assimp PROPERTIES FOLDER "External")
set_target_properties(glad PROPERTIES FOLDER "External")
set_target_properties(glfw PROPERTIES FOLDER "External")
set_target_properties(glm PROPERTIES FOLDER "External")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "External")
set_target_properties(zlibstatic PROPERTIES FOLDER "External")