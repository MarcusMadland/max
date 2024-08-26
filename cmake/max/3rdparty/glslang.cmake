# Ensure the directory exists
if(NOT IS_DIRECTORY ${MAX_DIR})
	message(SEND_ERROR "Could not load max, directory does not exist. ${MAX_DIR}")
	return()
endif()

set(GLSLANG ${MAX_DIR}/3rdparty/glslang)
set(SPIRV_TOOLS ${MAX_DIR}/3rdparty/spirv-tools)

file(
	GLOB_RECURSE
	GLSLANG_SOURCES
	${GLSLANG}/glslang/*.cpp
	${GLSLANG}/glslang/*.h
	#
	${GLSLANG}/hlsl/*.cpp
	${GLSLANG}/hlsl/*.h
	#
	${GLSLANG}/SPIRV/*.cpp
	${GLSLANG}/SPIRV/*.h
	#
	${GLSLANG}/OGLCompilersDLL/*.cpp
	${GLSLANG}/OGLCompilersDLL/*.h
)

if(WIN32)
	list(FILTER GLSLANG_SOURCES EXCLUDE REGEX "glslang/OSDependent/Unix/.*.cpp")
	list(FILTER GLSLANG_SOURCES EXCLUDE REGEX "glslang/OSDependent/Unix/.*.h")
else()
	list(FILTER GLSLANG_SOURCES EXCLUDE REGEX "glslang/OSDependent/Windows/.*.cpp")
	list(FILTER GLSLANG_SOURCES EXCLUDE REGEX "glslang/OSDependent/Windows/.*.h")
endif()

add_library(glslang STATIC ${GLSLANG_SOURCES})

target_compile_definitions(
	glslang
	PRIVATE #
			ENABLE_OPT=1 # spriv-tools
			ENABLE_HLSL=1 #
)

# Put in a "max" folder in Visual Studio
set_target_properties(glslang PROPERTIES FOLDER "max/3rdparty")

target_include_directories(
	glslang
	PUBLIC ${GLSLANG} #
		   ${GLSLANG}/glslang/Public #
	PRIVATE ${GLSLANG}/.. #
			${SPIRV_TOOLS}/include #
			${SPIRV_TOOLS}/source #
)
