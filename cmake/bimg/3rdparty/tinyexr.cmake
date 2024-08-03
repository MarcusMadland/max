
# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT TINYEXR_LIBRARIES)
	file(GLOB_RECURSE #
		 TINYEXR_SOURCES #
		 ${BIMG_DIR}/3rdparty/tinyexr/**.h #
	)
	set(TINYEXR_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
