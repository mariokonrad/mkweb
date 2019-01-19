
if(EXISTS $ENV{HOME}/local/repo/cxxopts)
	set(repo_cxxopts "file://$ENV{HOME}/local/repo/cxxopts")
else()
	set(repo_cxxopts "https://github.com/jarro2783/cxxopts")
endif()
message(STATUS "Repository for cxxopts: ${repo_cxxopts}")

ExternalProject_add(extern-cxxopts
	PREFIX "${CMAKE_CURRENT_BINARY_DIR}/cxxopts"
	GIT_REPOSITORY ${repo_cxxopts}
	GIT_TAG v1.0.0
	GIT_SHALLOW TRUE
	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=Release
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${extern_INSTALL_DIR}
		-DCXXOPTS_BUILD_EXAMPLES=OFF
		-DCXXOPTS_BUILD_TESTS=OFF
	INSTALL_DIR ${extern_INSTALL_DIR}
	)
add_library(cxxopts INTERFACE IMPORTED)
add_dependencies(cxxopts extern-cxxopts)

