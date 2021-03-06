#include "config.hpp"
#include <algorithm>
#include <regex>
#include <yaml-cpp/yaml.h>

namespace mkweb
{
namespace
{
template <class Container>
static bool in(const typename Container::value_type & element, const Container & container)
{
	return std::find(begin(container), end(container), element) != end(container);
}
}

bool operator<(const config::sort_description & a, const config::sort_description & b)
{
	return std::tie(a.dir, a.key) < std::tie(b.dir, b.key);
}

config::~config()
{
}

config::config(const std::string & filename)
{
	node_ = std::make_unique<YAML::Node>(YAML::LoadFile(filename));
}

const YAML::Node & config::node() const
{
	return *node_;
}

std::string config::get_source() const
{
	return get_str("source", "pages");
}

std::string config::get_destination() const
{
	return get_str("destination", "public");
}

std::string config::get_static() const
{
	return get_str("static", "pages");
}

std::string config::get_plugin_path(const std::string & plugin) const
{
	auto path = get_str("plugins", {});
	if (path.size()) {
		path += "/" + plugin + "/";
	}
	return path;
}

std::string config::get_site_url() const
{
	return get_str("site_url", "");
}

std::string config::get_plugin_url(const std::string & plugin) const
{
	auto url = get_str("plugin_url", {});
	if (url.size()) {
		url += plugin + "/";
	}
	return url;
}

std::string config::get_site_title() const
{
	return get_str("site_title", "TITLE");
}

std::string config::get_site_subtitle() const
{
	return get_str("site_subtitle", "");
}

std::string config::get_language() const
{
	return get_str("language", "");
}

std::string config::get_author() const
{
	return get_str("author", "");
}

int config::get_num_news() const
{
	return get_int("num_news", 8);
}

std::vector<std::string> config::get_source_process_filetypes() const
{
	const auto & types = node()["source-process-filetypes"];

	std::vector<std::string> result;
	if (types && types.IsSequence()) {
		result.reserve(types.size());
		for (const auto & type : types) {
			result.push_back(type.as<std::string>());
		}
	} else {
		result.push_back(".md");
	}
	return result;
}

std::vector<config::path_map_entry> config::get_path_map() const
{
	const auto & pmap = node()["path_map"];

	std::vector<path_map_entry> entries;
	if (pmap && pmap.IsSequence()) {
		entries.reserve(pmap.size());
		for (const auto & entry : pmap) {
			path_map_entry t;
			if (entry["base"])
				t.base = entry["base"].as<std::string>();
			if (entry["url"])
				t.url = substitute_vars(entry["url"].as<std::string>());
			if (entry["absolute"])
				t.absolute = entry["absolute"].as<bool>();
			entries.push_back(t);
		}
	}
	return entries;
}

bool config::get_social_enable() const
{
	return get_bool("social-enable", false);
}

std::string config::get_social() const
{
	return get_str("social", "");
}

bool config::get_menu_enable() const
{
	return get_bool("menu-enable", false);
}

std::string config::get_menu() const
{
	return get_str("menu", "");
}

bool config::get_tags_enable() const
{
	return get_bool("tags-enable", false);
}

bool config::get_page_tags_enable() const
{
	return get_bool("page-tags-enable", false);
}

config::pagelist config::get_pagelist() const
{
	static const std::string group = "pagelist";

	return {get_bool(group, "enable", false), get_sort_description(group, "sort"),
		get_int(group, "num_entries", 0)};
}

config::yearlist config::get_yearlist() const
{
	static const std::string group = "yearlist";

	return {get_bool(group, "enable", false), get_sort_description(group, "sort")};
}

config::sitemap config::get_sitemap() const
{
	static const std::string group = "sitemap";

	return {get_bool(group, "enable", false), get_sort_description(group, "sort")};
}

config::sort_description config::get_sort_description(
	const std::string & group, const std::string & name) const
{
	static const std::vector<std::string> valid_directions = {"ascending", "descending"};
	static const std::vector<std::string> valid_keys = {"title", "date"};

	const auto & g = node()[group];
	const auto & pls = g ? g[name] : node()[name];

	sort_description result;

	if (pls) {
		if (pls["direction"]) {
			const auto s = pls["direction"].as<std::string>();
			if (in(s, valid_directions)) {
				if (s == "descending") {
					result.dir = sort_direction::descending;
				}
			}
		}
		if (pls["key"]) {
			const auto s = pls["key"].as<std::string>();
			if (in(s, valid_keys)) {
				result.key = s;
			}
		}
	}
	return result;
}

config::theme config::get_theme() const
{
	return {get_str("theme", "default"), get_grouped("theme-config", "site_title_background"),
		get_grouped("theme-config", "copyright")};
}

std::string config::get_node_str(
	const std::string & tag, const std::string & default_value) const
{
	return (node()[tag] && node()[tag].IsScalar()) ? node()[tag].as<std::string>()
												   : default_value;
}

std::string config::substitute_vars(const std::string & s) const
{
	static const std::regex variable_regex("\\$\\{[0-9a-zA-Z_]+\\}");

	std::string result;

	auto callback = [&](const std::string & m) {
		if (m.empty())
			return;

		if (m[0] != '$' || (m.size() < 3)) {
			result.append(m);
			return;
		}

		const auto substitute_tag = m.substr(2, m.size() - 3);
		const auto substitute_text = get_node_str(substitute_tag, "");
		result.append(substitute_text);
	};

	std::sregex_token_iterator begin(s.begin(), s.end(), variable_regex, {-1, 0});
	std::sregex_token_iterator end;
	std::for_each(begin, end, callback);

	return result;
}

std::string config::get_str(const std::string & tag, const std::string & default_value) const
{
	return substitute_vars(get_node_str(tag, default_value));
}

int config::get_int(const std::string & tag, int default_value) const
{
	return (node()[tag] && node()[tag].IsScalar()) ? node()[tag].as<int>() : default_value;
}

int config::get_int(const std::string & group, const std::string & tag, int default_value) const
{
	const auto & g = node()[group];
	if (!g)
		return get_int(tag, default_value);
	return (g[tag] && g[tag].IsScalar()) ? g[tag].as<int>() : default_value;
}

bool config::get_bool(const std::string & tag, bool default_value) const
{
	return (node()[tag] && node()[tag].IsScalar()) ? node()[tag].as<bool>() : default_value;
}

bool config::get_bool(
	const std::string & group, const std::string & tag, bool default_value) const
{
	const auto & g = node()[group];
	if (!g)
		return get_bool(tag, default_value);
	return (g[tag] && g[tag].IsScalar()) ? g[tag].as<bool>() : default_value;
}

std::string config::get_grouped(const std::string & group, const std::string & field,
	const std::string & default_value) const
{
	const auto & g = node()[group];
	if (!g)
		return default_value;

	return (g[field] && g[field].IsScalar()) ? g[field].as<std::string>() : default_value;
}
}
