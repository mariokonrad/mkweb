#include <iostream>
#include <algorithm>
#include <string>
#include <map>
#include <stdexcept>
#include "json/json.hpp"
#include "cxxopts/cxxopts.hpp"

using json = nlohmann::json;

static std::map<std::string, std::string> roots;

static std::string replace_root(const std::string & link)
{
	auto pos_root = link.find_first_of("/", 0);
	auto root = link.substr(0, pos_root);
	auto rest = link.substr(pos_root + 1);

	const auto h = roots.find("home");
	if (h == roots.end())
		throw std::runtime_error{"home not specified"};

	const auto i = roots.find(root);
	if (i == roots.end())
		throw std::runtime_error{"specified root not impelemented"};

	return h->second + i->second + rest;
}

static void handle_link(json & data)
{
	auto i = data.find("c");
	if (i == data.end())
		return;
	if (!i->is_array())
		return;

	auto & link = (*i)[1][0];
	if (!link.is_string())
		return;

	link = replace_root(link);
}

static void recursrive_search_links(json & data)
{
	switch (data.type()) {
		case json::value_t::object: {
			auto i = data.find("t");
			if (i != data.end() && *i == "Link") {
				handle_link(data);
			} else {
				for (auto & p : data)
					recursrive_search_links(p);
			}
		} break;
		case json::value_t::array:
			for (auto & v : data)
				recursrive_search_links(v);
			break;

		default:
			break;
	}
}

int main(int argc, char ** argv)
{
	std::string location_home{""};
	std::string location_pages{""};
	std::string location_files{""};
	std::string location_images{""};

	// clang-format off
	cxxopts::Options options{argv[0], " - link filter"};
	options.add_options()
		("h,help",
			"shows help information")
		("home",
			"URL to the website",
			cxxopts::value<std::string>(location_home)->default_value(location_home)
			)
		("pages",
			"path to generated pages, relative to the home path",
			cxxopts::value<std::string>(location_pages)->default_value(location_pages)
			)
		("files",
			"path to files, relative to the home path",
			cxxopts::value<std::string>(location_files)->default_value(location_files)
			)
		("images",
			"path to images, relative to the home path",
			cxxopts::value<std::string>(location_images)->default_value(location_images)
			)
		("replace-links",
			"replaces links with the configured root directories"
			)
		;
	// clang-format on

	options.parse(argc, argv);

	if (options.count("help")) {
		std::cout << options.help() << "\n";
		return 0;
	}

	roots.emplace("home", location_home);
	roots.emplace("pages", location_pages);
	roots.emplace("files", location_files);
	roots.emplace("images", location_images);

	std::string s;
	std::noskipws(std::cin);
	std::copy(std::istream_iterator<char>{std::cin}, std::istream_iterator<char>{},
		std::back_inserter(s));

	if (options.count("replace-links")) {
		auto data = json::parse(s);
		recursrive_search_links(data);
		std::cout << data;
	}

	return 0;
}
