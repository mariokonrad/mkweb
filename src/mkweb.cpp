#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <experimental/optional>
#include <experimental/filesystem>
#include <cxxopts/cxxopts.hpp>
#include <yaml-cpp/yaml.h>
#include <json/json.hpp>
#include <fmt/format.h>
#include "system.hpp"
#include "config.hpp"
#include "posix_time.hpp"
#include "subprocess.hpp"

namespace std
{
namespace filesystem = experimental::filesystem;
}

namespace mkweb
{
struct meta_info {
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

static std::experimental::optional<meta_info> get_meta_for_source(
	const std::string & filename_in)
{
	const auto it = global.meta.find(filename_in);
	if (it != global.meta.end())
		return it->second;
	return {};
}

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

template <class Container> static void collect(const YAML::Node & node, Container & c)
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
	if (tags.size() == 0)
		return std::string{};

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
		return path.replace_extension(".html").string();
	}
	return std::string{};
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

static std::string prepare_page_tag_list(const std::string & filename_in)
{
	const auto meta = get_meta_for_source(filename_in);
	if (!meta)
		return std::string{};
	return prepare_tag_list(meta->tags);
}

static bool conversion_necessary(
	const std::string & filename_in, const std::string & filename_out)
{
	namespace fs = std::filesystem;

	if (!fs::exists(filename_in))
		return false;
	if (!fs::exists(filename_out))
		return true;

	const auto mtime_out = fs::last_write_time(filename_out);
	if (mtime_out < fs::last_write_time(filename_in))
		return true;
	if (mtime_out < fs::last_write_time(system::get_theme_template()))
		return true;
	if (!system::get_theme_style().empty()
		&& (mtime_out < fs::last_write_time(system::get_theme_style())))
		return true;
	if (!system::get_theme_footer().empty()
		&& (mtime_out < fs::last_write_time(system::get_theme_footer())))
		return true;

	return false;
}

static bool ensure_path(const std::string & filename)
{
	namespace fs = std::filesystem;

	if (filename.empty())
		return true;

	const auto path = fs::path{filename}.remove_filename();
	if (fs::exists(path))
		return fs::is_directory(path);
	return fs::create_directories(path);
}

static std::string read_json_str_from_document(const std::string & path)
{
	utils::subprocess p{{system::pandoc(), "-t", "json", path}};
	std::ostringstream os;

	p.exec();
	p.out() >> std::noskipws;
	std::copy(std::istream_iterator<char>{p.out()}, std::istream_iterator<char>{},
		std::ostream_iterator<char>{os});
	p.wait();
	return os.str();
}

static bool write_document_from_json(
	const std::vector<std::string> & params, const std::string & content)
{
	utils::subprocess p{params};
	std::ostringstream os;

	p.exec();

	p.in() << content;
	p.close_in();

	p.err() >> std::noskipws;
	std::copy(std::istream_iterator<char>{p.err()}, std::istream_iterator<char>{},
		std::ostream_iterator<char>{os});
	auto rc = p.wait();

	return (rc == 0) && (os.tellp() == 0);
}

static std::vector<std::string> split_path(const std::string & path)
{
	std::filesystem::path p{path};
	return std::vector<std::string>{p.begin(), p.end()};
}

static std::string join_path(const std::vector<std::string> & parts)
{
	// std::experimental::filesystem::path(InputIterator, InputIterator)
	// does not work at the moment, therefore it needs to be done maually

	std::ostringstream os;
	auto i = parts.begin();
	os << *i;
	++i;
	for (; i != parts.end(); ++i) {
		os << std::filesystem::path::preferred_separator << *i;
	}
	return os.str();
}

static std::string replace_root(const std::string & link)
{
	auto parts = split_path(link);

	if (parts.size() < 2)
		return link;

	const auto path_map = system::cfg().get_path_map();
	const auto entry = std::find_if(
		begin(path_map), end(path_map), [&](const auto & a) { return a.base == parts[0]; });

	if (entry == end(path_map))
		return link;

	if (entry->absolute) {
		parts[0] = entry->url;
	} else {
		parts[0] = system::cfg().get_site_url() + entry->url;
	}
	return join_path(parts);
}

static void handle_link(nlohmann::json & data)
{
	auto i = data.find("c");
	if (i == data.end())
		return;
	if (!i->is_array())
		return;
	if (i->empty())
		return;

	auto & link = (*i)[i->size() - 1][0];
	if (!link.is_string())
		return;

	link = replace_root(link);
}

static void fix_links_recursive(nlohmann::json & data)
{
	if (data.is_array()) {
		for (auto & item : data) {
			fix_links_recursive(item);
		}
		return;
	}

	if (data.is_object()) {
		auto i = data.find("t");
		if (i != data.end()) {
			if (*i == "Para") {
				fix_links_recursive(data["c"]);
				return;
			}
			if (*i == "Link") {
				handle_link(data);
				return;
			}
			if (*i == "Image") {
				handle_link(data);
				return;
			}
		}
		for (auto & item : data) {
			fix_links_recursive(item);
		}
	}
}

template <class Container>
static void append(Container & v, std::initializer_list<typename Container::value_type> l)
{
	for (const auto & item : l)
		v.push_back(item);
}

static std::string make_plugin_script_string(
	const std::string & plugin, const std::string & filename)
{
	return "<script type=\"text/javascript\" src=\"" + system::cfg().get_plugin_path(plugin)
		+ filename + "\"></script>";
}

static std::string create_header_for_plugin(const std::string & plugin)
{
	std::ostringstream os;
	auto cfg = YAML::LoadFile(system::get_plugin_config(plugin));
	if (cfg["include"]) {
		for (const auto & entry : cfg["include"]) {
			os << make_plugin_script_string(plugin, entry.as<std::string>());
		}
	}
	return os.str();
}

static std::vector<std::string> prepare_pandoc_params(const std::string & filename_in,
	const std::string & filename_out, const std::string & tags_list)
{
	// clang-format off
	std::vector<std::string> params {
		system::pandoc(),
		"-f", "json",
		"-t", "html5",
		"-o", filename_out,
		"-H", system::get_theme_style(),
		"-M", "title-prefix=" + system::cfg().get_site_title(),
		"-V", "siteurl=" + system::cfg().get_site_url(),
		"-V", "sitetitle=" + system::cfg().get_site_title(),
		"--template", system::get_theme_template(),
		"--standalone",
		"--preserve-tabs",
		"--toc", "--toc-depth=2",
		"--mathml"
	};
	// clang-format on

	if (!system::get_theme_footer().empty())
		append(params, {"-A", system::get_theme_footer()});
	if (!system::cfg().get_site_subtitle().empty())
		append(params, {"-V", "sitesubtitle=" + system::cfg().get_site_subtitle()});
	if (system::cfg().get_tags_enable())
		append(params, {"-V", "globaltags=" + global.tag_list});
	if (system::cfg().get_years_enable())
		append(params, {"-V", "globalyears=" + global.year_list});
	if (system::cfg().get_social_enable())
		append(params, {"-V", "social=" + system::cfg().get_social()});
	if (system::cfg().get_menu_enable())
		append(params, {"-V", "menu=" + system::cfg().get_menu()});
	if (system::cfg().get_page_tags_enable() && !tags_list.empty())
		append(params, {"-V", "pagetags=" + tags_list});
	if (system::cfg().get_pagelist_enable() && !global.page_list.empty())
		append(params, {"-V", "globalpagelist=" + global.page_list});

	const auto meta = get_meta_for_source(filename_in);
	if (meta) {
		for (const auto & plugin : meta->plugins) {
			append(params, {"-H", system::get_plugin_style(plugin)});
			append(params, {"-V", "header-string=" + create_header_for_plugin(plugin)});
		}
	}

	// theme specific stuff
	if (!system::cfg().get_site_title_background().empty())
		append(params,
			{"-V", "sitetitle-background=" + system::cfg().get_site_title_background()});
	if (!system::cfg().get_copyright().empty())
		append(params, {"-V", "copyright=" + system::cfg().get_copyright()});

	return params;
}

static void process_document(const std::string & filename_in, const std::string & filename_out,
	const std::string & tags_list = std::string{})
{
	if (!conversion_necessary(filename_in, filename_out)) {
		std::cout << "skip    " << filename_out << '\n';
		return;
	}

	std::cout << "        " << filename_out << '\n';
	ensure_path(filename_out);

	// conversion from source file to JSON and processing
	auto content = nlohmann::json::parse(read_json_str_from_document(filename_in));
	fix_links_recursive(content);

	// perform final conversion to HTML
	const auto params = prepare_pandoc_params(filename_in, filename_out, tags_list);
	const auto success = write_document_from_json(params, content.dump());
	if (!success)
		throw std::runtime_error{"unable to write file: " + filename_out};
}

static void process_single(const std::string & source_directory,
	const std::string & destination_directory, const std::string & filename_in)
{
	auto filename_out = convert_path(filename_in);

	if (!filename_out.empty()) {
		filename_out = destination_directory + filename_out.substr(source_directory.size());
		process_document(filename_in, filename_out, prepare_page_tag_list(filename_in));
	} else {
		std::cout << "ignore: " << filename_in << '\n';
	}
}

static bool is_subdir(const std::string & subdirectory, const std::string & directory)
{
	/* TODO
	path = os.path.realpath(path)
	directory = os.path.realpath(directory)
	relative = os.path.relpath(path, directory)
	return not (relative == os.pardir or relative.startswith(os.pardir + os.sep))
	*/
	return false;
}

static void process_pages(const std::string source_directory,
	const std::string & destination_directory, const std::string & specific_dir = std::string{})
{
	if (!specific_dir.empty()) {
		if (!is_subdir(specific_dir, source_directory))
			throw std::runtime_error{
				"error: " + specific_dir + " is not a subdir of " + source_directory};
	}

	namespace fs = std::filesystem;

	for (const auto & entry : fs::recursive_directory_iterator{source_directory}) {
		fs::path path{entry.path()};
		if (!specific_dir.empty() && fs::is_directory(path) && (specific_dir != path))
			continue;
		if (fs::is_regular_file(path)) {
			process_single(source_directory, destination_directory, path.string());
		}
	}
}

static std::string read_file_contents(
	const std::string & filename, const std::string & default_value)
{
	if (!std::filesystem::exists(filename)) {
		return default_value;
	}
	std::ostringstream os;
	std::ifstream ifs{filename.c_str()};
	ifs >> std::noskipws;
	std::copy(std::istream_iterator<char>{ifs}, std::istream_iterator<char>{},
		std::ostream_iterator<char>{os});
	return os.str();
}

static std::string get_meta_tags()
{
	return read_file_contents(system::get_theme_template_meta_tags(),
		"---\n"
		"title: \"Tag Overview: %s\"\n"
		"author: %s\n"
		"date: %s\n"
		"language: en\n"
		"---\n");
}

static std::string get_meta_years()
{
	return read_file_contents(system::get_theme_template_meta_years(),
		"---\n"
		"title: \"Year Overview: %s\"\n"
		"author: %s\n"
		"date: %s\n"
		"language: en\n"
		"---\n");
}

static std::string get_meta_contents()
{
	return read_file_contents(system::get_theme_template_meta_contents(), "---\n"
																		  "title: Contents\n"
																		  "author: %s\n"
																		  "date: %s\n"
																		  "language: en\n"
																		  "---\n");
}

static std::string create_temp_directory()
{
	std::string path = (std::filesystem::temp_directory_path() / "mkwebtmp-XXXXXX").string();
	::mkdtemp(&path[0]);
	return path;
}

template <typename Container, typename Comparison>
Container sorted(const Container & c, Comparison comp)
{
	Container t = c;
	std::sort(begin(t), end(t), comp);
	return t;
}

static void process_tags(const std::unordered_map<std::string, std::vector<std::string>> & items)
{
	namespace fs = std::filesystem;

	const auto date_str = posix_time::now().str_date();
	const auto file_meta_info = get_meta_tags();
	const auto author = system::cfg().get_author();

	const auto path = system::cfg().get_destination() + "/tag";
	ensure_path(path + '/');
	auto tmp = create_temp_directory();

	auto by_title = [](
		const auto & a, const auto & b) { return global.meta[a].title < global.meta[b].title; };

	for (auto const & entry : items) {
		try {
			// write meta data
			std::ofstream ofs{(tmp + '/' + entry.first + ".md").c_str()};
			ofs << fmt::sprintf(file_meta_info, entry.first, author, date_str) << '\n';

			// write list of links
			for (const auto &fn : sorted(entry.second, by_title)) {
				const auto link = fs::path{fn}.replace_extension(".html").string();
				ofs << "- [" << global.meta[fn].title << "](" << link << ")\n";
			}
		} catch (...) {
			std::filesystem::remove_all(tmp);
			throw std::runtime_error{"error in processing tag file for: " + entry.first};
		}
	}

	process_pages(tmp, path);
	std::filesystem::remove_all(tmp);
}
}

int main(int argc, char ** argv)
{
	// command line paramater handling

	std::string config_filename = "config.yml";
	std::string config_pandoc = "";
	std::string config_file;
	bool config_copy = false;
	bool config_plugins = false;

	// clang-format off
	cxxopts::Options options{argv[0], " - configuration read demo"};
	options.add_options()
		("h,help",
			"Shows help information")
		("c,config",
			"Read config from the specified file",
			cxxopts::value<std::string>(config_filename))
		("pandoc",
			"Specify pandoc binary to use",
			cxxopts::value<std::string>(config_pandoc))
		("file",
			"Specify a file or directory to process. This file or directory must be a "
			"part of the configured source directory within the configuration file.",
			cxxopts::value<std::string>(config_file))
		("copy",
			"Copies files from 'static' to 'destination'.",
			cxxopts::value<bool>(config_copy))
		("plugins",
			"Copies plugin files.",
			cxxopts::value<bool>(config_plugins))
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

	// generate site
	if (!config_file.empty()) {
		if (!std::filesystem::exists(config_file))
			throw std::runtime_error{"specified file does not exist: " + config_file};
		if (std::filesystem::is_directory(config_file)) {
			// TODO
			// process_pages(config.get_source(), config.get_destination(), specific_dir = path)
		} else if (std::filesystem::is_regular_file(config_file)) {
			// TODO
			process_single(
				system::cfg().get_source(), system::cfg().get_destination(), config_file);
		} else {
			throw std::runtime_error{"unable to process file type"};
		}
	} else {
		// TODO
		process_tags(global.tags);
		// process_years(years)
		// process_pages(config.get_source(), config.get_destination())
		// process_front()
		// process_redirect(config.get_destination())
		config_copy = true;
		config_plugins = true;
	}

	// copy files
	if (config_copy) {
		std::cout << "copy files\n";
		// TODO
	}

	// copy plugins
	if (config_plugins) {
		std::cout << "copy plugins\n";
		// TODO
	}

	return 0;
}
