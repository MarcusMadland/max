
add_library(max-vertexlayout INTERFACE)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/generated/vertexlayout.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/vertexlayout.cpp
)
target_sources(max-vertexlayout INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated/vertexlayout.cpp)
target_include_directories(max-vertexlayout INTERFACE ${MAX_DIR}/include)

add_library(max-shader INTERFACE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/generated/shader.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/shader.cpp)
target_sources(max-shader INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated/shader.cpp)
target_include_directories(max-shader INTERFACE ${MAX_DIR}/include)

# Frameworks required on OS X
if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
	find_library(COCOA_LIBRARY Cocoa)
	mark_as_advanced(COCOA_LIBRARY)
	target_link_libraries(max-vertexlayout INTERFACE ${COCOA_LIBRARY})
endif()
