project(levk-core)

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(levk::core ALIAS ${PROJECT_NAME})

# defines
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_XYZW_ONLY GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SILENT_WARNINGS GLM_ENABLE_EXPERIMENTAL GLM_EXT_INCLUDED)
target_compile_definitions(${PROJECT_NAME} PUBLIC KTL_FIXED_ANY_THROW)
target_include_directories(${PROJECT_NAME}
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		glm
		ktl::ktl
		dlog::dlog
		dtasks::dtasks
		decf::decf
		djson::djson
		levk::interface
	PRIVATE
		levk::compile-options
		physfs-static
)

# PCH
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	set(PRECOMPILE
		<deque>
		<string>
		<unordered_map>
		<vector>
	)
	target_precompile_headers(${PROJECT_NAME} PRIVATE ${PRECOMPILE})
endif()
