project(levk-interface)

# Interface library target
set(COMPILE_DEFS
	_UNICODE
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG LEVK_RELEASE>
	$<$<CONFIG:Debug>:LEVK_DEBUG>
	$<$<BOOL:${MSVC_RUNTIME}>:WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS>
	# LEVK Flags
	$<$<STREQUAL:${PLATFORM},Android>:LEVK_ANDROID>
	$<$<BOOL:${LEVK_PRE_RELEASE}>:LEVK_PRE_RELEASE>
)
set(CLANG_COMMON -Wconversion -Wunreachable-code -Wdeprecated-declarations -Wtype-limits -Wunused -Wno-unknown-pragmas -Wno-deprecated-volatile)
if(LINUX_GCC OR LINUX_CLANG OR WINDOWS_GCC OR WINDOWS_CLANG)
	set(COMPILE_OPTS
		-Wextra
		-Werror=return-type
		$<$<NOT:$<CONFIG:Debug>>:-Werror>
		$<$<NOT:$<BOOL:${WINDOWS_CLANG}>>:-fexceptions>
		$<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${LINUX_CLANG}>>:-Wall>
		$<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WINDOWS_GCC}>>:-utf-8 -Wno-unknown-pragmas -Wno-volatile>
		$<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WINDOWS_CLANG}>>:${CLANG_COMMON}>
		$<$<BOOL:${LEVK_ASAN}>:-O1 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls>
	)
elseif(WINDOWS_MSBUILD)
	set(COMPILE_OPTS
		$<$<NOT:$<CONFIG:Debug>>:/Oi /WX>
		/MP
	)
endif()
if(WINDOWS_CLANG AND MSVC)
	list(APPEND COMPILE_OPTS /W4 -utf-8)
endif()
if(LINUX_CLANG AND NOT PLATFORM STREQUAL Android AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "9.0.0")
	# Ignore Vulkan.hpp warnings
	message(STATUS "clang++ ${CMAKE_CXX_COMPILER_VERSION} detected; adding -Wno-deprecated-copy for vulkan.hpp")
	list(APPEND COMPILE_OPTS -Wno-deprecated-copy)
endif()
if(LINUX_GCC AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "10.0.0")
	# Ignore Vulkan.hpp warnings
	message(STATUS "g++ ${CMAKE_CXX_COMPILER_VERSION} detected; adding -Wno-deprecated-copy -Wno-class-memaccess -Wno-init-list-lifetime for vulkan.hpp")
	list(APPEND COMPILE_OPTS -Wno-deprecated-copy -Wno-class-memaccess -Wno-init-list-lifetime)
endif()

set(COMPILE_FTRS cxx_std_20)
set(LINK_LIBS $<$<STREQUAL:${PLATFORM},Linux>:pthread stdc++fs dl>)

# library interface
add_library(${PROJECT_NAME} INTERFACE)
add_library(levk::interface ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} INTERFACE ${COMPILE_FTRS})
target_compile_definitions(${PROJECT_NAME} INTERFACE ${COMPILE_DEFS})
target_compile_options(${PROJECT_NAME} INTERFACE ${COMPILE_OPTS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${LINK_LIBS})

if(PLATFORM STREQUAL "Linux")
	set(LINK_OPTS
		-Wl,-z,origin   # Allow $ORIGIN in RUNPATH
		$<$<BOOL:${LEVK_ASAN}>:-fsanitize=address>
	)
elseif(PLATFORM STREQUAL "Win64" AND NOT WINDOWS_GCC)
	if(MSVC)
		set(LINK_OPTS
			/ENTRY:mainCRTStartup                              # Link to main() and not WinMain()
			/SUBSYSTEM:$<IF:$<CONFIG:Debug>,CONSOLE,WINDOWS>   # Spawn a console in Debug
		)
	else()
		set(LINK_OPTS -Wl,/SUBSYSTEM:$<IF:$<CONFIG:Debug>,CONSOLE,WINDOWS>,/ENTRY:mainCRTStartup)
	endif()
endif()

# executable interface
add_library(${PROJECT_NAME}-exe INTERFACE)
add_library(levk::interface::exe ALIAS ${PROJECT_NAME}-exe)
target_link_options(${PROJECT_NAME}-exe INTERFACE ${LINK_OPTS})
target_link_libraries(${PROJECT_NAME}-exe INTERFACE levk::interface)
