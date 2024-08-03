# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT LIBSQUISH_LIBRARIES)
	file(
		GLOB_RECURSE #
		LIBSQUISH_SOURCES #
		${BIMG_DIR}/3rdparty/libsquish/**.cpp #
		${BIMG_DIR}/3rdparty/libsquish/**.h #
	)
	set(LIBSQUISH_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
