
# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT PVRTC_LIBRARIES)
	file(
		GLOB_RECURSE #
		PVRTC_SOURCES #
		${BIMG_DIR}/3rdparty/pvrtc/**.cpp #
		${BIMG_DIR}/3rdparty/pvrtc/**.h #
	)
	set(PVRTC_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
