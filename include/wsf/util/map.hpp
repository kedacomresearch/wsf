#ifndef WSF_UTIL_MAP_HPP
#define WSF_UTIL_MAP_HPP

#include <string>
#include <map>

typedef std::map< std::string, std::string> stringmap_t;

static inline bool has(const stringmap_t& m,const std::string& name )
{
	return !(m.cend() == m.find(name));
}


static inline const std::string& get( const stringmap_t& m, 
	const std::string& name,
	const std::string& defaulter= "")
{
	stringmap_t::const_iterator it = m.find(name);
	return m.cend() == it ? defaulter: it->second;
}

static inline int get(const stringmap_t& m,
	const std::string& name,
	int defaulter )
{
	stringmap_t::const_iterator it = m.find(name);
	return m.cend() == it ? defaulter : std::stoi( it->second);
}



static inline void set(stringmap_t& m,
	const std::string& name,
	const std::string& value)
{
	m[name] = value;
}

static inline void set(stringmap_t& m,
	const std::string& name,
	int value)
{
	m[name] = std::to_string(value);
}


#endif