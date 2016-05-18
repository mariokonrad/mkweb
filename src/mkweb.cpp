#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <experimental/filesystem>
#include <cxxopts/cxxopts.hpp>
#include <yaml-cpp/yaml.h>
#include "system.hpp"
#include "config.hpp"
#include "fs_util.hpp"
#include "posix_time.hpp"

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
	namespace fs = std::experimental::filesystem;

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
	if (!path_exists(config_filename))
		throw std::runtime_error{"config file not readable: " + config_filename};
	if (config_pandoc.size()) {
		if (!path_exists(config_pandoc))
			throw std::runtime_error{"executable not found: " + config_pandoc};
		system::set_pandoc(config_pandoc);
	}

	// read configuration
	system::reset(std::make_shared<config>(config_filename));

	collect_information(system::cfg().get_source());

	return 0;
}
