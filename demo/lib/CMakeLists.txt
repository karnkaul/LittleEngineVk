project(levk-demo-lib)

# Executable
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME} PUBLIC
	levk-graphics
	levk-window
	levk-core
	djson
	$<$<STREQUAL:${PLATFORM},Android>:levk-android-glue>
	levk-interface
)
if(LEVK_USE_PCH)
	target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-core)
endif()
