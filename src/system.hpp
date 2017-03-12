#ifndef MKWEB__SYSTEM__HPP
#define MKWEB__SYSTEM__HPP

#include <memory>
#include <string>
#include "plugin.hpp"
#include "theme.hpp"

namespace mkweb
{
class config; // forward declaration

class system
{
public:
	static void reset(const std::shared_ptr<config> & cfg);
	static config & cfg();

	static plugin get_plugin(const std::string & name);
	static theme get_theme();

	static std::string get_sitemap_filename();

	static std::string pandoc();
	static void set_pandoc(const std::string & path);

private:
	static std::shared_ptr<config> cfg_;
	static std::string pandoc_;

	static std::string get_theme_path();
};
}

#endif
