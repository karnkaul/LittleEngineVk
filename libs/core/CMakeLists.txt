project(core)

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.*pp"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(glm INTERFACE GLM_FORCE_RADIANS)
add_target_compile_definitions(${PROJECT_NAME} LEVK PUBLIC)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
if(PLATFORM STREQUAL "Linux")
	target_compile_options(fmt PUBLIC -fPIC)
endif()
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
if(W_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
