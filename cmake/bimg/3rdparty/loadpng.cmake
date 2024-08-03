# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg, directory does not exist. ${BIMG_DIR}")
	return()
endif()

if(NOT LOADPNG_LIBRARIES)
	file(
		GLOB_RECURSE #
		LOADPNG_SOURCES #
		${BIMG_DIR}/3rdparty/lodepng/lodepng.cpp #
		${BIMG_DIR}/3rdparty/lodepng/lodepng.h #
	)
	set_source_files_properties(${BIMG_DIR}/3rdparty/lodepng/lodepng.cpp PROPERTIES HEADER_FILE_ONLY ON)
	set(LOADPNG_INCLUDE_DIR ${BIMG_DIR}/3rdparty)
endif()
