include(bx.cmake)

if(MAX_BUILD_TOOLS_BIN2C)
	include(bin2c.cmake)
endif()

if(MAX_BUILD_TESTS)
	file(
		GLOB
		BX_TEST_SOURCES #
		${BX_DIR}/3rdparty/catch/catch_amalgamated.cpp
		${BX_DIR}/tests/*_test.cpp #
		${BX_DIR}/tests/*.h #
		${BX_DIR}/tests/dbg.* #
	)
	add_executable(bx_test ${BX_TEST_SOURCES})
	target_compile_definitions(bx_test PRIVATE CATCH_AMALGAMATED_CUSTOM_MAIN)
	target_link_libraries(bx_test PRIVATE bx)
	add_test(NAME bx.test COMMAND bx_test)

	file(
		GLOB
		BX_BENCH_SOURCES #
		${BX_DIR}/tests/*_bench.cpp #
		${BX_DIR}/tests/*_bench.h #
		${BX_DIR}/tests/dbg.* #
	)
	add_executable(bx_bench ${BX_BENCH_SOURCES})
	target_link_libraries(bx_bench PRIVATE bx)
	add_test(NAME bx.bench COMMAND bx_bench)
endif()
