#ifndef MKWEB__PLUGIN__HPP
#define MKWEB__PLUGIN__HPP

#include <string>

namespace mkweb
{
class plugin
{
	friend class system;

public:
	std::string get_path() const;
	std::string get_config() const;
	std::string get_style() const;

private:
	const std::string path;

	plugin(const std::string & path);
};
}

#endif
