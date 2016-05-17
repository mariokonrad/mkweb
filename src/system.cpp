#include "system.hpp"
#include <system_error>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include "config.hpp"

namespace mkweb
{
namespace
{
static std::string path_to_binary()
{
	char path[PATH_MAX];
	if (::readlink("/proc/self/exe", path, sizeof(path)) < 0)
		throw std::system_error{errno, std::system_category(), "error in 'readlink'"};

	std::string p{path};
	return p.substr(0, p.find_last_of('/'));
}

static bool path_exists(const std::string & path)
{
	struct stat st;
	if (::stat(path.c_str(), &st) < 0) {
		if (errno == ENOENT)
			return false;
		throw std::system_error{
			errno, std::system_category(), "error in 'stat' (" + path + ')'};
	}

	return S_ISREG(st.st_mode) || S_ISDIR(st.st_mode);
}
}

std::shared_ptr<config> system::cfg_;

void system::reset(const std::shared_ptr<config> & cfg) { cfg_ = cfg; }

std::string system::get_plugin_path(const std::string & plugin)
{
	return path_to_binary() + "/../shared/plugins/" + plugin + '/';
}

std::string system::get_plugin_style(const std::string & plugin)
{
	return get_plugin_path(plugin) + "style.html";
}

std::string system::get_theme_path()
{
	const std::string pb = path_to_binary();
	std::string path
		= pb + "/../shared/themes/" + cfg_->get_theme() + '.' + cfg_->get_language() + '/';
	if (path_exists(path))
		return path;
	return pb + "/../shared/themes/" + cfg_->get_theme() + '/';
}

std::string system::get_theme_template()
{
	return get_theme_path() + "template.html";
}

std::string system::get_theme_style()
{
	return get_theme_path() + "style.html";
}

std::string system::get_theme_footer()
{
	const auto path = get_theme_path() + "footer.html";
	return path_exists(path) ? path : std::string{};
}
}
