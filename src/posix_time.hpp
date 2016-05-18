#ifndef __MKWEB__POSIX_TIME__HPP__
#define __MKWEB__POSIX_TIME__HPP__

#include <ctime>

namespace mkweb
{
class posix_time
{
public:
	posix_time() { ::memset(&t, 0, sizeof(t)); }

	static posix_time from_string(const std::string & s)
	{
		posix_time t;
		::strptime(s.c_str(), "%Y-%m-%d %H:%M", &t.t);
		return t;
	}

	std::string str() const
	{
		char buf[32];
		::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &t);
		return std::string{buf};
	}

	uint32_t year() const { return t.tm_year + 1900; }
	uint32_t month() const { return t.tm_mon; }
	uint32_t day() const { return t.tm_mday; }
	uint32_t hour() const { return t.tm_hour; }
	uint32_t minute() const { return t.tm_min; }
	uint32_t second() const { return t.tm_sec; }

private:
	struct tm t;
};
}

#endif
