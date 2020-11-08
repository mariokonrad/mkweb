cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(mkweb VERSION 1.2.2.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ExternalProject)

# C++ Standard, Compiler (GNU) extensions disabled
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(extern_INSTALL_DIR "${CMAKE_BINARY_DIR}/local")
include(nlohmann_json)
include(fmt)
include(yaml-cpp)
include(cxxopts)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
	${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp
	)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
	PRIVATE
		src/mkweb.cpp
		src/config.cpp
		src/system.cpp
		src/theme.cpp
		src/plugin.cpp
		${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp
	)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
		$<BUILD_INTERFACE:${extern_INSTALL_DIR}/include>
	)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		nlohmann_json
		yaml-cpp
		fmt
		cxxopts
		stdc++fs
	)

target_compile_options(${PROJECT_NAME}
	PRIVATE
		-Wall
		-Wextra
		-pedantic
		-Wold-style-cast
	)

target_link_options(${PROJECT_NAME}
	PRIVATE
		$<$<CONFIG:Release>:-s>
	)

install(
	TARGETS ${PROJECT_NAME}
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
