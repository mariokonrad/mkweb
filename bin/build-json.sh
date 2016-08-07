#!/bin/bash -eu

script_dir=$(dirname `readlink -f $0`)

if [ $# != 2 ] ; then
	exit 1
fi

build_prefix=$1
build_dir=${build_prefix}/json
install_prefix=$2

if [ ! -d "${install_prefix}" ] ; then mkdir -p ${install_prefix} ; fi
if [ ! -d "${build_dir}" ] ; then mkdir -p ${build_dir} ; fi

pushd ${build_dir}
cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=${install_prefix} \
	-DBuildTests=OFF \
	~/local/repo/json
make -j 8
make install
popd
rm -fr ${build_dir}

