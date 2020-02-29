project(core)

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.*pp"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
add_target_compile_definitions(${PROJECT_NAME} LEVK)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
target_include_directories(${PROJECT_NAME}
	SYSTEM PUBLIC "${EXT_DIR}/fmt/include"
	SYSTEM PUBLIC "${EXT_DIR}/glm"
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		fmt
		glm
	PRIVATE
		physfs-static
		$<$<STREQUAL:${PLATFORM},Linux>:stdc++fs>
		$<$<STREQUAL:${PLATFORM},Linux>:pthread>
)
if(W_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
