cmake_minimum_required(VERSION 3.7)

# This build assumes all necessary libraries are available
# at `${CMAKE_CURRENT_SOURCE_DIR}/local`.
# For me personally, this can be achieved using the shell
# script `bin/prepare`. Repositories are assumed to be
# in `$HOME/local/repos`.

project(mkweb VERSION 1.2.1.2 LANGUAGES CXX)

# CMake >=3.7 finally supports C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
	src/theme.cpp
	src/plugin.cpp
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
	DESTINATION shared/
	)

# package
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_NAME "mkweb")
set(CPACK_PACKAGE_VENDOR "Mario Konrad")
set(CPACK_PACKAGE_CONTACT "Mario Konrad <mario.konrad@gmx.net>")
set(CPACK_PACAKGE_DESCRIPTION "Static Website Generator")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Web Tool")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_MONOLITHIC_INSTALL 1)
#set(CPACK_PACKAGE_EXECUTABLES "??" "??")
set(CPACK_STRIP_FILES true)

include(CPack)
