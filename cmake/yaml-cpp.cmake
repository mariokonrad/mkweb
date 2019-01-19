
if(EXISTS $ENV{HOME}/local/repo/yaml-cpp)
	set(repo_yaml "file://$ENV{HOME}/local/repo/yaml-cpp")
else()
	set(repo_yaml "https://github.com/jbeder/yaml-cpp")
endif()
message(STATUS "Repository for yaml-cpp: ${repo_yaml}")

ExternalProject_add(extern-yaml-cpp
	PREFIX "${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp"
	GIT_REPOSITORY ${repo_yaml}
	GIT_TAG yaml-cpp-0.6.2
	GIT_SHALLOW TRUE
	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=Release
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${extern_INSTALL_DIR}
		-DYAML_CPP_BUILD_TOOLS=OFF
		-DYAML_CPP_BUILD_CONTRIB=OFF
		-DYAML_CPP_BUILD_TESTS=OFF
	INSTALL_DIR ${extern_INSTALL_DIR}
	)

add_library(yaml-cpp STATIC IMPORTED)
set_target_properties(yaml-cpp
	PROPERTIES
		IMPORTED_LOCATION
			${extern_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
	)
add_dependencies(yaml-cpp extern-yaml-cpp)

