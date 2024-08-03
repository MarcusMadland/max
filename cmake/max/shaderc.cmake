# Grab the shaderc source files
file(
	GLOB
	SHADERC_SOURCES #
	${MAX_DIR}/tools/shaderc/*.cpp #
	${MAX_DIR}/tools/shaderc/*.h #
	${MAX_DIR}/src/shader* #
)

add_executable(shaderc ${SHADERC_SOURCES})

target_link_libraries(
	shaderc
	PRIVATE bx
			max-vertexlayout
			fcpp
			glslang
			glsl-optimizer
			spirv-opt
			spirv-cross
)
target_link_libraries(
	shaderc
	PRIVATE bx
			bimg
			max-vertexlayout
			fcpp
			glslang
			glsl-optimizer
			spirv-opt
			spirv-cross
			webgpu
)
if(MAX_AMALGAMATED)
	target_link_libraries(shaderc PRIVATE max-shader)
endif()

# Includes
target_include_directories(
	shaderc PRIVATE ${MAX_DIR}/3rdparty ${MINK_DIR}/include
)

set_target_properties(
	shaderc PROPERTIES FOLDER "max/tools" #
					   OUTPUT_NAME ${MAX_TOOLS_PREFIX}shaderc #
)

if(MAX_BUILD_TOOLS_SHADER)
	add_executable(max::shaderc ALIAS shaderc)
	if(MAX_CUSTOM_TARGETS)
		add_dependencies(tools shaderc)
	endif()
endif()

if(ANDROID)
	target_link_libraries(shaderc PRIVATE log)
elseif(IOS)
	set_target_properties(shaderc PROPERTIES MACOSX_BUNDLE ON MACOSX_BUNDLE_GUI_IDENTIFIER shaderc)
endif()

if(MAX_INSTALL)
	install(TARGETS shaderc EXPORT "${TARGETS_EXPORT_NAME}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
