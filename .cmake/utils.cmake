macro(get_git_commit_hash OUTPUT_VAR)
	set(${OUTPUT_VAR} "[unknown]")
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE ${OUTPUT_VAR}
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endmacro()

macro(glob_sources OUTPUT_VAR PATTERN)
	set(${OUTPUT_VAR} "")
	file(GLOB_RECURSE ${OUTPUT_VAR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${PATTERN}")
endmacro()

function(update_git_submodules REQUIRED)
	message(STATUS "Updating git submodules...")
	execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE UPDATE_SUBMODULES_RESULT
	)
	if(NOT UPDATE_SUBMODULES_RESULT EQUAL "0")
		if(REQUIRED)
			message(FATAL_ERROR "git submodule update failed!")
		else()
			message(WARNING "git submodule update failed!")
		endif()
	endif()
endfunction()

function(unzip_archive ARCHIVE_NAME SUBDIR)
	if(NOT EXISTS "${SUBDIR}/${ARCHIVE_NAME}")
		message(FATAL_ERROR "Required archvives missing!\n${SUBDIR}/${ARCHIVE_NAME}")
	endif()
	message(STATUS "Extracting ${ARCHIVE_NAME}...")
	execute_process(COMMAND 
		${CMAKE_COMMAND} -E tar -xf "${ARCHIVE_NAME}"
		WORKING_DIRECTORY "${SUBDIR}"
	)
endfunction()

function(configure_file_src_to_bin SRC DEST)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
		set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/${DEST}")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${SRC}" "${OUTFILE}")
		list(APPEND SOURCES "${OUTFILE}")
		source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${OUTFILE}")
	else()
		message(WARNING "Required file not present to configure: ${SRC}")
	endif()
endfunction()

function(set_output_directory TARGET_NAME DIRECTORY_PATH)
	set_target_properties(${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${DIRECTORY_PATH}")
	set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${DIRECTORY_PATH}")
	set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${DIRECTORY_PATH}")
	foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${CONFIG} CONFIG)
		set_target_properties(${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} "${DIRECTORY_PATH}")
		set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${DIRECTORY_PATH}")
		set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${DIRECTORY_PATH}")
	endforeach()
endfunction()

function(add_target_compile_definitions TARGET_NAME PREFIX SCOPE)
	set(MSVC_FLAGS WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(${TARGET_NAME} ${SCOPE}
		_UNICODE
		$<$<NOT:$<CONFIG:Debug>>:
			NDEBUG
			${PREFIX}_RELEASE
		>
		$<$<CONFIG:Debug>:
			${PREFIX}_DEBUG
		>
		$<$<BOOL:${MSVC_RUNTIME}>:${MSVC_FLAGS}>
	)
endfunction()

function(add_target_compile_options TARGET_NAME SCOPE)
	set(CLANG_COMMON -Wconversion -Wunreachable-code -Wdeprecated-declarations -Wtype-limits -Wunused)
	if(LINUX_GCC OR LINUX_CLANG OR WIN64_GCC OR WIN64_CLANG)
		set(FLAGS
			-Wextra
			-Werror=return-type
			$<$<NOT:$<CONFIG:Debug>>:-Werror>
			$<$<NOT:$<BOOL:${WIN64_CLANG}>>:-fexceptions>
			$<$<BOOL:${WIN64_CLANG}>:/W4>
			$<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${LINUX_CLANG}>>:-Wall>
			$<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WIN64_GCC}>,$<BOOL:${WIN64_CLANG}>>:-utf-8>
			$<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WIN64_CLANG}>>:${CLANG_COMMON}>
		)
	elseif(WIN64_MSBUILD)
		set(FLAGS
			$<$<NOT:$<CONFIG:Debug>>:
				/O2
				/Oi
				/Ot
			>
			/MP
		)
	endif()
	if(PLATFORM STREQUAL "Linux")
		list(APPEND FLAGS -fPIC)
	endif()
	target_compile_options(${TARGET_NAME} ${SCOPE} ${FLAGS})
endfunction()

function(add_target_link_options TARGET_NAME)
	if(PLATFORM STREQUAL "Linux")
		target_link_options(${TARGET_NAME} PRIVATE
			-no-pie         # Build as application
			-Wl,-z,origin   # Allow $ORIGIN in RUNPATH
		)
	elseif(PLATFORM STREQUAL "Win64")
		if(NOT WIN64_GCC)
			target_link_options(${TARGET_NAME} PRIVATE
				$<$<CONFIG:Debug>:
					/SUBSYSTEM:CONSOLE
					/OPT:NOREF
					/DEBUG:FULL
				>
				$<$<NOT:$<CONFIG:Debug>>:
					/ENTRY:mainCRTStartup
					/SUBSYSTEM:WINDOWS
					/OPT:REF
					/OPT:ICF
					/INCREMENTAL:NO
				>
				$<$<CONFIG:RelWithDebinfo>:
					/DEBUG:FULL
				>
			)
		endif()
	endif()
endfunction()
