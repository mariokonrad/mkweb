#!/bin/bash -eu

script_dir=$(dirname `readlink -f $0`)

if [ $# != 2 ] ; then
	exit 1
fi

build_prefix=$1
build_dir=${build_prefix}/yaml-cpp
install_prefix=$2

if [ ! -d "${install_prefix}" ] ; then mkdir -p ${install_prefix} ; fi
if [ ! -d "${build_dir}" ] ; then mkdir -p ${build_dir} ; fi

pushd ${build_dir}
cmake \
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=${install_prefix} \
	-DYAML_CPP_BUILD_TOOLS=OFF \
	-DYAML_CPP_BUILD_CONTRIB=OFF \
	~/local/repo/yaml-cpp
make -j 8
make install
popd
rm -fr ${build_dir}

