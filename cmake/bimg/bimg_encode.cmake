
# Ensure the directory exists
if(NOT IS_DIRECTORY ${BIMG_DIR})
	message(SEND_ERROR "Could not load bimg_encode, directory does not exist. ${BIMG_DIR}")
	return()
endif()

add_library(bimg_encode STATIC)

# Put in a "max" folder in Visual Studio
set_target_properties(bimg_encode PROPERTIES FOLDER "max/3rdparty")

target_include_directories(
	bimg_encode
	PUBLIC $<BUILD_INTERFACE:${BIMG_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE ${LIBSQUISH_INCLUDE_DIR} #
			${ASTC_ENCODER_INCLUDE_DIR} #
			${EDTAA3_INCLUDE_DIR} #
			${ETC1_INCLUDE_DIR} #
			${ETC2_INCLUDE_DIR} #
			${NVTT_INCLUDE_DIR} #
			${PVRTC_INCLUDE_DIR} #
			${TINYEXR_INCLUDE_DIR} #
			${IQA_INCLUDE_DIR} #
			${MINIZ_INCLUDE_DIR} #
)

file(
	GLOB_RECURSE
	BIMG_ENCODE_SOURCES
	${BIMG_DIR}/include/* #
	${BIMG_DIR}/src/image_encode.* #
	${BIMG_DIR}/src/image_cubemap_filter.* #
	${LIBSQUISH_SOURCES} #
	${EDTAA3_SOURCES} #
	${ETC1_SOURCES} #
	${ETC2_SOURCES} #
	${NVTT_SOURCES} #
	${PVRTC_SOURCES} #
	${TINYEXR_SOURCES}
	${IQA_SOURCES} #
)

target_sources(bimg_encode PRIVATE ${BIMG_ENCODE_SOURCES})

target_link_libraries(
	bimg_encode
	PUBLIC bx #
		   ${LIBSQUISH_LIBRARIES} #
		   ${ASTC_ENCODER_LIBRARIES} #
		   ${EDTAA3_LIBRARIES} #
		   ${ETC1_LIBRARIES} #
		   ${ETC2_LIBRARIES} #
		   ${NVTT_LIBRARIES} #
		   ${PVRTC_LIBRARIES} #
		   ${TINYEXR_LIBRARIES} #
		   ${IQA_LIBRARIES} #
)

include(CheckCXXCompilerFlag)
foreach(flag "-Wno-implicit-fallthrough" "-Wno-shadow" "-Wno-shift-negative-value" "-Wno-undef")
	check_cxx_compiler_flag(${flag} flag_supported)
	if(flag_supported)
		target_compile_options(bimg_encode PRIVATE ${flag})
	endif()
endforeach()

foreach(flag "-Wno-class-memaccess" "-Wno-deprecated-copy")
	check_cxx_compiler_flag(${flag} flag_supported)
	if(flag_supported)
		foreach(file ${BIMG_ENCODE_SOURCES})
			get_source_file_property(lang ${file} LANGUAGE)
			if(lang STREQUAL "CXX")
				set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS ${flag})
			endif()
		endforeach()
	endif()
endforeach()

if(MAX_INSTALL AND NOT MAX_LIBRARY_TYPE MATCHES "SHARED")
	install(
		TARGETS bimg_encode
		EXPORT "${TARGETS_EXPORT_NAME}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)
endif()
