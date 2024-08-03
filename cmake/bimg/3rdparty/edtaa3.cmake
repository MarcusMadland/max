# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT EDTAA3_LIBRARIES)
	file(
		GLOB_RECURSE #
		EDTAA3_SOURCES #
		${BIMG_DIR}/3rdparty/edtaa3/**.cpp #
		${BIMG_DIR}/3rdparty/edtaa3/**.h #
	)
	set(EDTAA3_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
