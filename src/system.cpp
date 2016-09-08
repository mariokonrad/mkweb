#include "system.hpp"
#include <experimental/filesystem>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "config.hpp"

namespace
{
static std::string path_to_binary()
{
	// for a reason, not investigated further, this does not work at the moment,
	// therefore it has to be done manually and natively.
	//
	// namespace fs = std::filesystem;
	// return fs::read_symlink("/proc/self/exe").remove_filename().string();

	char path[PATH_MAX];
	if (::readlink("/proc/self/exe", path, sizeof(path)) < 0)
		throw std::system_error{errno, std::system_category(), "error in 'readlink'"};

	std::string p{path};
	return p.substr(0, p.find_last_of('/'));
}
}

namespace mkweb
{
namespace fs
{
using std::experimental::filesystem::exists;
}

std::shared_ptr<config> system::cfg_;
std::string system::pandoc_ = "pandoc";

void system::reset(const std::shared_ptr<config> & cfg) { cfg_ = cfg; }

config & system::cfg() { return *cfg_; }

std::string system::get_plugin_path(const std::string & plugin)
{
	return path_to_binary() + "/../shared/plugins/" + plugin + '/';
}

std::string system::get_plugin_config(const std::string & plugin)
{
	return get_plugin_path(plugin) + "files.yml";
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
	if (fs::exists(path))
		return path;
	return pb + "/../shared/themes/" + cfg_->get_theme() + '/';
}

std::string system::get_theme_template()
{
	return get_theme_path() + "template.html";
}

std::string system::get_theme_template_meta_tags()
{
	return get_theme_path() + "meta-tags.txt";
}

std::string system::get_theme_template_meta_years()
{
	return get_theme_path() + "meta-year.txt";
}

std::string system::get_theme_template_meta_contents()
{
	return get_theme_path() + "meta-contents.txt";
}

std::string system::get_theme_style()
{
	return get_theme_path() + "style.html";
}

std::string system::get_theme_footer()
{
	const auto path = get_theme_path() + "footer.html";
	return fs::exists(path) ? path : std::string{};
}

std::string system::get_theme_title_newest_entries()
{
	return get_theme_path() + "title_newest_entries.txt";
}

std::string system::pandoc() { return pandoc_; }

void system::set_pandoc(const std::string & path) { pandoc_ = path; }
}
