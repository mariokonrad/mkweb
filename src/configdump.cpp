#include "cxxopts.hpp"
#include "config.hpp"

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
