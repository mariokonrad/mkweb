#include "system.hpp"
#include <experimental/filesystem>
#include <cerrno>
#include <unistd.h>
#include <linux/limits.h>
#include "config.hpp"
#include "version.hpp"

namespace mkweb
{
namespace fs
{
using std::experimental::filesystem::exists;
}

std::shared_ptr<config> system::cfg_;
std::string system::pandoc_ = "pandoc";

std::string system::path_to_binary()
{
	// for a reason, not investigated further, this does not work at the moment,
	// therefore it has to be done manually and natively.
	//
	// namespace fs = std::filesystem;
	// return fs::read_symlink("/proc/self/exe").remove_filename().string();

	char path[PATH_MAX];
	if (::readlink("/proc/self/exe", path, sizeof(path)) < 0)
		throw std::system_error{errno, std::system_category(), "error in 'readlink'"};

	const std::string p{path};
	return p.substr(0, p.find_last_of('/'));
}

std::string system::path_to_shared()
{
	return path_to_binary() + "/../shared/" + mkweb::project_name + '/';
}

void system::reset(const std::shared_ptr<config> & cfg)
{
	cfg_ = cfg;
}

config & system::cfg()
{
	return *cfg_;
}

plugin system::get_plugin(const std::string & name)
{
	return plugin{path_to_shared() + "plugins/" + name + '/'};
}

std::string system::get_sitemap_filename()
{
	return "sitemap.html";
}

theme system::get_theme()
{
	return theme{get_theme_path()};
}

std::string system::get_theme_path()
{
	const std::string ps = path_to_shared();
	std::string path
		= ps + "themes/" + cfg().get_theme().type + '.' + cfg().get_language() + '/';
	if (fs::exists(path))
		return path;
	return ps + "themes/" + cfg().get_theme().type + '/';
}

std::string system::pandoc()
{
	return pandoc_;
}

void system::set_pandoc(const std::string & path)
{
	pandoc_ = path;
}
}
