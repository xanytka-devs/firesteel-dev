cmake_minimum_required(VERSION 3.12)
project(FiresteelDev)
set(PRJ_NAME FiresteelDev)

option(FS_INCLUDE_ASSIMP "Include Assimp libraries (G)" ON)
option(FS_INCLUDE_GLM "Include GLM library (U)" ON)
option(FS_INCLUDE_FT "Include FreeType library (U)" ON)
option(FS_INCLUDE_IMGUI "Include ImGui library (U)" ON)
option(FS_INCLUDE_OCULUS "Include Oculus library (P)" ON)
option(FS_INCLUDE_OGL "Include OpenGL libraries (G)" ON)
option(FS_INCLUDE_OAL "Include OpenAL library (S)" ON)

file(GLOB_RECURSE ENGINE_INCLUDES "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")
if(FS_INCLUDE_FT)
	file(GLOB_RECURSE FT_INCLUDES "external/freetype/*.hpp external/freetype/*.h")
endif()
if(FS_INCLUDE_OCULUS)
	file(GLOB_RECURSE OCULUS_INCLUDES "external/oculus/*.h")
endif()

set(PRJ_ALL
	${ENGINE_INCLUDES}
	${ENGINE_SOURCES}
	${FT_INCLUDES}
	${OCULUS_INCLUDES}
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

if(FS_INCLUDE_OGL)
	add_subdirectory(external/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
	target_link_libraries(${PRJ_NAME} PRIVATE glfw)
	set_target_properties(glfw PROPERTIES FOLDER "External/GLFW3")
	set_target_properties(uninstall PROPERTIES FOLDER "External/GLFW3")
	set_target_properties(update_mappings PROPERTIES FOLDER "External/GLFW3")
	
	add_subdirectory(external/glad ${CMAKE_CURRENT_BINARY_DIR}/glad)
	target_link_libraries(${PRJ_NAME} PRIVATE glad)
	set_target_properties(glad PROPERTIES FOLDER "External")
endif()

if(FS_INCLUDE_GLM)
	add_subdirectory(external/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)
	target_link_libraries(${PRJ_NAME} PRIVATE glm)
	set_target_properties(glm PROPERTIES FOLDER "External")
endif()

if(FS_INCLUDE_ASSIMP)
	add_subdirectory(external/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp)
	target_link_libraries(${PRJ_NAME} PRIVATE assimp)
	set_target_properties(assimp PROPERTIES FOLDER "External/assimp")
	set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "External/assimp")
	set_target_properties(zlibstatic PROPERTIES FOLDER "External/assimp")
endif()

if(FS_INCLUDE_FT)
	find_package(Freetype REQUIRED)
	target_include_directories(${PRJ_NAME} PRIVATE external/freetype)
	target_link_libraries(${PRJ_NAME} PRIVATE ../external/freetype/freetype)
endif()

if(FS_INCLUDE_OCULUS)
	target_include_directories(${PRJ_NAME} PRIVATE external/oculus)
	target_link_libraries(${PRJ_NAME} PRIVATE ../external/oculus/LibOVR)
endif()

if(FS_INCLUDE_IMGUI)
	add_subdirectory(external/imgui ${CMAKE_CURRENT_BINARY_DIR}/imgui)
	target_link_libraries(${PRJ_NAME} PRIVATE ImGui)
	set_target_properties(ImGui PROPERTIES FOLDER "External")
endif()

if(FS_INCLUDE_OAL)
	add_subdirectory(external/openal-soft ${CMAKE_CURRENT_BINARY_DIR}/openal-soft)
	target_link_libraries(${PRJ_NAME} PRIVATE OpenAL)
	set_target_properties(OpenAL PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(openal-info PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(alcommon PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(al-excommon PROPERTIES FOLDER "External/openal-soft")
endif()