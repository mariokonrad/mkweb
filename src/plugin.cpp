#include "plugin.hpp"

namespace mkweb
{
plugin::plugin(const std::string & path)
	: path(path)
{
}

std::string plugin::get_path() const
{
	return path;
}

std::string plugin::get_config() const
{
	return path + "files.yml";
}

std::string plugin::get_style() const
{
	return path + "style.html";
}
}
