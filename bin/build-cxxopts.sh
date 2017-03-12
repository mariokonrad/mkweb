#!/bin/bash -eu

script_dir=$(dirname `readlink -f $0`)

if [ $# != 2 ] ; then
	exit 1
fi

source ${script_dir}/common.sh

pkg=cxxopts

build_prefix=$1
build_dir=${build_prefix}/${pkg}
install_prefix=$2

if [ ! -d "${install_prefix}" ] ; then mkdir -p ${install_prefix} ; fi
if [ ! -d "${build_dir}" ] ; then mkdir -p ${build_dir} ; fi

tmpdir=$(clone_repository "v1.0.0" ${HOME}/local/repo/${pkg} https://github.com/jarro2783/cxxopts.git)

pushd ${build_dir}
cmake \
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=${install_prefix} \
	-DCXXOPTS_BUILD_EXAMPLES=OFF \
	${tmpdir}
make -j 8
make install
popd
rm -fr ${build_dir}

if [ -d "${tmpdir}" ] ; then rm -fr ${tmpdir} ; fi
