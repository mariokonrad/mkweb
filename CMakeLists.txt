cmake_minimum_required(VERSION 3.5)

# This build assumes all necessary libraries are available
# at `${CMAKE_CURRENT_SOURCE_DIR}/local`.
# For me personally, this can be achieved using the shell
# script `bin/prepare`. Repositories are assumed to be
# in `$HOME/local/repos`.

project(mkweb VERSION 1.0.0 LANGUAGES CXX)

# TODO: enable those three lines, once C++17 becomes a thing
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
	set(CXX_STANDARD_OPTIONS "--std=c++1z")
else()
	message(FATAL_ERROR "Unsupported compiler detected: ${CMAKE_CXX_COMPILER_ID}")
endif()

# strip binary in release mode
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

find_package(nlohmann_json PATHS ${CMAKE_CURRENT_SOURCE_DIR}/local REQUIRED)
find_package(yaml-cpp      PATHS ${CMAKE_CURRENT_SOURCE_DIR}/local REQUIRED)

# fmt cannot be found by find_package, because it forces '--std=c++11'
# if CMAKE_CXX_STANDARD is not set, and this cannot be set to `17` or `1z`.
add_library(fmt STATIC IMPORTED)
set_target_properties(fmt PROPERTIES
	IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
	IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/local/lib/libfmt.a"
	)

# cxxopts is a header only library
add_library(cxxopts INTERFACE IMPORTED)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/src/version.hpp
	)

add_executable(mkweb
	src/mkweb.cpp
	src/config.cpp
	src/system.cpp
	)

target_include_directories(mkweb
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/local/include
		${CMAKE_CURRENT_BINARY_DIR}/src
	)

target_link_libraries(mkweb
	PRIVATE
		yaml-cpp
		nlohmann_json
		fmt
		cxxopts
		stdc++fs
	)

target_compile_options(mkweb
	PRIVATE
		-Wall
		-Wextra
		-pedantic
		-Wold-style-cast
		${CXX_STANDARD_OPTIONS}
	)

install(
	TARGETS mkweb
	RUNTIME DESTINATION bin
	)

install(
	DIRECTORY shared/
	DESTINATION shared/${PROJECT_NAME}
	)

