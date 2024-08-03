# Ensure the directory exists
if(NOT IS_DIRECTORY ${MAX_DIR})
	message(SEND_ERROR "Could not load max, directory does not exist. ${MAX_DIR}")
	return()
endif()

if(NOT DEAR_IMGUI_LIBRARIES)
	file(
		GLOB #
		DEAR_IMGUI_SOURCES #
		${MAX_DIR}/3rdparty/dear-imgui/*.cpp #
		${MAX_DIR}/3rdparty/dear-imgui/*.h #
		${MAX_DIR}/3rdparty/dear-imgui/*.inl #
	)
	set(DEAR_IMGUI_INCLUDE_DIR ${MAX_DIR}/3rdparty)
endif()
