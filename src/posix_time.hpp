#ifndef __MKWEB__POSIX_TIME__HPP__
#define __MKWEB__POSIX_TIME__HPP__

#include <tuple>
#include <ctime>

namespace mkweb
{
class posix_time
{
public:
	enum class type { local, utc };

	posix_time() { ::memset(&t, 0, sizeof(t)); }

	static posix_time now(type time_type = type::utc)
	{
		posix_time t;
		auto tmp = std::time(nullptr);
		switch (time_type) {
			case type::utc:
				gmtime_r(&tmp, &t.t);
				break;
			case type::local:
				localtime_r(&tmp, &t.t);
				break;
		}
		return t;
	}

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

	std::string str_date() const
	{
		char buf[32];
		::strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
		return std::string{buf};
	}

	uint32_t year() const { return t.tm_year + 1900; }
	uint32_t month() const { return t.tm_mon; }
	uint32_t day() const { return t.tm_mday; }
	uint32_t hour() const { return t.tm_hour; }
	uint32_t minute() const { return t.tm_min; }
	uint32_t second() const { return t.tm_sec; }

	bool operator<(const posix_time & other) const
	{
		return std::tie(t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec)
			< std::tie(other.t.tm_year, other.t.tm_mon, other.t.tm_mday, other.t.tm_hour,
				   other.t.tm_min, other.t.tm_sec);
	}

	bool operator>(const posix_time & other) const
	{
		return std::tie(t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec)
			> std::tie(other.t.tm_year, other.t.tm_mon, other.t.tm_mday, other.t.tm_hour,
				   other.t.tm_min, other.t.tm_sec);
	}

private:
	struct tm t;
};
}

#endif
