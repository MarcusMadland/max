# Grab the geometryc source files
file(
	GLOB_RECURSE
	GEOMETRYC_SOURCES #
	${MAX_DIR}/tools/geometryc/*.cpp #
	${MAX_DIR}/tools/geometryc/*.h #
	#
	${MESHOPTIMIZER_SOURCES}
)
add_executable(geometryc ${GEOMETRYC_SOURCES})

target_include_directories(geometryc PRIVATE ${MAX_DIR}/3rdparty ${MINK_DIR}/include)

target_link_libraries(geometryc PRIVATE bx max max-vertexlayout meshoptimizer)

# @todo Do similar to max.cmake instead.
set(CMAKE_MODULE_PATH ${MAX_DIR}/../cmake/modules)
find_package(Fbx 2016)
if (Fbx_FOUND)
    target_compile_definitions(geometryc PRIVATE MAX_GEOMETRYC_FBX_COMPILER)
    target_link_libraries(geometryc PRIVATE fbx::sdk)
endif ()
#

target_compile_definitions(geometryc PRIVATE "-D_CRT_SECURE_NO_WARNINGS")
set_target_properties(
	geometryc PROPERTIES FOLDER "max/tools" #
						 OUTPUT_NAME ${MAX_TOOLS_PREFIX}geometryc #
)

if(MAX_BUILD_TOOLS_GEOMETRY)
	add_executable(max::geometryc ALIAS geometryc)
	if(MAX_CUSTOM_TARGETS)
		add_dependencies(tools geometryc)
	endif()
endif()

if(IOS)
	set_target_properties(geometryc PROPERTIES MACOSX_BUNDLE ON MACOSX_BUNDLE_GUI_IDENTIFIER geometryc)
endif()

if(MAX_INSTALL)
	install(TARGETS geometryc EXPORT "${TARGETS_EXPORT_NAME}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
