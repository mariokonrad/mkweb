#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <experimental/filesystem>
#include <cxxopts/cxxopts.hpp>
#include <yaml-cpp/yaml.h>
#include "system.hpp"
#include "config.hpp"
#include "posix_time.hpp"

namespace std
{
namespace filesystem = experimental::filesystem;
}

namespace mkweb
{
struct meta_info
{
	posix_time date;
	std::string title;
	std::vector<std::string> authors;
	std::vector<std::string> tags;
	std::string language;
	std::string summary;
	std::vector<std::string> plugins;
};

static struct {
	std::unordered_map<std::string, meta_info> meta;
	std::unordered_set<std::string> plugins;
	std::unordered_map<std::string, std::vector<std::string>> tags;
	std::unordered_map<uint32_t, std::vector<std::string>> years;

	std::string tag_list;
	std::string year_list;
	std::string page_list;
} global;

static std::string read_meta_string_from_markdown(const std::string & path)
{
	std::ifstream ifs{path.c_str()};

	std::string line;
	while (std::getline(ifs, line) && (line != "---"))
		;
	std::string contents;
	while (std::getline(ifs, line) && (line != "---")) {
		contents += line + '\n';
	}
	return contents;
}

template <class Container>
static void collect(const YAML::Node & node, Container & c)
{
	if (!node)
		return;

	if (node.IsScalar()) {
		c.push_back(node.as<std::string>());
	} else if (node.IsSequence()) {
		for (const auto & entry : node) {
			c.push_back(entry.as<std::string>());
		}
	}
}

static void collect(const YAML::Node & node, std::string & s)
{
	if (!node)
		return;

	if (node.IsScalar())
		s = node.as<std::string>();
}

static meta_info read_meta(const std::string & path)
{
	// TODO: handle different file types besides markdown, depending on extension

	const auto txt = read_meta_string_from_markdown(path);
	const auto doc = YAML::Load(txt);

	meta_info info;

	if (!doc["title"])
		throw std::runtime_error{"essential information missing: 'title'"};

	collect(doc["title"], info.title);
	collect(doc["author"], info.authors);
	collect(doc["tags"], info.tags);
	collect(doc["plugins"], info.plugins);
	collect(doc["language"], info.language);
	collect(doc["summary"], info.summary);

	if (doc["date"]) {
		info.date = posix_time::from_string(doc["date"].as<std::string>());
	} else {
		info.date = posix_time::from_string("2000-01-01 00:00");
	}

	return info;
}

static void collect_information(const std::string & source_root_directory)
{
	namespace fs = std::filesystem;

	const fs::path source_path{source_root_directory};

	if (!fs::exists(source_path) || !fs::is_directory(source_path))
		return;

	for (auto it = fs::recursive_directory_iterator{source_path};
		 it != fs::recursive_directory_iterator{}; ++it) {

		// TODO: handle absolute paths?

		if (!fs::is_regular_file(it->path()))
			continue;

		try {
			const auto path = it->path().string();
			const auto info = read_meta(path);

			global.meta[path] = info;
			for (const auto & plugin : info.plugins)
				global.plugins.insert(plugin);
			for (const auto & tag : info.tags)
				global.tags[tag].push_back(path);
			global.years[info.date.year()].push_back(path);

		} catch (...) {
			// no relevant meta data found for file, there is nothing to be done
			continue;
		}
	}
}

static std::string prepare_tag_list(std::vector<std::string> tags)
{
	const auto site_url = system::cfg().get_site_url();

	std::sort(begin(tags), end(tags));

	std::ostringstream os;
	os << "<ul>";
	for (const auto & tag : tags) {
		os << "<li><a href=\"" << site_url << "tag/" << tag << ".html\">" << tag << "</a></li>";
	}
	os << "</ul>";
	return os.str();
}

static std::string prepare_global_tag_list(
	const std::unordered_map<std::string, std::vector<std::string>> & tags)
{
	std::vector<std::string> ids;
	ids.reserve(tags.size());
	for (const auto & tag : tags)
		ids.push_back(tag.first);
	return prepare_tag_list(ids);
}

static std::string prepare_global_year_list(
	const std::unordered_map<uint32_t, std::vector<std::string>> & years)
{
	const auto site_url = system::cfg().get_site_url();

	std::vector<uint32_t> ids;
	ids.reserve(years.size());
	for (const auto & year : years)
		ids.push_back(year.first);

	std::sort(rbegin(ids), rend(ids));

	std::ostringstream os;
	os << "<br>";
	for (const auto year : ids) {
		os << "<a href=\"" << site_url << "year/" << year << ".html\">" << year << "</a>";
		os << ' ';
	}
	return os.str();
}

template <class Container>
static bool contains(
	const typename Container::value_type & element, const Container & container)
{
	return std::find(begin(container), end(container), element) != end(container);
}

static std::string convert_path(const std::string & fn)
{
	std::filesystem::path path{fn};
	if (contains(path.extension().string(), system::cfg().get_source_process_filetypes())) {
		path.replace_extension(".html");
	}
	return path.string();
}

static std::string prepare_global_pagelist(
	const std::unordered_map<std::string, meta_info> & meta)
{
	const auto sort_desc = system::cfg().get_pagelist_sort();

	// extract sorting criteria, build map key->filename
	std::vector<std::pair<std::string, std::string>> ids;
	if (sort_desc.key == "date") {
		for (const auto & fn : meta) {
			ids.push_back(std::make_pair(fn.second.date.str(), fn.first));
		}
	} else if (sort_desc.key == "title") {
		for (const auto & fn : meta) {
			ids.push_back(std::make_pair(fn.second.title, fn.first));
		}
	} else {
		throw std::runtime_error{"sort key not supported: " + sort_desc.key};
	}

	// sort ids
	switch (sort_desc.dir) {
		case config::sort_direction::ascending:
			std::sort(begin(ids), end(ids),
				[](const auto & a, const auto & b) { return a.first < b.first; });
			break;
		case config::sort_direction::descending:
			std::sort(begin(ids), end(ids),
				[](const auto & a, const auto & b) { return a.first > b.first; });
			break;
	}

	// generate HTML list of sorted entries
	const auto site_url = system::cfg().get_site_url();
	std::ostringstream os;
	os << "<ul>";
	for (const auto & entry : ids) {
		const auto filename = entry.second;
		const auto title = meta.find(filename)->second.title; // single-thread, still valid
		os << "<li><a href=\"" << site_url << convert_path(filename) << "\">" << title
		   << "</a></li>";
	}
	os << "</ul>";
	return os.str();
}
}

int main(int argc, char ** argv)
{
	// command line paramater handling

	std::string config_filename = "config.yml";
	std::string config_pandoc = "";

	// clang-format off
	cxxopts::Options options{argv[0], " - configuration read demo"};
	options.add_options()
		("h,help",
			"shows help information")
		("c,config",
			"read config from the specified file",
			cxxopts::value<std::string>(config_filename))
		("pandoc",
			"specify pandoc binary to use",
			cxxopts::value<std::string>(config_pandoc))
		;
	// clang-format on

	options.parse(argc, argv);

	if (options.count("help")) {
		std::cout << options.help() << '\n';
		return 0;
	}

	using namespace mkweb;

	// validation
	if (!std::filesystem::exists(config_filename))
		throw std::runtime_error{"config file not readable: " + config_filename};
	if (config_pandoc.size()) {
		if (!std::filesystem::exists(config_pandoc))
			throw std::runtime_error{"executable not found: " + config_pandoc};
		system::set_pandoc(config_pandoc);
	}

	// read configuration
	system::reset(std::make_shared<config>(config_filename));

	// collect and prepare information
	collect_information(system::cfg().get_source());
	global.tag_list = prepare_global_tag_list(global.tags);
	global.year_list = prepare_global_year_list(global.years);
	global.page_list = prepare_global_pagelist(global.meta);

	// TODO: generate site

	// TODO: copy files

	// TODO: copy plugins

	return 0;
}
