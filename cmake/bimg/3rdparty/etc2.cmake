# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT ETC2_LIBRARIES)
	file(
		GLOB_RECURSE #
		ETC2_SOURCES #
		${BIMG_DIR}/3rdparty/etc2/**.cpp #
		${BIMG_DIR}/3rdparty/etc2/**.hpp #
	)
	set(ETC2_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
