# MAX
cmake_minimum_required(VERSION 3.10.2) # version supported by android studio
project(max)
cmake_policy(SET CMP0054 NEW)

# Standard
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Makefile generators on apple need this flag to compile mixed objective/c++
if(APPLE AND NOT XCODE)
	set(CMAKE_CXX_FLAGS "-ObjC++")
endif()

# Options
include(CMakeDependentOption)
option(BX_AMALGAMATED "Amalgamated bx build for faster compilation" OFF)
option(BX_CONFIG_DEBUG "Log debug messages (default: on in debug)" OFF)
option(MAX_AMALGAMATED "Amalgamated max build for faster compilation" OFF)
option(MAX_BUILD_TOOLS "Build max tools." ON)
cmake_dependent_option(MAX_BUILD_TOOLS_BIN2C "Build bx binary to c converter." ON MAX_BUILD_TOOLS OFF)
cmake_dependent_option(MAX_BUILD_TOOLS_SHADER "Build max shader tools." ON MAX_BUILD_TOOLS OFF)
cmake_dependent_option(MAX_BUILD_TOOLS_GEOMETRY "Build max geometry tools." ON MAX_BUILD_TOOLS OFF)
cmake_dependent_option(MAX_BUILD_TOOLS_TEXTURE "Build max texture tools." ON MAX_BUILD_TOOLS OFF)
set(MAX_TOOLS_PREFIX "" CACHE STRING "Prefix name to add to name of tools (to avoid clashes)")
option(MAX_BUILD_EXAMPLES "Build max examples." ON)
option(MAX_BUILD_TESTS "Build max tests." OFF)
option(MAX_INSTALL "Create installation target." ON)
cmake_dependent_option(
	MAX_INSTALL_EXAMPLES "Install examples and their runtimes." OFF "MAX_INSTALL;MAX_BUILD_EXAMPLES" OFF
)
option(MAX_CUSTOM_TARGETS "Include convenience custom targets." ON)
option(MAX_CONFIG_MULTITHREADED "Build max with multithreaded configuration" ON)
option(MAX_CONFIG_RENDERER_WEBGPU "Enable the webgpu renderer" OFF)
option(MAX_CONFIG_DEBUG_ANNOTATION "Enable gfx debug annotations (default: on in debug)" OFF)

set(MAX_OPENGL_VERSION "" CACHE STRING "Specify minimum opengl version")
set(MAX_OPENGLES_VERSION "" CACHE STRING "Specify minimum OpenGL ES version")
set(MAX_LIBRARY_TYPE "STATIC" CACHE STRING "Linking type for library")
set_property(CACHE MAX_LIBRARY_TYPE PROPERTY STRINGS "STATIC" "SHARED")

set(MAX_CONFIG_DEFAULT_MAX_ENCODERS "" CACHE STRING "Specify default maximum encoder count (multithreaded only)")
set(MAX_CONFIG_MAX_DRAW_CALLS "" CACHE STRING "Specify maximum draw calls")
set(MAX_CONFIG_MAX_VIEWS "" CACHE STRING "Specify maximum view count (should be a power of two)")
set(MAX_CONFIG_MAX_FRAME_BUFFERS "" CACHE STRING "Specify maximum frame buffers")
set(MAX_CONFIG_MAX_VERTEX_LAYOUTS "" CACHE STRING "Specify maximum vertex layouts")
set(MAX_CONFIG_MAX_VERTEX_BUFFERS "" CACHE STRING "Specify maximum vertex buffer count")
set(MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS "" CACHE STRING "Specify maximum dynamic vertex buffer count")
set(MAX_CONFIG_MAX_INDEX_BUFFERS "" CACHE STRING "Specify maximum index buffer count")
set(MAX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS "" CACHE STRING "Specify maximum dynamic index buffer count")
set(MAX_CONFIG_MAX_TEXTURES "" CACHE STRING "Specify maximum texture count")
set(MAX_CONFIG_MAX_TEXTURE_SAMPLERS "" CACHE STRING "Specify maximum texture samplers")
set(MAX_CONFIG_MAX_SHADERS "" CACHE STRING "Specify shader count")
set(MAX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM "" CACHE STRING "Specify maximum program count as 2^(program bits)")
set(MAX_CMAKE_USER_SCRIPT "" CACHE STRING "Path to custom cmake script to include")

if(MAX_CMAKE_USER_SCRIPT)
	include(${MAX_CMAKE_USER_SCRIPT})
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MAX_BUILD_TESTS)
	enable_testing()
endif()

# Directories
if(NOT BX_DIR)
	set(BX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bx" CACHE STRING "Location of bx.")
elseif(NOT IS_ABSOLUTE "${BX_DIR}")
	get_filename_component(BX_DIR "${BX_DIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
if(NOT BIMG_DIR)
	set(BIMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bimg" CACHE STRING "Location of bimg.")
elseif(NOT IS_ABSOLUTE "${BIMG_DIR}")
	get_filename_component(BIMG_DIR "${BIMG_DIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
if(NOT MINK_DIR)
	set(MINK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mink" CACHE STRING "Location of mink.")
elseif(NOT IS_ABSOLUTE "${MINK_DIR}")
	get_filename_component(MINK_DIR "${MINK_DIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
if(NOT MAX_DIR)
	set(MAX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/max" CACHE STRING "Location of max.")
elseif(NOT IS_ABSOLUTE "${MAX_DIR}")
	get_filename_component(MAX_DIR "${MAX_DIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

file(TO_CMAKE_PATH "${BX_DIR}" BX_DIR)
file(TO_CMAKE_PATH "${BIMG_DIR}" BIMG_DIR)
file(TO_CMAKE_PATH "${MINK_DIR}" MINK_DIR)
file(TO_CMAKE_PATH "${MAX_DIR}" MAX_DIR)

# Sets project version from api ver / git rev
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)
if(MAX_BUILD_TOOLS AND MAX_CUSTOM_TARGETS)
	add_custom_target(tools)
	set_target_properties(tools PROPERTIES FOLDER "max/tools")
endif()

if(MAX_INSTALL)
	include(GNUInstallDirs)

	# Layout. This works for all platforms:
	#   * <prefix>/lib*/cmake/<PROJECT-NAME>
	#   * <prefix>/lib*/
	#   * <prefix>/include/
	set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
	set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

	# Configuration
	set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
	set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
	set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
	set(namespace "${PROJECT_NAME}::")
endif()

# Check if we should build with MINK
if(IS_DIRECTORY ${MINK_DIR})
	set(MAX_BUILD_MINK ON)
else()
	message(STATUS "Directory doesn't contain mink. Building without it.")
	set(MAX_BUILD_MINK OFF)
endif()

# Subdir
if (MAX_BUILD_MINK)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/mink)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/bx)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/bimg)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmake/max)

# Install
if(MAX_INSTALL)
	# Include module with fuction 'write_basic_package_version_file'
	include(CMakePackageConfigHelpers)

	# Configure '<PROJECT-NAME>ConfigVersion.cmake'
	# Use:
	#   * PROJECT_VERSION
	if(${CMAKE_VERSION} VERSION_LESS "3.11.0")
		set(MAX_COMPATIBILITY SameMajorVersion)
	else()
		set(MAX_COMPATIBILITY SameMinorVersion)
	endif()
	write_basic_package_version_file("${version_config}" VERSION ${PROJECT_VERSION} COMPATIBILITY ${MAX_COMPATIBILITY})

	# Configure '<PROJECT-NAME>Config.cmake'
	# Use variables:
	#   * TARGETS_EXPORT_NAME
	#   * PROJECT_NAME
	#   * MAX_CMAKE_USER_SCRIPT_PRESENT
	#   * MAX_CMAKE_USER_SCRIPT_INSTALL_NAME
	if(MAX_CMAKE_USER_SCRIPT STREQUAL "")
		set(MAX_CMAKE_USER_SCRIPT_PRESENT OFF)
		set(MAX_CMAKE_USER_SCRIPT_INSTALL_NAME "NOT-USED")
	else()
		set(MAX_CMAKE_USER_SCRIPT_PRESENT ON)
		get_filename_component(MAX_CMAKE_USER_SCRIPT_INSTALL_NAME ${MAX_CMAKE_USER_SCRIPT} NAME)
	endif()
	configure_package_config_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in" "${project_config}"
		INSTALL_DESTINATION "${config_install_dir}"
	)

	install(FILES ${MAX_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licences/${PROJECT_NAME})

	# install tools
	if(MAX_CMAKE_USER_SCRIPT)
		install(FILES ${MAX_CMAKE_USER_SCRIPT} DESTINATION "${config_install_dir}")
	endif()
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/maxToolUtils.cmake DESTINATION "${config_install_dir}")

	# Config
	#   * <prefix>/lib/cmake/max/maxConfig.cmake
	#   * <prefix>/lib/cmake/max/maxConfigVersion.cmake
	install(FILES "${project_config}" "${version_config}" DESTINATION "${config_install_dir}")

	# Config
	#   * <prefix>/lib/cmake/max/maxTargets.cmake
	install(EXPORT "${TARGETS_EXPORT_NAME}" NAMESPACE "${namespace}" DESTINATION "${config_install_dir}")
endif()
