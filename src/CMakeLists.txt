project(levk)

# Executable
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.*pp"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "src" FILES ${SOURCES})
set(GENERATED_SOURCES
	"${CMAKE_CURRENT_BINARY_DIR}/generated/build_version.hpp"
)
configure_file_src_to_bin(build_version.hpp.in generated/build_version.hpp)
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${GENERATED_SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES} ${GENERATED_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set(EXE_NAME ${PROJECT_NAME}-$<CONFIG>${EXE_SUFFIX})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${EXE_NAME})
add_target_compile_definitions(${PROJECT_NAME} LEVK)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
add_target_link_options(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE 
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		glm
		core
	PRIVATE
		#$<$<BOOL:${LE3D_USE_GLFW}>:glfw>
		#stb-image
		#tinyobjloader
		physfs-static
		$<$<STREQUAL:${PLATFORM},Linux>:stdc++fs>

		## TODO: Remove later
		$<$<STREQUAL:${PLATFORM},Linux>:X11>
)
if(W_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
