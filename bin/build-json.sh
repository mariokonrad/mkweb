#!/bin/bash -eu

script_dir=$(dirname `readlink -f $0`)

if [ $# != 2 ] ; then
	exit 1
fi

pkg=json
repo_local=${HOME}/local/repo/${pkg}
repo_remote=https://github.com/nlohmann/json.git
branch="v2.1.1"

build_prefix=$1
build_dir=${build_prefix}/${pkg}
install_prefix=$2
repo_src=

if [ ! -d "${install_prefix}" ] ; then mkdir -p ${install_prefix} ; fi
if [ ! -d "${build_dir}" ] ; then mkdir -p ${build_dir} ; fi

tmpdir=
if [ -d "${repo_local}" ] ; then
	repo_src="${repo_local}"
else
	repo_src="${repo_remote}"
fi
tmpdir=$(mktemp -d)
pushd ${tmpdir}
git clone --depth 1 --branch ${branch} ${repo_src}
popd
repo_dir=${tmpdir}/${pkg}

pushd ${build_dir}
cmake \
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=${install_prefix} \
	-DBuildTests=OFF \
	${repo_dir}
make -j 8
make install
popd
rm -fr ${build_dir}

if [ -d "${tmpdir}" ] ; then rm -fr ${tmpdir} ; fi
