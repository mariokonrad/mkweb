#include <iostream>
#include <fstream>
#include <string>
#include <yaml-cpp/yaml.h>
#include "cxxopts/cxxopts.hpp"

int main(int argc, char ** argv)
{
	std::string meta_tag;

	// clang-format off
	cxxopts::Options options{argv[0], " - markdown meta information extractor"};
	options.add_options()
		("h,help",
			"shows help information")
		("read",
			"read data of a meta information",
			cxxopts::value<std::string>(meta_tag)
			)
		;
	// clang-format on

	options.parse(argc, argv);

	if (options.count("help")) {
		std::cout << options.help() << "\n";
		return 0;
	}

	if (options.count("read") == 0) {
		return -1;
	}

	// read meta information from stdin

	std::string contents;
	std::string line;
	while (std::getline(std::cin, line) && (line != "---"))
		;
	while (std::getline(std::cin, line) && (line != "---")) {
		contents += line + "\n";
	}

	// parse yaml and extract meta information

	try {
		auto const info = YAML::Load(contents);
		auto const meta = info[meta_tag];
		if (meta.IsSequence()) {
			for (auto const & i : meta)
				std::cout << i << "\n";
		} else {
			std::cout << meta << "\n";
		}
	} catch (...) {
		// ignore
	}

	return 0;
}
