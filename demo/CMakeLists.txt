project(levk-demo)

# Library
add_subdirectory(lib)

# Executable
set(SOURCES
	main.cpp
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX src FILES ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} levk::demo-lib)
if(WINDOWS_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

# Android
set(ANDROID_CMAKE_VERSION 3.18.1)
set(ANDROID_DATA "\"../../data\"")
set(ANDROID_ABI "\"arm64-v8a\", \"armeabi-v7a\"")
set(ANDROID_VALIDATION "\"src/lib\"")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/android/app/build.gradle.in" "${CMAKE_CURRENT_SOURCE_DIR}/android/app/build.gradle")
