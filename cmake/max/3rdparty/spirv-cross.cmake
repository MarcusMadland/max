# Ensure the directory exists
if(NOT IS_DIRECTORY ${MAX_DIR})
	message(SEND_ERROR "Could not load max, directory does not exist. ${MAX_DIR}")
	return()
endif()

set(SPIRV_CROSS ${MAX_DIR}/3rdparty/spirv-cross)

file(
	GLOB
	SPIRV_CROSS_SOURCES
	#
	${SPIRV_CROSS}/spirv.hpp
	${SPIRV_CROSS}/spirv_cfg.cpp
	${SPIRV_CROSS}/spirv_cfg.hpp
	${SPIRV_CROSS}/spirv_common.hpp
	${SPIRV_CROSS}/spirv_cpp.cpp
	${SPIRV_CROSS}/spirv_cpp.hpp
	${SPIRV_CROSS}/spirv_cross.cpp
	${SPIRV_CROSS}/spirv_cross.hpp
	${SPIRV_CROSS}/spirv_cross_parsed_ir.cpp
	${SPIRV_CROSS}/spirv_cross_parsed_ir.hpp
	${SPIRV_CROSS}/spirv_cross_util.cpp
	${SPIRV_CROSS}/spirv_cross_util.hpp
	${SPIRV_CROSS}/spirv_glsl.cpp
	${SPIRV_CROSS}/spirv_glsl.hpp
	${SPIRV_CROSS}/spirv_hlsl.cpp
	${SPIRV_CROSS}/spirv_hlsl.hpp
	${SPIRV_CROSS}/spirv_msl.cpp
	${SPIRV_CROSS}/spirv_msl.hpp
	${SPIRV_CROSS}/spirv_parser.cpp
	${SPIRV_CROSS}/spirv_parser.hpp
	${SPIRV_CROSS}/spirv_reflect.cpp
	${SPIRV_CROSS}/spirv_reflect.hpp
)

add_library(spirv-cross STATIC ${SPIRV_CROSS_SOURCES})

target_compile_definitions(spirv-cross PRIVATE SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)

# Put in a "max" folder in Visual Studio
set_target_properties(spirv-cross PROPERTIES FOLDER "max/3rdparty")

target_include_directories(
	spirv-cross #
	PUBLIC #
		   ${SPIRV_CROSS} #
	PRIVATE #
			${SPIRV_CROSS}/include #
)
