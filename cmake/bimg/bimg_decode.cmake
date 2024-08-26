
# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg_decode, directory does not exist. ${BIMG_DIR}")
	return()
endif()

file(
	GLOB_RECURSE
	BIMG_DECODE_SOURCES #
	${BIMG_DIR}/include/* #
	${BIMG_DIR}/src/image_decode.* #
	#
	${LOADPNG_SOURCES} #
	${MINIZ_SOURCES} #
)

add_library(bimg_decode STATIC ${BIMG_DECODE_SOURCES})

# Put in a "max" folder in Visual Studio
set_target_properties(bimg_decode PROPERTIES FOLDER "max/3rdparty")
target_include_directories(
	bimg_decode
	PUBLIC $<BUILD_INTERFACE:${BIMG_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE ${LOADPNG_INCLUDE_DIR} #
			${MINIZ_INCLUDE_DIR} #
			${TINYEXR_INCLUDE_DIR} #
)

target_link_libraries(
	bimg_decode
	PUBLIC bx #
		   ${LOADPNG_LIBRARIES} #
		   ${MINIZ_LIBRARIES} #
		   ${TINYEXR_LIBRARIES} #
)

if(MAX_INSTALL AND NOT MAX_LIBRARY_TYPE MATCHES "SHARED")
	install(
		TARGETS bimg_decode
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)
endif()
