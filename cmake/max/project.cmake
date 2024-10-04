include(CMakeParseArguments)

include(${CMAKE_CURRENT_LIST_DIR}/util/ConfigureDebugging.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/../maxToolUtils.cmake)

function(add_shader FILE FOLDER)
	get_filename_component(FILENAME "${FILE}" NAME_WE)
	string(SUBSTRING "${FILENAME}" 0 2 TYPE)
	if("${TYPE}" STREQUAL "fs")
		set(TYPE "FRAGMENT")
	elseif("${TYPE}" STREQUAL "vs")
		set(TYPE "VERTEX")
	elseif("${TYPE}" STREQUAL "cs")
		set(TYPE "COMPUTE")
	else()
		set(TYPE "")
	endif()

	if(NOT "${TYPE}" STREQUAL "")
		set(COMMON FILE ${FILE} ${TYPE} INCLUDES ${MAX_DIR}/src)
		set(OUTPUTS "")
		set(OUTPUTS_PRETTY "")

		if(WIN32)
			# dx11
			set(DX11_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders/dx11/${FILENAME}.bin)
			if(NOT "${TYPE}" STREQUAL "COMPUTE")
				_max_shaderc_parse(
					DX11 ${COMMON} WINDOWS
					PROFILE s_5_0
					O 3
					OUTPUT ${DX11_OUTPUT}
				)
			else()
				_max_shaderc_parse(
					DX11 ${COMMON} WINDOWS
					PROFILE s_5_0
					O 1
					OUTPUT ${DX11_OUTPUT}
				)
			endif()
			list(APPEND OUTPUTS "DX11")
			set(OUTPUTS_PRETTY "${OUTPUTS_PRETTY}DX11, ")
		endif()

		if(APPLE)
			# metal
			set(METAL_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders/metal/${FILENAME}.bin)
			_max_shaderc_parse(METAL ${COMMON} OSX PROFILE metal OUTPUT ${METAL_OUTPUT})
			list(APPEND OUTPUTS "METAL")
			set(OUTPUTS_PRETTY "${OUTPUTS_PRETTY}Metal, ")
		endif()

		# essl
		if(NOT "${TYPE}" STREQUAL "COMPUTE")
			set(ESSL_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders/essl/${FILENAME}.bin)
			#_max_shaderc_parse(ESSL ${COMMON} ANDROID PROFILE 100_es OUTPUT ${ESSL_OUTPUT})
			_max_shaderc_parse(ESSL ${COMMON} ANDROID PROFILE 300_es OUTPUT ${ESSL_OUTPUT}) # Changed version to support 'texelFetch'
			list(APPEND OUTPUTS "ESSL")
			set(OUTPUTS_PRETTY "${OUTPUTS_PRETTY}ESSL, ")
		endif()

		# glsl
		set(GLSL_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders/glsl/${FILENAME}.bin)
		if(NOT "${TYPE}" STREQUAL "COMPUTE")
			_max_shaderc_parse(GLSL ${COMMON} LINUX PROFILE 140 OUTPUT ${GLSL_OUTPUT})
		else()
			_max_shaderc_parse(GLSL ${COMMON} LINUX PROFILE 430 OUTPUT ${GLSL_OUTPUT})
		endif()
		list(APPEND OUTPUTS "GLSL")
		set(OUTPUTS_PRETTY "${OUTPUTS_PRETTY}GLSL, ")

		# spirv
		if(NOT "${TYPE}" STREQUAL "COMPUTE")
			set(SPIRV_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders/spirv/${FILENAME}.bin)
			_max_shaderc_parse(SPIRV ${COMMON} LINUX PROFILE spirv OUTPUT ${SPIRV_OUTPUT})
			list(APPEND OUTPUTS "SPIRV")
			set(OUTPUTS_PRETTY "${OUTPUTS_PRETTY}SPIRV")
			set(OUTPUT_FILES "")
			set(COMMANDS "")
		endif()

		foreach(OUT ${OUTPUTS})
			list(APPEND OUTPUT_FILES ${${OUT}_OUTPUT})
			list(APPEND COMMANDS COMMAND "max::shaderc" ${${OUT}})
			get_filename_component(OUT_DIR ${${OUT}_OUTPUT} DIRECTORY)
			file(MAKE_DIRECTORY ${OUT_DIR})
		endforeach()

		file(RELATIVE_PATH PRINT_NAME ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
		add_custom_command(
			MAIN_DEPENDENCY ${FILE} OUTPUT ${OUTPUT_FILES} ${COMMANDS}
			COMMENT "Compiling shader ${PRINT_NAME} for ${OUTPUTS_PRETTY}"
		)
	endif()
endfunction()

function(add_max_project ARG_NAME)
	# Parse arguments
	cmake_parse_arguments(ARG "COMMON" "" "DIRECTORIES;SOURCES" ${ARGN})

	message(STATUS "Adding max project: ${ARG_NAME}")
    message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}/src/")

	# Get all source files
	set(SOURCES "")
	set(SHADERS "")
	if(APPLE)
		file(GLOB_RECURSE GLOB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm)
		list(APPEND SOURCES ${GLOB_SOURCES})
	endif()
	file(GLOB_RECURSE GLOB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.sc)
	list(APPEND SOURCES ${GLOB_SOURCES})
	file(GLOB_RECURSE GLOB_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.sc)
	list(APPEND SHADERS ${GLOB_SHADERS})

	# Add target
	if(ANDROID)
		add_library(${ARG_NAME} SHARED ${SOURCES})
	else()
		add_executable(${ARG_NAME} WIN32 ${SOURCES})
	endif()
	if(NOT MAX_INSTALL_PROJECTS)
		set_property(TARGET ${ARG_NAME} PROPERTY EXCLUDE_FROM_ALL ON)
	endif()
	
	target_link_libraries(${ARG_NAME} PUBLIC max)

	configure_debugging(${ARG_NAME} WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/runtime)
	
	target_include_directories(
		${ARG_NAME} PUBLIC ${MAX_DIR}/3rdparty
	)
	
	if(MSVC)
		set_target_properties(${ARG_NAME} PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"")
	endif()
	if(IOS)
		set_target_properties(
			${ARG_NAME}
			PROPERTIES MACOSX_BUNDLE ON
					   MACOSX_BUNDLE_GUI_IDENTIFIER ${ARG_NAME}
					   MACOSX_BUNDLE_BUNDLE_VERSION 0
					   MACOSX_BUNDLE_SHORT_VERSION_STRING 0
					   XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		)
	endif()
	
	# Entry stuff.
	if(MAX_WITH_GLFW)
		find_package(glfw3 REQUIRED)
		target_link_libraries(${ARG_NAME} PUBLIC glfw)
		target_compile_definitions(${ARG_NAME} PUBLIC ENTRY_CONFIG_USE_GLFW)
	elseif(MAX_WITH_SDL)
		find_package(SDL2 REQUIRED)
		target_link_libraries(${ARG_NAME} PUBLIC ${SDL2_LIBRARIES})
		target_compile_definitions(${ARG_NAME} PUBLIC ENTRY_CONFIG_USE_SDL)
	elseif(UNIX AND NOT APPLE AND NOT ANDROID)
		target_link_libraries(${ARG_NAME} PUBLIC X11)
	endif()

	if(ANDROID)
		target_include_directories(${ARG_NAME} PRIVATE ${MAX_DIR}/3rdparty/native_app_glue)
		target_link_libraries(${ARG_NAME} INTERFACE android EGL GLESv2)
	endif()
	
	# Runtime resources
	if(IOS OR WIN32)
		# on iOS we need to build a bundle so have to copy the data rather than symlink
		# and on windows we can't create symlinks
		add_custom_command(
			TARGET ${ARG_NAME} COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/runtime/
												$<TARGET_FILE_DIR:${ARG_NAME}>
		)
	else()
		# For everything else symlink some folders into our output directory
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/font
					$<TARGET_FILE_DIR:${ARG_NAME}>/font
		)
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/images
					$<TARGET_FILE_DIR:${ARG_NAME}>/images
		)
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/meshes
					$<TARGET_FILE_DIR:${ARG_NAME}>/meshes
		)
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/motions
					$<TARGET_FILE_DIR:${ARG_NAME}>/motions
		)
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders
					$<TARGET_FILE_DIR:${ARG_NAME}>/shaders
		)
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/text
					$<TARGET_FILE_DIR:${ARG_NAME}>/text
		)
		add_custom_command(
			TARGET ${ARG_NAME}
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/runtime/textures
					$<TARGET_FILE_DIR:${ARG_NAME}>/textures
		)
	endif()

	target_compile_definitions(
		${ARG_NAME}
		PRIVATE "-D_CRT_SECURE_NO_WARNINGS" #
				"-D__STDC_FORMAT_MACROS" #
				"-DENTRY_CONFIG_IMPLEMENT_MAIN=1" #
	)

	# Configure shaders
	if(NOT IOS
	   AND NOT EMSCRIPTEN
	   AND NOT ANDROID
	)
		foreach(SHADER ${SHADERS})
			add_shader(${SHADER} ${ARG_NAME})
		endforeach()
		source_group("Shader Files" FILES ${SHADERS})
	endif()

	if(EMSCRIPTEN)
		set_target_properties(
			${ARG_NAME}
			PROPERTIES LINK_FLAGS
			"-s PRECISE_F32=1 -s TOTAL_MEMORY=268435456 -s ENVIRONMENT=web --memory-init-file 1 --emrun"
			SUFFIX ".html"
		)
	endif()

	#
	set_target_properties(${ARG_NAME} PROPERTIES ENABLE_EXPORTS ON)
endfunction()