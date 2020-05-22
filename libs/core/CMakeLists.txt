project(levk-core)

# Library
glob_sources(SOURCES "*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
# -fPIC for fmt
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
if(MSVC_RUNTIME AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	target_compile_definitions(fmt PUBLIC FMT_EXCEPTIONS=0)
endif()
# glm defines
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SILENT_WARNINGS GLM_ENABLE_EXPERIMENTAL GLM_EXT_INCLUDED)

add_target_compile_definitions(${PROJECT_NAME} LEVK PUBLIC)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
target_include_directories(${PROJECT_NAME}
	SYSTEM PUBLIC
		"${EXT_DIR}/fmt/include"
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		fmt
		glm
	PRIVATE
		physfs-static
		$<$<STREQUAL:${PLATFORM},Linux>:pthread>
		$<$<STREQUAL:${PLATFORM},Linux>:stdc++fs>
)

