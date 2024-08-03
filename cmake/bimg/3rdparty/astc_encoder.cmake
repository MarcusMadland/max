# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT ASTC_ENCODER_LIBRARIES)
	file(
		GLOB_RECURSE #
		ASTC_ENCODER_SOURCES #
		${BIMG_DIR}/3rdparty/astc-encoder/source/*.cpp #
		${BIMG_DIR}/3rdparty/astc-encoder/source/*.h #
	)
	set(ASTC_ENCODER_INCLUDE_DIR ${BIMG_DIR}/3rdparty/astc-encoder/include)
endif()
