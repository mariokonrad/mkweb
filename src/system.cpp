#include "system.hpp"
#include "config.hpp"
#include "fs_util.hpp"

namespace mkweb
{
std::shared_ptr<config> system::cfg_;
std::string system::pandoc_ = "pandoc";

void system::reset(const std::shared_ptr<config> & cfg) { cfg_ = cfg; }

config & system::cfg() { return *cfg_; }

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

std::string system::pandoc() { return pandoc_; }

void system::set_pandoc(const std::string & path) { pandoc_ = path; }
}
