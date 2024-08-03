# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT ETC1_LIBRARIES)
	file(GLOB_RECURSE ETC1_SOURCES ${BIMG_DIR}/3rdparty/etc1/**.cpp #
		 ${BIMG_DIR}/3rdparty/etc1/**.hpp #
	)
	set(ETC1_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
