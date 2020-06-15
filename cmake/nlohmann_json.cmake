
if(EXISTS $ENV{HOME}/local/repo/json)
	set(repo_json "file://$ENV{HOME}/local/repo/json")
else()
	set(repo_json "https://github.com/nlohmann/json")
endif()
message(STATUS "Repository for nlohmann_json: ${repo_json}")

ExternalProject_add(extern-json
	PREFIX "${CMAKE_CURRENT_BINARY_DIR}/json"
	GIT_REPOSITORY ${repo_json}
	GIT_TAG v3.8.0
	GIT_SHALLOW TRUE
	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=Release
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${extern_INSTALL_DIR}
		-DJSON_BuildTests=OFF
		-DJSON_MultipleHeaders=OFF
	INSTALL_DIR ${extern_INSTALL_DIR}
	)
add_library(nlohmann_json INTERFACE IMPORTED)
add_dependencies(nlohmann_json extern-json)

