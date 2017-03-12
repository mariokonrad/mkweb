#!/bin/bash -eu

function clone_repository()
{
	branch=$1
	repo_local=$2
	repo_remote=$3

	tmpdir=
	if [ -d "${repo_local}" ] ; then
		repo_src="${repo_local}"
	else
		repo_src="${repo_remote}"
	fi

	tmpdir=$(mktemp -d)
	#pushd ${tmpdir}
	git clone --depth 1 --branch ${branch} ${repo_src} ${tmpdir}
	#popd

	echo "${tmpdir}"
}
