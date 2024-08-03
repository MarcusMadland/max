# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT NVTT_LIBRARIES)
	file(
		GLOB_RECURSE #
		NVTT_SOURCES #
		${BIMG_DIR}/3rdparty/nvtt/**.cpp #
		${BIMG_DIR}/3rdparty/nvtt/**.h #
	)
	set(NVTT_INCLUDE_DIR ${BIMG_DIR}/3rdparty/nvtt)
endif()
