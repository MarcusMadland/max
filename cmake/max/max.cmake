# To prevent this warning: https://cmake.org/cmake/help/git-stage/policy/CMP0072.html
if(POLICY CMP0072)
	cmake_policy(SET CMP0072 NEW)
endif()

# Ensure the directory exists
if(NOT IS_DIRECTORY ${MAX_DIR})
	message(SEND_ERROR "Could not load max, directory does not exist. ${MAX_DIR}")
	return()
endif()

if(NOT APPLE)
	set(MAX_AMALGAMATED_SOURCE ${MAX_DIR}/src/amalgamated.cpp)
else()
	set(MAX_AMALGAMATED_SOURCE ${MAX_DIR}/src/amalgamated.mm)
endif()

# Grab the max source files
file(
	GLOB
	MAX_SOURCES
	${MAX_DIR}/src/*.cpp
	${MAX_DIR}/src/*.mm
	${MAX_DIR}/src/*.h
	${MAX_DIR}/include/max/*.h
	${MAX_DIR}/include/max/c99/*.h
)
if(MAX_AMALGAMATED)
	set(MAX_NOBUILD ${MAX_SOURCES})
	list(REMOVE_ITEM MAX_NOBUILD ${MAX_AMALGAMATED_SOURCE})
	foreach(MAX_SRC ${MAX_NOBUILD})
		set_source_files_properties(${MAX_SRC} PROPERTIES HEADER_FILE_ONLY ON)
	endforeach()
else()
	# Do not build using amalgamated sources
	set_source_files_properties(${MAX_DIR}/src/amalgamated.cpp PROPERTIES HEADER_FILE_ONLY ON)
	set_source_files_properties(${MAX_DIR}/src/amalgamated.mm PROPERTIES HEADER_FILE_ONLY ON)
endif()

# Create the max target
if(MAX_LIBRARY_TYPE STREQUAL STATIC)
	add_library(max STATIC ${MAX_SOURCES})
else()
	add_library(max SHARED ${MAX_SOURCES})
	target_compile_definitions(max PUBLIC MAX_SHARED_LIB_BUILD=1)
endif()

if(MAX_CONFIG_RENDERER_WEBGPU)
	include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/webgpu.cmake)
	target_compile_definitions(max PRIVATE MAX_CONFIG_RENDERER_WEBGPU=1)
	if(EMSCRIPTEN)
		target_link_options(max PRIVATE "-s USE_WEBGPU=1")
	else()
		target_link_libraries(max PRIVATE webgpu)
	endif()
endif()

if(EMSCRIPTEN)
	target_link_options(max PUBLIC "-sMAX_WEBGL_VERSION=2")
endif()

if(NOT ${MAX_OPENGL_VERSION} STREQUAL "")
	target_compile_definitions(max PRIVATE MAX_CONFIG_RENDERER_OPENGL_MIN_VERSION=${MAX_OPENGL_VERSION})
endif()

if(NOT ${MAX_OPENGLES_VERSION} STREQUAL "")
	target_compile_definitions(max PRIVATE MAX_CONFIG_RENDERER_OPENGLES_MIN_VERSION=${MAX_OPENGLES_VERSION})
endif()

if(NOT ${MAX_CONFIG_DEFAULT_MAX_ENCODERS} STREQUAL "")
	target_compile_definitions(
		max
		PUBLIC
			"MAX_CONFIG_DEFAULT_MAX_ENCODERS=$<IF:$<BOOL:${MAX_CONFIG_MULTITHREADED}>,${MAX_CONFIG_DEFAULT_MAX_ENCODERS},1>"
	)
endif()

set(MAX_CONFIG_OPTIONS "")
list(
	APPEND
	MAX_CONFIG_OPTIONS
	"MAX_CONFIG_MAX_DRAW_CALLS"
	"MAX_CONFIG_MAX_VIEWS"
	"MAX_CONFIG_MAX_FRAME_BUFFERS"
	"MAX_CONFIG_MAX_VERTEX_LAYOUTS"
	"MAX_CONFIG_MAX_VERTEX_BUFFERS"
	"MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS"
	"MAX_CONFIG_MAX_INDEX_BUFFERS"
	"MAX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS"
	"MAX_CONFIG_MAX_TEXTURES"
	"MAX_CONFIG_MAX_TEXTURE_SAMPLERS"
	"MAX_CONFIG_MAX_SHADERS"
	"MAX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM"
)
foreach(MAX_CONFIG_OPTION IN LISTS MAX_CONFIG_OPTIONS)
	if(NOT ${${MAX_CONFIG_OPTION}} STREQUAL "")
		target_compile_definitions(max PUBLIC "${MAX_CONFIG_OPTION}=${${MAX_CONFIG_OPTION}}")
	endif()
endforeach()

# Special Visual Studio Flags
if(MSVC)
	target_compile_definitions(max PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

# Add debug config required in bx headers since bx is private
target_compile_definitions(
	max
	PUBLIC
		"BX_CONFIG_DEBUG=$<OR:$<CONFIG:Debug>,$<BOOL:${BX_CONFIG_DEBUG}>>"
		"MAX_CONFIG_DEBUG_ANNOTATION=$<AND:$<NOT:$<STREQUAL:${CMAKE_SYSTEM_NAME},WindowsStore>>,$<OR:$<CONFIG:Debug>,$<BOOL:${MAX_CONFIG_DEBUG_ANNOTATION}>>>"
		"MAX_CONFIG_MULTITHREADED=$<BOOL:${MAX_CONFIG_MULTITHREADED}>"
)

# directx-headers
set(DIRECTX_HEADERS)
if(UNIX
   AND NOT APPLE
   AND NOT EMSCRIPTEN
   AND NOT ANDROID
) # Only Linux
	set(DIRECTX_HEADERS
		${MAX_DIR}/3rdparty/directx-headers/include/directx ${MAX_DIR}/3rdparty/directx-headers/include
		${MAX_DIR}/3rdparty/directx-headers/include/wsl/stubs
	)
elseif(WIN32) # Only Windows
	set(DIRECTX_HEADERS ${MAX_DIR}/3rdparty/directx-headers/include/directx
						${MAX_DIR}/3rdparty/directx-headers/include
	)
endif()

# Includes
target_include_directories(
	max PRIVATE ${DIRECTX_HEADERS} ${MAX_DIR}/3rdparty ${MAX_DIR}/3rdparty/khronos
	PUBLIC $<BUILD_INTERFACE:${MAX_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# max depends on bx and bimg
target_link_libraries(max PUBLIC bx bimg bimg_decode jolt meshoptimizer)

if (MAX_BUILD_MINK)
	target_link_libraries(max PUBLIC mink)
endif()

# Frameworks required on iOS, tvOS and macOS
if(${CMAKE_SYSTEM_NAME} MATCHES iOS|tvOS)
	target_link_libraries(
		max
		PUBLIC
			"-framework OpenGLES -framework Metal -framework UIKit -framework CoreGraphics -framework QuartzCore -framework IOKit -framework CoreFoundation"
	)
elseif(APPLE)
	find_library(COCOA_LIBRARY Cocoa)
	find_library(METAL_LIBRARY Metal)
	find_library(QUARTZCORE_LIBRARY QuartzCore)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
	mark_as_advanced(COCOA_LIBRARY)
	mark_as_advanced(METAL_LIBRARY)
	mark_as_advanced(QUARTZCORE_LIBRARY)
	mark_as_advanced(IOKIT_LIBRARY)
	mark_as_advanced(COREFOUNDATION_LIBRARY)
	target_link_libraries(
		max PUBLIC ${COCOA_LIBRARY} ${METAL_LIBRARY} ${QUARTZCORE_LIBRARY} ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY}
	)
endif()

if(UNIX
   AND NOT APPLE
   AND NOT EMSCRIPTEN
   AND NOT ANDROID
)
	find_package(X11 REQUIRED)
	find_package(OpenGL REQUIRED)
	#The following commented libraries are linked by bx
	#find_package(Threads REQUIRED)
	#find_library(LIBRT_LIBRARIES rt)
	#find_library(LIBDL_LIBRARIES dl)
	target_link_libraries(max PUBLIC ${X11_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

# Exclude mm files if not on OS X
if(NOT APPLE)
	set_source_files_properties(${MAX_DIR}/src/glcontext_eagl.mm PROPERTIES HEADER_FILE_ONLY ON)
	set_source_files_properties(${MAX_DIR}/src/glcontext_nsgl.mm PROPERTIES HEADER_FILE_ONLY ON)
	set_source_files_properties(${MAX_DIR}/src/renderer_mtl.mm PROPERTIES HEADER_FILE_ONLY ON)
endif()

# Exclude glx context on non-unix
if(NOT UNIX OR APPLE)
	set_source_files_properties(${MAX_DIR}/src/glcontext_glx.cpp PROPERTIES HEADER_FILE_ONLY ON)
endif()

# Put in a "max" folder in Visual Studio
set_target_properties(max PROPERTIES FOLDER "max")

# in Xcode we need to specify this file as objective-c++ (instead of renaming to .mm)
if(XCODE)
	set_source_files_properties(
		${MAX_DIR}/src/renderer_vk.cpp PROPERTIES LANGUAGE OBJCXX XCODE_EXPLICIT_FILE_TYPE sourcecode.cpp.objcpp
	)
endif()

if(MAX_INSTALL)
	install(
		TARGETS max
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)

	install(DIRECTORY ${MAX_DIR}/include/max DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

	# header required for shader compilation
	install(FILES ${MAX_DIR}/src/max_shader.sh ${MAX_DIR}/src/max_compute.sh
			DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/max"
	)
endif()
