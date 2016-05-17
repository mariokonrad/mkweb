#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <cxxopts/cxxopts.hpp>
#include "system.hpp"
#include "config.hpp"
#include "fs_util.hpp"

namespace mkweb
{
static std::string read_meta_from_markdown(const std::string & path)
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

static void collect_information(const std::string & source_root_directory)
{
	namespace fs = boost::filesystem;

	const fs::path source_path{source_root_directory};

	if (!fs::exists(source_path) || !fs::is_directory(source_path))
		return;

	// TODO

	for (auto dir = fs::recursive_directory_iterator{source_path};
		 dir != fs::recursive_directory_iterator{}; ++dir) {
		const fs::path path = dir->path();
		if (!fs::is_regular_file(path))
			continue;

		std::cerr << path << '\n';
		/*
		filename = dirname + '/' + fn
		rel_filename = os.path.relpath(dirname, root_directory) + '/' + fn
		meta = read_meta(filename)

		# if a file does not provide metadata, there is nothing
		# we can do at this point
		if meta == None:
			continue

		# replace 'date' entry with datetime
		if 'date' in meta:
			meta['date'] = make_datetime(meta['date'])

		# read meta data and remember general file information
		info[rel_filename] = meta

		# collect plugins
		if 'plugins' in meta:
			for plugin in meta['plugins']:
				plugins.add(plugin)

		# check and extract tags
		if 'tags' in meta:
			for tag in meta['tags']:
				if not tag in tags:
					tags[tag] = []
				tags[tag].append({'filename':rel_filename, 'meta':meta})

		# check and extract date tags
		if 'date' in meta:
			d = meta['date']
			if d.year not in years:
				years[d.year] = []
			years[d.year].append({'filename':rel_filename, 'meta':meta})
		*/
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
