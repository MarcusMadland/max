# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT MINIZ_LIBRARIES)
	file(GLOB_RECURSE #
		 MINIZ_SOURCES #
		 ${BIMG_DIR}/3rdparty/tinyexr/deps/miniz/miniz.* #
	)
	set(MINIZ_INCLUDE_DIR ${BIMG_DIR}/3rdparty/tinyexr/deps/miniz)
endif()
