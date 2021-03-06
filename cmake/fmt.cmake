
if(EXISTS $ENV{HOME}/local/repo/fmt)
	set(repo_fmt "file://$ENV{HOME}/local/repo/fmt")
else()
	set(repo_fmt "https://github.com/fmtlib/fmt")
endif()
message(STATUS "Repository for fmt: ${repo_fmt}")

ExternalProject_add(extern-fmt
	PREFIX "${CMAKE_CURRENT_BINARY_DIR}/fmt"
	GIT_REPOSITORY ${repo_fmt}
	GIT_TAG 3.0.1
	GIT_SHALLOW TRUE
	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=Release
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		-DCMAKE_INSTALL_PREFIX=${extern_INSTALL_DIR}
		-DFMT_DOC=NO
		-DFMT_INSTALL=YES
		-DFMT_TEST=NO
		-DFMT_USE_CPP14=YES
	INSTALL_DIR ${extern_INSTALL_DIR}
	)

add_library(fmt STATIC IMPORTED)
set_target_properties(fmt
	PROPERTIES
		IMPORTED_LOCATION
			${extern_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}fmt${CMAKE_STATIC_LIBRARY_SUFFIX}
	)
add_dependencies(fmt extern-fmt)

