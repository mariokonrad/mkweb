cmake_minimum_required(VERSION 3.11)

project(mkweb VERSION 1.2.2.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ExternalProject)

# location of repositories
option(LOCAL_REPO "Use local repositories ($HOME/local/repo/...)" ON)
if(LOCAL_REPO)
	set(repo_json    "file://$ENV{HOME}/local/repo/json")
	set(repo_fmt     "file://$ENV{HOME}/local/repo/fmt")
	set(repo_yaml    "file://$ENV{HOME}/local/repo/yaml-cpp")
	set(repo_cxxopts "file://$ENV{HOME}/local/repo/cxxopts")
else()
	set(repo_json    "https://github.com/nlohmann/json")
	set(repo_fmt     "https://github.com/fmtlib/fmt")
	set(repo_yaml    "https://github.com/jbeder/yaml-cpp")
	set(repo_cxxopts "https://github.com/jarro2783/cxxopts")
endif()

# C++ Standard, Compiler (GNU) extensions disabled
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# strip binary in release mode
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(extern_INSTALL_DIR "${CMAKE_BINARY_DIR}/local")
include(nlohmann_json)
include(fmt)
include(yaml-cpp)
include(cxxopts)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/src/version.hpp
	)

add_executable(${PROJECT_NAME}
	src/mkweb.cpp
	src/config.cpp
	src/system.cpp
	src/theme.cpp
	src/plugin.cpp
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
		${CXX_STANDARD_OPTIONS}
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
