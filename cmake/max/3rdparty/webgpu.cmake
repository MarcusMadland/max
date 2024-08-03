if(TARGET webgpu)
	return()
endif()

file(GLOB WEBGPU_SOURCES ${MAX_DIR}/3rdparty/webgpu/include/webgpu/*.h
	 # ${MAX_DIR}/3rdparty/webgpu/webgpu_cpp.cpp  # requires dawn to be installed
)

# Library without sources is interface
#add_library( webgpu STATIC ${WEBGPU_SOURCES} )
add_library(webgpu INTERFACE)
target_include_directories(
	webgpu # PUBLIC
	INTERFACE $<BUILD_INTERFACE:${MAX_DIR}/3rdparty/webgpu/include>
)

# These properties are not allowed on interface
# set_target_properties(webgpu PROPERTIES FOLDER "max/3rdparty" PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}max-")

if(MAX_INSTALL AND MAX_CONFIG_RENDERER_WEBGPU)
	install(
		TARGETS webgpu
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)
endif()
