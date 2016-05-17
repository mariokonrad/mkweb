#ifndef __MKWEB__SYSTEM__HPP__
#define __MKWEB__SYSTEM__HPP__

#include <string>
#include <memory>

namespace mkweb
{
class config; // forward declaration

class system
{
public:
	static void reset(const std::shared_ptr<config> & cfg);
	static config & cfg();

	static std::string get_plugin_path(const std::string & plugin);
	static std::string get_plugin_style(const std::string & plugin);
	static std::string get_theme_path();
	static std::string get_theme_template();
	static std::string get_theme_style();
	static std::string get_theme_footer();

	static std::string pandoc();
	static void set_pandoc(const std::string & path);

private:
	static std::shared_ptr<config> cfg_;
	static std::string pandoc_;
};
}

#endif
