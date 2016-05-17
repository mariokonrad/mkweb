#ifndef __MKWEB__FS_UTIL__HPP__
#define __MKWEB__FS_UTIL__HPP__

#include <string>

namespace mkweb
{
std::string path_to_binary();
bool path_exists(const std::string & path);
}

#endif
