set(MESHOPTIMIZER_DIR ${BGFX_DIR}/3rdparty/meshoptimizer)

file(
	GLOB #
	MESHOPTIMIZER_SOURCES #
	${MESHOPTIMIZER_DIR}/src/*.cpp #
	${MESHOPTIMIZER_DIR}/src/*.h #
)
# Create Jolt lib
add_library(meshoptimizer ${MESHOPTIMIZER_SOURCES})

if(BGFX_INSTALL)
	install(
		TARGETS meshoptimizer
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)
	install(DIRECTORY ${MESHOPTIMIZER_DIR}/src/meshoptimizer DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

# Put in a "bgfx" folder in Visual Studio
set_target_properties(meshoptimizer PROPERTIES FOLDER "bgfx")

#target_include_directories(meshoptimizer PUBLIC ${MESHOPTIMIZER_DIR})
