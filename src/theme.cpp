#include "theme.hpp"
#include <experimental/filesystem>

namespace mkweb
{
namespace fs
{
using std::experimental::filesystem::exists;
}

theme::theme(const std::string & path)
	: path(path)
{
}

std::string theme::get_template() const
{
	return path + "template.html";
}

std::string theme::get_template_meta_tags() const
{
	return path + "meta-tags.txt";
}

std::string theme::get_template_meta_years() const
{
	return path + "meta-year.txt";
}

std::string theme::get_template_meta_contents() const
{
	return path + "meta-contents.txt";
}

std::string theme::get_template_meta_sitemap() const
{
	return path + "meta-sitemap.txt";
}

std::string theme::get_style() const
{
	return path + "style.html";
}

std::string theme::get_footer() const
{
	const auto p = path + "footer.html";
	return fs::exists(p) ? p : std::string{};
}

std::string theme::get_title_newest_entries() const
{
	return path + "title_newest_entries.txt";
}
}
