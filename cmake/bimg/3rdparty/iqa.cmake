# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT IQA_LIBRARIES)
	file(
		GLOB_RECURSE #
		IQA_SOURCES #
		${BIMG_DIR}/3rdparty/iqa/include/**.h #
		${BIMG_DIR}/3rdparty/iqa/source/**.c #
	)
	set(IQA_INCLUDE_DIR ${BIMG_DIR}/3rdparty/iqa/include)
endif()
