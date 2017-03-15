#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <experimental/optional>
#include <experimental/filesystem>

#include <cxxopts.hpp>

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

#include <fmt/format.h>

#include "system.hpp"
#include "config.hpp"
#include "posix_time.hpp"
#include "subprocess.hpp"
#include "version.hpp"

namespace mkweb
{
namespace fs
{
using path = std::experimental::filesystem::path;
using copy_options = std::experimental::filesystem::copy_options;
using recursive_directory_iterator
	= std::experimental::filesystem::recursive_directory_iterator;
using std::experimental::filesystem::exists;
using std::experimental::filesystem::is_regular_file;
using std::experimental::filesystem::is_directory;
using std::experimental::filesystem::last_write_time;
using std::experimental::filesystem::temp_directory_path;
using std::experimental::filesystem::remove_all;
using std::experimental::filesystem::copy;
using std::experimental::filesystem::create_directories;
using std::experimental::filesystem::canonical;
}

/// Meta information about a document.
struct meta_info {
	posix_time date;
	std::string title;
	std::vector<std::string> authors;
	std::vector<std::string> tags;
	std::string language;
	std::string summary;
	std::vector<std::string> plugins;
};

/// Contains all global data.
static struct {
	std::unordered_map<std::string, meta_info> meta;
	std::unordered_map<std::string, std::vector<std::string>> tags;
	std::unordered_map<std::string, std::vector<std::string>> years;
	std::map<posix_time, std::vector<std::string>, std::greater<>> dates;
	std::unordered_set<std::string> plugins;

	std::string tag_list;
	std::string year_list;
	std::string page_list;
} global;

/// Returns meta information about the specified file.
static std::experimental::optional<meta_info> get_meta_for_source(
	const std::string & filename_in)
{
	const auto it = global.meta.find(filename_in);
	if (it != global.meta.end())
		return it->second;
	return {};
}

/// Reads meta information from the header of a markdown file.
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

/// Collects information from the node into the container.
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

/// Specialization of the collect function for string.
static void collect(const YAML::Node & node, std::string & s)
{
	if (!node)
		return;

	if (node.IsScalar())
		s = node.as<std::string>();
}

/// Read meta data from the specified file.
///
/// Meta data is in YAML within the header of the markdown file.
static meta_info read_meta(const std::string & path)
{
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

/// Collects information recursively down the directory tree for each file.
///
/// Meta data is collected into the global data.
static void collect_information(const std::string & source_root_directory)
{
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
			global.years[fmt::sprintf("%04u", info.date.year())].push_back(path);
			global.dates[info.date].push_back(path);

		} catch (...) {
			// no relevant meta data found for file, there is nothing to be done
			continue;
		}
	}
}

/// Splits specified path into its parts.
static std::vector<std::string> split_path(const std::string & path)
{
	fs::path p{path};
	return std::vector<std::string>{p.begin(), p.end()};
}

/// Returns a path constructed from parts.
template <typename Iterator> static std::string join_path(Iterator begin, Iterator end)
{
	// std::experimental::filesystem::path(InputIterator, InputIterator)
	// does not work at the moment, therefore it needs to be done maually

	fs::path p;
	for (; begin != end; ++begin)
		p /= *begin;
	return p.string();
}

/// Replaces the root of links with the configured one.
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
	return join_path(parts.begin(), parts.end());
}

/// Prepares the tag list as a string containing HTML.
///
/// \param[in] tags Container of tags to render into HTML.
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

/// Returns a string representing the global tag list in HTML.
static std::string prepare_global_tag_list(
	const std::unordered_map<std::string, std::vector<std::string>> & tags)
{
	std::vector<std::string> ids;
	ids.reserve(tags.size());
	for (const auto & tag : tags)
		ids.push_back(tag.first);
	return prepare_tag_list(ids);
}

/// Returns a string containing links to year overview pages.
static std::string prepare_global_year_list(
	const std::unordered_map<std::string, std::vector<std::string>> & years)
{
	const auto site_url = system::cfg().get_site_url();

	std::vector<std::string> ids;
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

/// Checks wheather or not the specified container contains the element.
///
/// \param[in] element Element to check.
/// \param[in] container The container to be searched.
template <class Container>
static bool contains(
	const typename Container::value_type & element, const Container & container)
{
	return std::find(begin(container), end(container), element) != end(container);
}

/// Converts the specified path, replaces the file extension if configured
/// to be processed.
///
/// Example: configuration says to process '.md' files, so the file extension
///          of 'foo.md' will be replaced to 'foo.html'.
///
static std::string convert_path(const std::string & fn)
{
	fs::path path{fn};
	if (contains(path.extension().string(), system::cfg().get_source_process_filetypes())) {
		return path.replace_extension(".html").string();
	}
	return std::string{};
}

/// Sorts the specified container of IDs according to the sort criteria
/// defined by the sort description.
///
/// \param[in,out] ids The container to be sorted.
/// \param[in] desc Sort description.
///
static void sort(std::vector<std::pair<std::string, std::string>> & ids,
	const config::sort_description & desc)
{
	switch (desc.dir) {
		case config::sort_direction::ascending:
			std::sort(begin(ids), end(ids),
				[](const auto & a, const auto & b) { return a.first < b.first; });
			break;
		case config::sort_direction::descending:
			std::sort(begin(ids), end(ids),
				[](const auto & a, const auto & b) { return a.first > b.first; });
			break;
	}
}

/// Returns a container, IDs sorted according to the specified criteria.
///
/// \param[in] meta Meta information about pages.
/// \param[in] sort_desc Sorting criteria.
/// \return Container of sorted IDs.
///
static auto sorted_ids_of_global_pagelist(
	const std::unordered_map<std::string, meta_info> & meta,
	const config::sort_description & sort_desc)
{
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

	sort(ids, sort_desc);
	return ids;
}

/// Returns a string (HTML) with a list of all pages, created from the
/// specified meta information. The list will be sorted according to the
/// configuration.
static std::string prepare_global_pagelist(
	const std::unordered_map<std::string, meta_info> & meta)
{
	// generate HTML list of sorted entries
	const auto site_url = system::cfg().get_site_url();
	const auto sort_desc = system::cfg().get_pagelist().sorting;
	const auto num_entries = system::cfg().get_pagelist().num_entries;

	auto count = 0;

	const auto ids = sorted_ids_of_global_pagelist(meta, sort_desc);

	std::ostringstream os;
	os << "<ul>";
	for (const auto & entry : ids) {
		if ((num_entries != 0) && (count >= num_entries))
			break;
		++count;

		const auto filename = entry.second;
		const auto title = meta.find(filename)->second.title; // single-thread, still valid
		const auto url = replace_root(convert_path(filename));
		os << "<li><a href=\"" << url << "\">" << title << "</a></li>";
	}
	os << "</ul>";

	if ((system::cfg().get_sitemap().enable) && (num_entries != 0)
		&& (ids.size() > static_cast<std::size_t>(num_entries))) {
		os << "<div id=\"morelink\"><a href=\"" << site_url + system::get_sitemap_filename()
		   << "\">...</a></div>";
	}

	return os.str();
}

/// Returns a string (HTML) representing a list of tags for a specific document.
/// The list will be sorted according to the configuration.
static std::string prepare_page_tag_list(const std::string & filename_in)
{
	const auto meta = get_meta_for_source(filename_in);
	if (!meta)
		return std::string{};
	return prepare_tag_list(meta->tags);
}

/// Finds out if a conversion of a specific document is necessary or not.
///
/// \param[in] filename_in Source document.
/// \param[in] filename_out Destination filename.
///
static bool conversion_necessary(
	const std::string & filename_in, const std::string & filename_out)
{
	if (!fs::exists(filename_in))
		return false;
	if (!fs::exists(filename_out))
		return true;

	const auto mtime_out = fs::last_write_time(filename_out);
	if (mtime_out < fs::last_write_time(filename_in))
		return true;

	const auto th = system::get_theme();
	if (mtime_out < fs::last_write_time(th.get_template()))
		return true;
	if (!th.get_style().empty() && (mtime_out < fs::last_write_time(th.get_style())))
		return true;
	if (!th.get_footer().empty() && (mtime_out < fs::last_write_time(th.get_footer())))
		return true;

	return false;
}

/// Makes sure the entire path specified by the filename/filepath
/// is present. All non-existing directories will be created.
static bool ensure_path_for_file(const std::string & filename)
{
	if (filename.empty())
		return true;

	const auto path = fs::path{filename}.remove_filename();
	if (fs::exists(path))
		return fs::is_directory(path);
	return fs::create_directories(path);
}

/// Reads and returns JSON representation (generated by pandoc) of the specified
/// document.
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

/// Uses pandoc to write the destination document using the JSON data.
///
/// \param[in] params Parameters to execute pandoc.
/// \param[in] content JSON data for pandoc to render into the destination
///   document.
/// \return `true` if successful, `false` otherwise.
///
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

/// Processes a link within the JSON node. Links need to point to the
/// configured destination root.
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

/// Searches recursively links within the JSON DOM and processes
/// them to point to the configured destination URLs.
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

/// Appends the specified elements to the container.
template <class Container>
static void append(Container & v, std::initializer_list<typename Container::value_type> l)
{
	for (const auto & item : l)
		v.push_back(item);
}

/// Returns a string (HTML) to include a JavaScript script on the destination document.
///
/// \param[in] plugin The name of the plugin.
/// \param[in] filename The filename (from the plugin) to load.
///
static std::string make_plugin_script_string(
	const std::string & plugin, const std::string & filename)
{
	return "<script type=\"text/javascript\" src=\"" + system::cfg().get_plugin_url(plugin)
		+ filename + "\"></script>";
}

/// Creates a string for the HTML head section to load the necessary files for
/// the specified plugin.
///
/// \param[in] plugin The plugin's name.
/// \return HTML string.
///
static std::string create_header_for_plugin(const std::string & plugin)
{
	std::ostringstream os;
	auto cfg = YAML::LoadFile(system::get_plugin(plugin).get_config());
	if (cfg["include"]) {
		for (const auto & entry : cfg["include"]) {
			os << make_plugin_script_string(plugin, entry.as<std::string>());
		}
	}
	return os.str();
}

/// Prepares parameters for pandoc to generate the destination document.
///
/// \param[in] filename_in The source filename.
/// \param[in] filename_out The destination filename.
/// \param[in] tags_list Tags list for the page.
static std::vector<std::string> prepare_pandoc_params(const std::string & filename_in,
	const std::string & filename_out, const std::string & tags_list)
{
	const auto th = system::get_theme();

	// clang-format off
	std::vector<std::string> params {
		system::pandoc(),
		"-f", "json",
		"-t", "html5",
		"-o", filename_out,
		"-H", th.get_style(),
		"-M", "title-prefix=" + system::cfg().get_site_title(),
		"-V", "siteurl=" + system::cfg().get_site_url(),
		"-V", "sitetitle=" + system::cfg().get_site_title(),
		"--template", th.get_template(),
		"--standalone",
		"--preserve-tabs",
		"--toc", "--toc-depth=2",
		"--mathml"
	};
	// clang-format on

	if (!th.get_footer().empty())
		append(params, {"-A", th.get_footer()});
	if (!system::cfg().get_site_subtitle().empty())
		append(params, {"-V", "sitesubtitle=" + system::cfg().get_site_subtitle()});
	if (system::cfg().get_tags_enable())
		append(params, {"-V", "globaltags=" + global.tag_list});
	if (system::cfg().get_yearlist().enable)
		append(params, {"-V", "globalyears=" + global.year_list});
	if (system::cfg().get_social_enable())
		append(params, {"-V", "social=" + system::cfg().get_social()});
	if (system::cfg().get_menu_enable())
		append(params, {"-V", "menu=" + system::cfg().get_menu()});
	if (system::cfg().get_page_tags_enable() && !tags_list.empty())
		append(params, {"-V", "pagetags=" + tags_list});
	if (system::cfg().get_pagelist().enable && !global.page_list.empty())
		append(params, {"-V", "globalpagelist=" + global.page_list});

	const auto meta = get_meta_for_source(filename_in);
	if (meta) {
		for (const auto & plugin : meta->plugins) {
			append(params, {"-H", system::get_plugin(plugin).get_style()});
			append(params, {"-V", "header-string=" + create_header_for_plugin(plugin)});
		}
	}

	// theme specific stuff
	if (!system::cfg().get_theme().site_title_background.empty())
		append(params,
			{"-V", "sitetitle-background=" + system::cfg().get_theme().site_title_background});
	if (!system::cfg().get_theme().copyright.empty())
		append(params, {"-V", "copyright=" + system::cfg().get_theme().copyright});

	return params;
}

/// Processes a document.
///
/// \param[in] filename_in Filename of the source document.
/// \param[in] filename_out Filename of the destinatino document.
/// \param[in] tags_list List of tags for the document.
static void process_document(const std::string & filename_in, const std::string & filename_out,
	const std::string & tags_list = std::string{})
{
	if (!conversion_necessary(filename_in, filename_out)) {
		std::cout << "skip    " << filename_out << '\n';
		return;
	}

	std::cout << "        " << filename_out << '\n';
	ensure_path_for_file(filename_out);

	// conversion from source file to JSON and processing
	auto content = nlohmann::json::parse(read_json_str_from_document(filename_in));
	fix_links_recursive(content);

	// perform final conversion to HTML
	const auto params = prepare_pandoc_params(filename_in, filename_out, tags_list);
	const auto success = write_document_from_json(params, content.dump());
	if (!success)
		throw std::runtime_error{"unable to write file: " + filename_out};
}

/// Processes a single document.
///
/// \param[in] source_directory Source directory in which the source
///   document is to be found.
/// \param[in] destination_directory The directory in which the destination
///   document will be created.
/// \param[in] filename_in The filename of the document to process.
///
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

/// Returns `true` if the specified path is a subdirectory of the directory.
///
/// \param[in] path Path to check if it is a subdirectory.
/// \param[in] directory Directory to check against.
///
static bool is_subdir(const std::string & path, const std::string & directory)
{
	const auto p = fs::canonical(path);
	const auto d = fs::canonical(directory);

	return std::equal(d.begin(), d.end(), p.begin());
}

/// Process files of a complete directory tree.
///
/// \param[in] source_directory Directory tree to process the files from.
/// \param[in] destination_directory Directory to create destination documents in.
///
static void process_pages(const std::string source_directory,
	const std::string & destination_directory, const std::string & specific_dir = std::string{})
{
	if (!specific_dir.empty()) {
		if (!is_subdir(specific_dir, source_directory))
			throw std::runtime_error{
				"error: " + specific_dir + " is not a subdir of " + source_directory};
	}

	for (const auto & entry : fs::recursive_directory_iterator{source_directory}) {
		fs::path path{entry.path()};
		if (!specific_dir.empty() && fs::is_directory(path) && (specific_dir != path))
			continue;
		if (fs::is_regular_file(path)) {
			process_single(source_directory, destination_directory, path.string());
		}
	}
}

/// Reads and returns the contents of the specified file. If the file does not
/// exist, the default value will be returned.
///
/// \param[in] filename Filename of the file to read.
/// \param[in] default_value Value to be returned if the file does not exist.
static std::string read_file_contents(
	const std::string & filename, const std::string & default_value)
{
	if (!fs::exists(filename)) {
		return default_value;
	}
	std::ostringstream os;
	std::ifstream ifs{filename.c_str()};
	ifs >> std::noskipws;
	std::copy(std::istream_iterator<char>{ifs}, std::istream_iterator<char>{},
		std::ostream_iterator<char>{os});
	return os.str();
}

/// Returns the markdown header for a tag overview document.
static std::string get_meta_tags()
{
	return read_file_contents(system::get_theme().get_template_meta_tags(),
		"---\n"
		"title: \"Tag Overview: %s\"\n"
		"author: %s\n"
		"date: %s\n"
		"language: en\n"
		"---\n");
}

/// Returns the markdown header for a year overview document.
static std::string get_meta_years()
{
	return read_file_contents(system::get_theme().get_template_meta_years(),
		"---\n"
		"title: \"Year Overview: %s\"\n"
		"author: %s\n"
		"date: %s\n"
		"language: en\n"
		"---\n");
}

/// Returns the markdown header for a contents document.
static std::string get_meta_contents()
{
	return read_file_contents(system::get_theme().get_template_meta_contents(),
		"---\n"
		"title: Contents\n"
		"author: %s\n"
		"date: %s\n"
		"language: en\n"
		"---\n");
}

/// Returns the markdown header for a sitemap document.
static std::string get_meta_sitemap()
{
	return read_file_contents(system::get_theme().get_template_meta_sitemap(),
		"---\n"
		"title: Sitemap\n"
		"author: %s\n"
		"date: %s\n"
		"language: en\n"
		"---\n");
}

static std::string get_title_newest_entries()
{
	return read_file_contents(
		system::get_theme().get_title_newest_entries(), "Newest Entries:");
}

static std::string create_temp_directory()
{
	std::string path = (fs::temp_directory_path() / "mkwebtmp-XXXXXX").string();
	if (::mkdtemp(&path[0]) == nullptr)
		throw std::runtime_error{"Unable to create temporary directory"};
	return path;
}

/// Returns a sorted container.
template <typename Container, typename Comparison>
Container sorted(const Container & c, Comparison comp)
{
	Container t = c;
	std::sort(begin(t), end(t), comp);
	return t;
}

/// Returns a comparison function for the specified name.
static std::function<bool(const std::string &, const std::string &)> get_overview_sorting(
	const std::string & name)
{
	static const std::map<config::sort_description,
		std::function<bool(const std::string &, const std::string &)>>
		func_map = {
			{{config::sort_direction::ascending, "date"},
				[](const auto & a, const auto & b) {
					return global.meta[a].date < global.meta[b].date;
				}},
			{{config::sort_direction::descending, "date"},
				[](const auto & a, const auto & b) {
					return global.meta[a].date > global.meta[b].date;
				}},
			{{config::sort_direction::ascending, "title"},
				[](const auto & a, const auto & b) {
					return global.meta[a].title < global.meta[b].title;
				}},
			{{config::sort_direction::descending, "title"},
				[](const auto & a, const auto & b) {
					return global.meta[a].title > global.meta[b].title;
				}},
		};

	config::sort_description desc;

	if (name == "year")
		desc = system::cfg().get_yearlist().sorting;

	const auto i = func_map.find(desc);
	if (i != func_map.end())
		return i->second;

	// default comparator
	return [](
		const auto & a, const auto & b) { return global.meta[a].title > global.meta[b].title; };
}

static std::function<std::string(const meta_info &)> get_overview_decoration(
	const std::string & name)
{
	if (name == "year")
		return [](const meta_info & info) { return "`" + info.date.str_date() + "` "; };

	return [](const meta_info &) { return std::string{}; };
}

/// Creates a temporary document for the desired overview and processes it.
///
static void process_overview(
	const std::unordered_map<std::string, std::vector<std::string>> & items,
	const std::string & name, const std::string & file_meta_info)
{
	const auto date_str = posix_time::now().str_date();
	const auto author = system::cfg().get_author();

	const auto path = system::cfg().get_destination() + '/' + name;
	ensure_path_for_file(path + '/');
	auto tmp = create_temp_directory();

	const auto sorting = get_overview_sorting(name);
	const auto decoration = get_overview_decoration(name);

	for (auto const & entry : items) {
		const std::string id = entry.first;
		try {
			// write meta data
			std::ofstream ofs{(tmp + '/' + id + ".md").c_str()};
			ofs << fmt::sprintf(file_meta_info, id, author, date_str) << '\n';

			// write list of links
			for (const auto & fn : sorted(entry.second, sorting)) {
				const auto info = global.meta[fn];
				const auto link = fs::path{fn}.replace_extension(".html").string();
				ofs << "- " << decoration(info) << "[" << info.title << "](" << link << ")\n";
			}
		} catch (...) {
			fs::remove_all(tmp);
			throw std::runtime_error{"error in processing " + name + " file for: " + id};
		}
	}

	process_pages(tmp, path);
	fs::remove_all(tmp);
}

/// Creates a front page.
static void process_front()
{
	// write file with links to newest n pages

	const auto date_str = posix_time::now().str_date();
	const auto author = system::cfg().get_author();

	auto tmp = create_temp_directory();

	const auto index_filename = tmp + "/index.md";
	const auto destination_filename = system::cfg().get_destination() + "/index.html";

	try {
		std::ofstream ofs{index_filename.c_str()};
		ofs << fmt::sprintf(get_meta_contents(), author, date_str) << '\n';
		ofs << get_title_newest_entries() << "\n\n";

		const auto num = system::cfg().get_num_news();
		auto count = 0;
		for (const auto & date : global.dates) {
			if (count >= num)
				break;
			const auto date_str = date.first.str_date();
			for (const auto & fn : date.second) {
				if (count >= num)
					break;
				++count;

				const auto & info = global.meta[fn];

				const auto link = replace_root(convert_path(fn));
				ofs << "  - `" << date_str << "` : [" << info.title << "](" << link << ")\n";

				if (!info.summary.empty()) {
					ofs << '\n' << "    " << info.summary << '\n';
				}

				ofs << '\n';
			}
		}

		ofs.close();
		ensure_path_for_file(destination_filename);
		process_document(index_filename, destination_filename);
	} catch (...) {
		fs::remove_all(tmp);
		throw std::runtime_error{"error in processing front page"};
	}

	fs::remove_all(tmp);
}

/// Creates the sitemap.
static void process_sitemap()
{
	// write a page containing all pages of the site,
	// sorted according to the configuration

	const auto sitemap = system::cfg().get_sitemap();
	if (!sitemap.enable)
		return;

	const auto date_str = posix_time::now().str_date();
	const auto author = system::cfg().get_author();

	auto tmp = create_temp_directory();

	const auto index_filename = tmp + "/sitemap.md";
	const auto destination_filename
		= system::cfg().get_destination() + "/" + system::get_sitemap_filename();

	try {
		std::ofstream ofs{index_filename.c_str()};
		ofs << fmt::sprintf(get_meta_sitemap(), author, date_str) << '\n';

		for (const auto & entry : sorted_ids_of_global_pagelist(global.meta, sitemap.sorting)) {
			const auto meta = get_meta_for_source(entry.second);
			if (!meta)
				continue;

			const auto link = replace_root(convert_path(entry.second));
			ofs << " - `" << meta->date.str_date() << "` [" << meta->title << "](" << link
				<< ")\n";
		}

		ofs.close();
		ensure_path_for_file(destination_filename);
		process_document(index_filename, destination_filename);
	} catch (...) {
		fs::remove_all(tmp);
		throw std::runtime_error{"error in processing site map"};
	}

	fs::remove_all(tmp);
}

/// Creates a redirecion page. Useful to have such file in a directory to
/// prevent directory listing.
static void process_redirect(const std::string & directory)
{
	static const std::string content_fmt = "<!DOCTYPE html>\n"
										   "<html><head><meta http-equiv=\"refresh\" "
										   "content=\"0;url=%s\"></head><body></body></html>\n";

	const auto site_url = system::cfg().get_site_url();

	for (const auto path : fs::recursive_directory_iterator(directory)) {
		if (!fs::is_directory(path))
			continue;
		const fs::path filepath = path / "index.html";
		if (fs::exists(filepath))
			continue;

		std::cout << "redir:  " << filepath.string() << '\n';
		try {
			std::ofstream ofs{filepath.string().c_str()};
			ofs << fmt::sprintf(content_fmt, site_url) << '\n';
		} catch (...) {
			// intentionally ignored
		}
	}
}

/// Copies files or directories. It is possible to specify a function to ignore
/// specific entries.
///
/// \param[in] from File or directory to copy from.
/// \param[in] from File or directory to copy to.
/// \param[in] ignore Function to ask if an item has to be copied or not.
///
static void copy(
	const fs::path & from, const fs::path & to, std::function<bool(const fs::path &)> ignore)
{
	if (fs::is_regular_file(from) && !ignore(from)) {
		ensure_path_for_file(to.string());
		fs::copy(from, to, fs::copy_options::update_existing | fs::copy_options::skip_symlinks);
		return;
	}

	if (fs::is_directory(from)) {
		// empty directories are ignored
		for (const auto entry : fs::recursive_directory_iterator(from)) {
			const fs::path & path = entry.path();

			if (ignore(path))
				continue;

			// replace top path of source with the destination path and copy file
			const fs::path & dst = to / join_path(++path.begin(), path.end());
			ensure_path_for_file(dst.string());
			fs::copy(
				path, dst, fs::copy_options::update_existing | fs::copy_options::skip_symlinks);
		}
		return;
	}
}

/// Default option to copy without ignoring anything.
static void copy(const fs::path & from, const fs::path & to)
{
	// ignore none
	copy(from, to, [](const fs::path &) -> bool { return false; });
}

/// Copies static files to the destination directory.
static void process_copy_file()
{
	std::cout << "copy files\n";

	// if no static directory is configured, we assume the source directory to
	// contain files to copy. Just let's ignore the file types we are already
	// processing. if there is a static directory, just copy this and ignore the
	// source directory.
	if (system::cfg().get_static().empty()) {
		mkweb::copy(system::cfg().get_static(), system::cfg().get_destination(),
			[](const fs::path & path) {
				return fs::is_regular_file(path)
					&& contains(path.extension().string(),
						   system::cfg().get_source_process_filetypes());
			});
	} else {
		mkweb::copy(system::cfg().get_static(), system::cfg().get_destination());
	}
}

/// Copies all necessary files of a plugin to the destination directory.
///
/// \param[in] plugin The plugin's name. The files to copy are listed in
///   the plugin's configuration.
///
static void copy_plugin_files(const std::string & plugin)
{
	std::cout << "install plugin: " << plugin << '\n';

	const auto plg = system::get_plugin(plugin);
	const auto cfg = YAML::LoadFile(plg.get_config());

	if (!cfg || !cfg["install"])
		throw std::runtime_error{"error: unable to read configuration for plugin: " + plugin};

	const auto destination_path = fs::path{system::cfg().get_plugin_path(plugin)}; // TODO: correct?
	const auto plugin_path = fs::path{plg.get_path()};

	for (const auto & entry : cfg["install"]) {
		const auto f = entry.as<std::string>();
		const auto fn = plugin_path / f;
		if (!fs::exists(fn))
			throw std::runtime_error{
				"error: unable to copy file '" + f + "' of plugin " + plugin};
		if (fs::is_regular_file(fn)) {
			std::cout << "  copy [f] " << fn << " -> " << destination_path << '\n';
			mkweb::copy(fn, destination_path.string());
		} else if (fs::is_directory(fn)) {
			std::cout << "  copy [d] " << fn << " -> " << (destination_path / f) << '\n';
			ensure_path_for_file(destination_path.string());
			fs::copy(fn, destination_path / f, fs::copy_options::overwrite_existing
					| fs::copy_options::recursive | fs::copy_options::skip_symlinks);
		} else {
			throw std::runtime_error{"error: '" + f + "' is not a file or directory"};
		}
	}
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
	cxxopts::Options options{argv[0], std::string{mkweb::project_name} + " - Static Website Generator"};
	options.add_options()
		("h,help",
			"Shows help information")
		("version",
			"shows version")
		("info",
			"shows information")
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

	if (options.count("version")) {
		std::cout << project_name << ' ' << project_version << '\n';
		return 0;
	}

	if (options.count("info")) {
		std::cout << "path to binary: " << system::path_to_binary() << '\n';
		std::cout << "path to shared: " << system::path_to_shared() << '\n';
		return 0;
	}

	// validation
	if (!fs::exists(config_filename))
		throw std::runtime_error{"config file not readable: " + config_filename};
	if (config_pandoc.size()) {
		if (!fs::exists(config_pandoc))
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
		if (!fs::exists(config_file))
			throw std::runtime_error{"specified file does not exist: " + config_file};
		if (fs::is_directory(config_file)) {
			process_pages(
				system::cfg().get_source(), system::cfg().get_destination(), config_file);
		} else if (fs::is_regular_file(config_file)) {
			process_single(
				system::cfg().get_source(), system::cfg().get_destination(), config_file);
		} else {
			throw std::runtime_error{"unable to process file type"};
		}
	} else {
		process_overview(global.tags, "tag", get_meta_tags());
		process_overview(global.years, "year", get_meta_years());
		process_pages(system::cfg().get_source(), system::cfg().get_destination());
		process_front();
		process_sitemap();
		process_redirect(system::cfg().get_destination());
		config_copy = true;
		config_plugins = true;
	}

	if (config_copy) {
		process_copy_file();
	}
	if (config_plugins) {
		std::cout << "copy plugins\n";
		for (const auto & plugin : global.plugins)
			copy_plugin_files(plugin);
	}

	return 0;
}
