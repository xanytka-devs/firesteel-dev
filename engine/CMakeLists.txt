cmake_minimum_required(VERSION 3.12)
project(firesteel)
set(PRJ_NAME firesteel)

option(FS_INCLUDE_ASSIMP "Include Assimp libraries (GRAPHICS)" ON)
option(FS_INCLUDE_GLM "Include GLM library (UI)" ON)
option(FS_INCLUDE_FT "Include FreeType library (UI)" ON)
option(FS_INCLUDE_IMGUI "Include ImGui library (UI)" ON)
option(FS_INCLUDE_OCULUS "Include Oculus library (PLATFORM)" OFF)
option(FS_INCLUDE_OGL "Include OpenGL libraries (GRAPHICS)" ON)
option(FS_INCLUDE_OAL "Include OpenAL library (SOUND)" ON)
option(FS_INCLUDE_LUA "Include LuaBridge library (WRAPPER)" ON)
option(FS_INCLUDE_BULLET "Include Bullet3 library (PHYSICS)" ON)
option(FS_INCLUDE_BOX2D "Include Box2D library (PHYSICS)" ON)

file(GLOB_RECURSE ENGINE_INCLUDES "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "include/*.cpp")
if(FS_INCLUDE_FT)
	file(GLOB_RECURSE FT_INCLUDES "external/freetype/*.hpp external/freetype/*.h")
endif()
if(FS_INCLUDE_OCULUS)
	file(GLOB_RECURSE OCULUS_INCLUDES "external/oculus/*.h")
endif()
if(FS_INCLUDE_LUA)
	file(GLOB_RECURSE LUA_INCLUDES "external/lua/include/*.hpp external/lua/include/*.h")
endif()

set(PRJ_ALL
	${ENGINE_INCLUDES}
	${ENGINE_SOURCES}
	${FT_INCLUDES}
	${OCULUS_INCLUDES}
	${LUA_INCLUDES}
)

add_library(${PRJ_NAME}
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
	target_include_directories(${PRJ_NAME} PUBLIC external/glfw)
	
	add_subdirectory(external/glad ${CMAKE_CURRENT_BINARY_DIR}/glad)
	target_link_libraries(${PRJ_NAME} PRIVATE glad)
	set_target_properties(glad PROPERTIES FOLDER "External")
endif()

if(FS_INCLUDE_GLM)
	add_subdirectory(external/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)
	target_link_libraries(${PRJ_NAME} PRIVATE glm)
	set_target_properties(glm PROPERTIES FOLDER "External")
endif()

if(FS_INCLUDE_BOX2D)
	add_subdirectory(external/box2d ${CMAKE_CURRENT_BINARY_DIR}/box2d)
	target_link_libraries(${PRJ_NAME} PRIVATE box2d)
	set_target_properties(box2d PROPERTIES FOLDER "External")
endif()

if(FS_INCLUDE_BULLET)
	add_subdirectory(external/bullet3 ${CMAKE_CURRENT_BINARY_DIR}/bullet3)
	target_link_libraries(${PRJ_NAME} PRIVATE BulletDynamics)
	target_link_libraries(${PRJ_NAME} PRIVATE BulletCollision)
	target_link_libraries(${PRJ_NAME} PRIVATE LinearMath)
	
	set_target_properties(Bullet2FileLoader PROPERTIES FOLDER "External/bullet3")
	set_target_properties(Bullet3Collision PROPERTIES FOLDER "External/bullet3")
	set_target_properties(Bullet3Common PROPERTIES FOLDER "External/bullet3")
	set_target_properties(Bullet3Dynamics PROPERTIES FOLDER "External/bullet3")
	set_target_properties(Bullet3Geometry PROPERTIES FOLDER "External/bullet3")
	set_target_properties(Bullet3OpenCL_clew PROPERTIES FOLDER "External/bullet3")
	set_target_properties(BulletCollision PROPERTIES FOLDER "External/bullet3")
	set_target_properties(BulletDynamics PROPERTIES FOLDER "External/bullet3")
	set_target_properties(BulletInverseDynamics PROPERTIES FOLDER "External/bullet3")
	set_target_properties(BulletSoftBody PROPERTIES FOLDER "External/bullet3")
	set_target_properties(LinearMath PROPERTIES FOLDER "External/bullet3")
endif()

if(FS_INCLUDE_ASSIMP)
	add_subdirectory(external/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp)
	target_link_libraries(${PRJ_NAME} PRIVATE assimp)
	set_target_properties(assimp PROPERTIES FOLDER "External/assimp")
	set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "External/assimp")
	set_target_properties(zlibstatic PROPERTIES FOLDER "External/assimp")
endif()

if(FS_INCLUDE_FT)
	target_include_directories(${PRJ_NAME} PRIVATE "external\\freetype\\include" "external\\freetype\\")
	target_link_libraries(${PRJ_NAME} PUBLIC "engine\\freetype\\freetype")
endif()

if(FS_INCLUDE_OCULUS)
	target_include_directories(${PRJ_NAME} PRIVATE external/oculus)
	target_link_libraries(${PRJ_NAME} PRIVATE external/oculus/LibOVR)
endif()

if(FS_INCLUDE_IMGUI)
	add_subdirectory(external ${CMAKE_CURRENT_BINARY_DIR}/imgui)
	target_include_directories(imgui PUBLIC external/imgui)
	set_target_properties(imgui PROPERTIES FOLDER "External")
	
	target_include_directories(${PRJ_NAME} PUBLIC external/imgui)
endif()

if(FS_INCLUDE_OAL)
	add_subdirectory(external/openal-soft ${CMAKE_CURRENT_BINARY_DIR}/openal-soft)
	target_link_libraries(${PRJ_NAME} PRIVATE OpenAL)
	set_target_properties(OpenAL PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(openal-info PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(alsoft.build_version PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(alsoft.common PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(alsoft.excommon PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(alsoft.fmt PROPERTIES FOLDER "External/openal-soft")
	set_target_properties(clang-tidy-check PROPERTIES FOLDER "External/openal-soft")
endif()

if(FS_INCLUDE_LUA)
	target_link_libraries(${PRJ_NAME} PRIVATE lua54.lib)

	add_subdirectory(external/lua/LuaBridge ${CMAKE_CURRENT_BINARY_DIR}/LuaBridge)
	target_link_libraries(${PRJ_NAME} PRIVATE LuaBridge)
	set_target_properties(LuaBridge PROPERTIES FOLDER "External/Lua")
endif()