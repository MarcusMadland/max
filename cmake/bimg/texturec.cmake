# Grab the texturec source files
file(GLOB_RECURSE TEXTUREC_SOURCES #
	 ${BIMG_DIR}/tools/texturec/*.cpp #
	 ${BIMG_DIR}/tools/texturec/*.h #
)

add_executable(texturec ${TEXTUREC_SOURCES})

target_link_libraries(texturec PRIVATE bimg_decode bimg_encode bimg)
set_target_properties(
	texturec PROPERTIES FOLDER "max/tools" #
						OUTPUT_NAME ${MAX_TOOLS_PREFIX}texturec #
)

if(MAX_BUILD_TOOLS_TEXTURE)
	add_executable(max::texturec ALIAS texturec)
	if(MAX_CUSTOM_TARGETS)
		add_dependencies(tools texturec)
	endif()
endif()

if(ANDROID)
	target_link_libraries(texturec PRIVATE log)
elseif(IOS)
	set_target_properties(texturec PROPERTIES MACOSX_BUNDLE ON MACOSX_BUNDLE_GUI_IDENTIFIER texturec)
endif()

if(MAX_INSTALL)
	install(TARGETS texturec EXPORT "${TARGETS_EXPORT_NAME}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
