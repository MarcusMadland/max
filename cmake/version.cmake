# max versioning scheme:
# max 1.104.7082
#      ^ ^^^ ^^^^
#      | |   +--- Commit number  (https://github.com/marcusmadland/max / git rev-list --count HEAD)
#      | +------- API version    (from https://github.com/marcusmadland/max/blob/master/scripts/max.idl#L4)
#      +--------- Major revision (always 1)
#
# MAX_API_VERSION generated from https://github.com/marcusmadland/max/blob/master/scripts/max.idl#L4
# max/src/version.h:
# MAX_REV_NUMBER
# MAX_REV_SHA1

find_package(Git QUIET)

execute_process(
	COMMAND "${GIT_EXECUTABLE}" -C max log --pretty=format:'%h' -n 1
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_REV
	ERROR_QUIET
)

execute_process(
	COMMAND "${GIT_EXECUTABLE}" -C max rev-list --count HEAD
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_REV_COUNT
	OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
)

# read version(100) from max.idl
file(READ "${MAX_DIR}/scripts/max.idl" MAX_IDL)
string(REGEX MATCH "version\\(([^\)]+)\\)" MAX_API_VERSION ${MAX_IDL})
set(MAX_API_VERSION ${CMAKE_MATCH_1})
set(MAX_REV_NUMBER ${GIT_REV_COUNT})
set(MAX_REV ${GIT_REV})

# set project specific versions
set(PROJECT_VERSION 1.${MAX_API_VERSION}.${MAX_REV_NUMBER})
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR ${MAX_API_VERSION})
set(PROJECT_VERSION_PATCH ${MAX_REV_NUMBER})
