#ifndef __MKWEB__CONFIG_HPP__
#define __MKWEB__CONFIG_HPP__

#include <memory>
#include <string>
#include <vector>

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

	struct sort_description {
		sort_direction dir = sort_direction::ascending;
		std::string key = "title";
	};

	struct theme {
		std::string type;
		std::string site_title_background;
		std::string copyright;
	};

	struct pagelist {
		bool enable;
		sort_description sorting;
	};

	struct yearlist {
		bool enable;
		sort_description sorting;
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

	pagelist get_pagelist() const;
	yearlist get_yearlist() const;
	theme get_theme() const;

private:
	std::unique_ptr<YAML::Node> node_;

	const YAML::Node & node() const;

	std::string get_node_str(const std::string & tag, const std::string & default_value) const;
	std::string substitue_vars(const std::string & s) const;

	std::string get_str(const std::string & tag, const std::string & default_value) const;

	int get_int(const std::string & tag, int default_value) const;

	bool get_bool(const std::string & tag, bool default_value) const;
	bool get_bool(const std::string & group, const std::string & tag, bool default_value) const;

	sort_description get_sort_description(
		const std::string & group, const std::string & name) const;

	std::string get_grouped(const std::string & group, const std::string & field,
		const std::string & default_value = "") const;
};

bool operator<(const config::sort_description &, const config::sort_description &);
}

#endif
