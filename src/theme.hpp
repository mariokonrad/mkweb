#ifndef MKWEB__THEME__HPP
#define MKWEB__THEME__HPP

#include <string>

namespace mkweb
{
class theme
{
	friend class system;

public:
	std::string get_template() const;
	std::string get_template_meta_tags() const;
	std::string get_template_meta_years() const;
	std::string get_template_meta_contents() const;
	std::string get_template_meta_sitemap() const;
	std::string get_style() const;
	std::string get_footer() const;
	std::string get_title_newest_entries() const;

private:
	const std::string path;

	theme(const std::string & path);
};
}

#endif
