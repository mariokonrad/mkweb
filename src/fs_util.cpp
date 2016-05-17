#include "fs_util.hpp"
#include <system_error>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <boost/filesystem.hpp>

namespace mkweb
{
std::string path_to_binary()
{
	char path[PATH_MAX];
	if (::readlink("/proc/self/exe", path, sizeof(path)) < 0)
		throw std::system_error{errno, std::system_category(), "error in 'readlink'"};

	std::string p{path};
	return p.substr(0, p.find_last_of('/'));
}

bool path_exists(const std::string & path)
{
	/*
	struct stat st;
	if (::stat(path.c_str(), &st) < 0) {
		if (errno == ENOENT)
			return false;
		throw std::system_error{
			errno, std::system_category(), "error in 'stat' (" + path + ')'};
	}

	return S_ISREG(st.st_mode) || S_ISDIR(st.st_mode);
	*/

	namespace fs = boost::filesystem;

	const fs::path p(path);
	return fs::exists(p) && (fs::is_directory(p) || fs::is_regular_file(p));
}
}
