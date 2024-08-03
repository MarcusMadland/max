# Grab the bin2c source files
file(GLOB_RECURSE BIN2C_SOURCES #
	 ${BX_DIR}/tools/bin2c/*.cpp #
	 ${BX_DIR}/tools/bin2c/*.h #
)

add_executable(bin2c ${BIN2C_SOURCES})

target_link_libraries(bin2c PRIVATE bx)
set_target_properties(
	bin2c PROPERTIES FOLDER "max/tools" #
					 OUTPUT_NAME ${MAX_TOOLS_PREFIX}bin2c #
)

if(MAX_BUILD_TOOLS_BIN2C)
	add_executable(max::bin2c ALIAS bin2c)
	if(MAX_CUSTOM_TARGETS)
		add_dependencies(tools bin2c)
	endif()
endif()

if(ANDROID)
	target_link_libraries(bin2c PRIVATE log)
elseif(IOS)
	set_target_properties(bin2c PROPERTIES MACOSX_BUNDLE ON MACOSX_BUNDLE_GUI_IDENTIFIER bin2c)
endif()

if(MAX_INSTALL)
	install(TARGETS bin2c EXPORT "${TARGETS_EXPORT_NAME}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
