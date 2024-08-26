
# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

file(
	GLOB_RECURSE
	BIMG_SOURCES
	${BIMG_DIR}/include/* #
	${BIMG_DIR}/src/image.* #
	${BIMG_DIR}/src/image_gnf.cpp #
	#
	${ASTC_ENCODER_SOURCES}
	${MINIZ_SOURCES}
)

add_library(bimg STATIC ${BIMG_SOURCES})

# Put in a "max" folder in Visual Studio
set_target_properties(bimg PROPERTIES FOLDER "max/3rdparty")

target_include_directories(
	bimg PUBLIC $<BUILD_INTERFACE:${BIMG_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE ${ASTC_ENCODER_INCLUDE_DIR} #
			${MINIZ_INCLUDE_DIR} #
)

target_link_libraries(
	bimg
	PUBLIC bx #
		   ${ASTC_ENCODER_LIBRARIES} #
		   ${MINIZ_LIBRARIES} #
)

if(MAX_INSTALL)
	install(
		TARGETS bimg
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)
	install(DIRECTORY ${BIMG_DIR}/include/bimg DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
