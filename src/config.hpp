#ifndef __MKWEB__CONFIG_HPP__
#define __MKWEB__CONFIG_HPP__

#include <string>
#include <vector>
#include <memory>

namespace YAML
{
class Node; // forward declaration
}

namespace mkweb
{
class config
{
public:
	struct path_map_entry {
		std::string base;
		std::string url;
		bool absolute = false;
	};

	enum class sort_direction { ascending, descending };

	struct pagelist_sort_desc {
		sort_direction dir;
		std::string key;
	};

	~config();

	config(const std::string & filename);

	config(const config &) = delete;
	config & operator=(const config &) = delete;

	config(config &&) = default;
	config & operator=(config &&) = default;

	std::string get_source() const;
	std::string get_destination() const;
	std::string get_static() const;

	std::string get_plugin_path(const std::string & plugin = "") const;

	std::string get_site_url() const;
	std::string get_plugin_url(const std::string & plugin = "") const;
	std::string get_site_title() const;
	std::string get_site_subtitle() const;
	std::string get_language() const;
	std::string get_theme() const;
	std::string get_author() const;
	int get_num_news() const;

	std::vector<std::string> get_source_process_filetypes() const;

	std::vector<path_map_entry> get_path_map() const;

	bool get_social_enable() const;
	std::string get_social() const;

	bool get_menu_enable() const;
	std::string get_menu() const;

	bool get_tags_enable() const;
	bool get_page_tags_enable() const;
	bool get_years_enable() const;
	bool get_pagelist_enable() const;

	pagelist_sort_desc get_pagelist_sort() const;

	std::string get_site_title_background() const;
	std::string get_copyright() const;

private:
	std::unique_ptr<YAML::Node> node_;

	const YAML::Node & node() const;

	std::string get_str(const std::string & tag, const std::string & default_value) const;
	int get_int(const std::string & tag, int default_value) const;
	bool get_bool(const std::string & tag, bool default_value) const;
	std::string get_themed(const std::string field, const std::string & default_value = "") const;
};
}

#endif
