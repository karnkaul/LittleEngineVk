function(git_get_commit_hash OUTPUT_VAR)
	set(git_commit_hash "[unknown]")
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE git_commit_hash
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	set(${OUTPUT_VAR} ${git_commit_hash} CACHE STRING "Git commit hash")
	message(STATUS "Git commit hash [${git_commit_hash}] saved to [${OUTPUT_VAR}]")
endfunction()

function(git_update_submodules msg_type)
	message(STATUS "Updating git submodules...")
	execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE UPDATE_SUBMODULES_RESULT
	)
	if(NOT UPDATE_SUBMODULES_RESULT EQUAL "0")
		message(${msg_type} "git submodule update failed!")
	endif()
endfunction()

function(unzip_archive archive_name subdir)
	if(NOT EXISTS "${subdir}/${archive_name}")
		message(FATAL_ERROR "Required archvives missing!\n${subdir}/${archive_name}")
	endif()
	message(STATUS "Extracting ${archive_name}...")
	execute_process(COMMAND 
		${CMAKE_COMMAND} -E tar -xf "${archive_name}"
		WORKING_DIRECTORY "${subdir}"
	)
endfunction()

function(target_source_group target)
	get_target_property(sources ${target} SOURCES)
	if(${ARGC} GREATER 1)
		source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX ${ARGV1} FILES ${sources})
	else()
		source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
	endif()
endfunction()
