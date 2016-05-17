#include <fstream>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "cxxopts.hpp"

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

	config(const std::string & filename) { node_ = YAML::LoadFile(filename); }

	std::string get_source() const { return get_str("source", "pages"); }
	std::string get_destination() const { return get_str("destination", "pages"); }
	std::string get_static() const { return get_str("static", "pages"); }

	std::string get_plugin_path(const std::string & plugin = "") const
	{
		auto path = get_str("plugins", {});
		if (path.size()) {
			path += "/" + plugin + "/";
		}
		return path;
	}

	std::string get_site_url() const { return get_str("site_url", "?"); }
	std::string get_plugin_url() const { return get_str("plugin_url", ""); }
	std::string get_site_title() const { return get_str("site_title", "TITLE"); }
	std::string get_site_subtitle() const { return get_str("site_subtitle", ""); }
	std::string get_language() const { return get_str("language", ""); }
	std::string get_theme() const { return get_str("theme", "default"); }
	std::string get_author() const { return get_str("author", "?"); }
	int get_num_news() const { return get_int("num_news", 8); }

	std::vector<std::string> get_source_process_filetypes() const
	{
		const auto & types = node_["source-process-filetypes"];

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

	std::vector<path_map_entry> get_path_map() const
	{
		const auto & pmap = node_["path_map"];

		std::vector<path_map_entry> entries;
		if (pmap && pmap.IsSequence()) {
			entries.reserve(pmap.size());
			for (const auto & entry : pmap) {
				path_map_entry t;
				if (entry["base"])
					t.base = entry["base"].as<std::string>();
				if (entry["url"])
					t.url = entry["url"].as<std::string>();
				if (entry["absolute"])
					t.absolute = entry["absolute"].as<bool>();
				entries.push_back(t);
			}
		}
		return entries;
	}

	bool get_social_enable() const { return get_bool("social-enable", false); }
	std::string get_social() const { return get_str("social", ""); }

	bool get_menu_enable() const { return get_bool("menu-enable", false); }
	std::string get_menu() const { return get_str("menu", ""); }

	bool get_tags_enable() const { return get_bool("tags-enable", false); }
	bool get_page_tags_enable() const { return get_bool("page-tags-enable", false); }
	bool get_years_enable() const { return get_bool("years-enable", false); }
	bool get_pagelist_enable() const { return get_bool("pagelist-enable", false); }

	pagelist_sort_desc get_pagelist_sort() const
	{
		static const std::vector<std::string> valid_directions = {"ascending", "descending"};
		static const std::vector<std::string> valid_keys = {"title", "date"};

		const auto & pls = node_["pagelist-sort"];

		pagelist_sort_desc result{ sort_direction::ascending, "title" };

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

	std::string get_site_title_background() const
	{
		return get_themed("site_title_background");
	}

	std::string get_copyright() const { return get_themed("copyright"); }

private:
	YAML::Node node_;

	template <class Container>
	static bool in(const typename Container::value_type & element, const Container & container)
	{
		return std::find(begin(container), end(container), element) != end(container);
	}

	std::string get_str(const std::string & tag, const std::string & default_value) const
	{
		return (node_[tag] && node_[tag].IsScalar()) ? node_[tag].as<std::string>()
													 : default_value;
	}

	int get_int(const std::string & tag, int default_value) const
	{
		return (node_[tag] && node_[tag].IsScalar()) ? node_[tag].as<int>() : default_value;
	}

	bool get_bool(const std::string & tag, bool default_value) const
	{
		return (node_[tag] && node_[tag].IsScalar()) ? node_[tag].as<bool>() : default_value;
	}

	std::string get_themed(const std::string field, const std::string & default_value = "") const
	{
		// TODO
		return default_value;
	}
};
}

int main(int argc, char ** argv)
{
	std::string config_filename = "config.yml";

	// clang-format off
	cxxopts::Options options{argv[0], " - configuration read demo"};
	options.add_options()
		("h,help",
			"shows help information")
		("c,config",
			"read config from the specified file",
			cxxopts::value<std::string>(config_filename))
		;
	// clang-format on

	options.parse(argc, argv);

	if (options.count("help")) {
		std::cout << options.help() << '\n';
		return 0;
	}

	mkweb::config cfg(config_filename);

	std::cout << "source               =[" << cfg.get_source() << "]\n";
	std::cout << "destination          =[" << cfg.get_destination() << "]\n";
	std::cout << "static               =[" << cfg.get_static() << "]\n";
	std::cout << "plugins              =[" << cfg.get_plugin_path() << "]\n";
	std::cout << "site url             =[" << cfg.get_site_url() << "]\n";
	std::cout << "plugin url           =[" << cfg.get_plugin_url() << "]\n";
	std::cout << "site_title           =[" << cfg.get_site_title() << "]\n";
	std::cout << "site_subtitle        =[" << cfg.get_site_subtitle() << "]\n";
	std::cout << "language             =[" << cfg.get_language() << "]\n";
	std::cout << "theme                =[" << cfg.get_theme() << "]\n";
	std::cout << "author               =[" << cfg.get_author() << "]\n";
	std::cout << "num_news             =[" << cfg.get_num_news() << "]\n";

	std::cout << "source-process-filetypes:";
	for (const auto & t : cfg.get_source_process_filetypes())
		std::cout << " " << t;
	std::cout << '\n';

	std::cout << "path_map:\n";
	for (const auto & t : cfg.get_path_map()) {
		std::cout << "- {base=" << t.base << ", url=" << t.url << ", absolute=" << t.absolute
				  << "}\n";
	}

	std::cout << "social-enable        =[" << cfg.get_social_enable() << "]\n";
	std::cout << "social               =[" << cfg.get_social() << "]\n";
	std::cout << "menu-enable          =[" << cfg.get_menu_enable() << "]\n";
	std::cout << "menu                 =[" << cfg.get_menu() << "]\n";
	std::cout << "tags-enable          =[" << cfg.get_tags_enable() << "]\n";
	std::cout << "page-tags-enable     =[" << cfg.get_page_tags_enable() << "]\n";
	std::cout << "years-enable         =[" << cfg.get_years_enable() << "]\n";
	std::cout << "pagelist-enable      =[" << cfg.get_pagelist_enable() << "]\n";

	const auto pagelist_sort = cfg.get_pagelist_sort();
	std::cout << "pagelist-sort        = dir=" << static_cast<int>(pagelist_sort.dir)
			  << ", key=" << pagelist_sort.key << '\n';

	std::cout << "site-title-background=[" << cfg.get_site_title_background() << "]\n";
	std::cout << "copyright            =[" << cfg.get_copyright() << "]\n";

	return EXIT_SUCCESS;
}
