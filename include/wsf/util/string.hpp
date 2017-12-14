
#ifndef WSF_UTIL_STRING_HPP
#define WSF_UTIL_STRING_HPP
#include <string>

namespace wsf
{
	namespace util
	{
		inline bool startWith(const std::string& str, const std::string& prefix, std::string::size_type offset = 0)
		{
			return  (str.empty() || prefix.empty()) ? false :
				str.compare(offset, prefix.size(), prefix) == 0;
		}


// 		template<typename M>
// 		const typename M::mapped_type* get_value( const M& m, const typename M::key_type& key)
// 		{
// 			M::const_iterator it = m.find(key);
// 			return it == m.cend() ? nullptr : &it->second;
// 		}

// 		inline const std::string& get_value(const std::map<std::string, std::string>& m,
// 			const std::string& key,
// 			const std::string& default_value = "")
// 		{
// 			std::map<std::string, std::string>::const_iterator it = m.find(key);
// 
// 			return m.cend() == it ? default_value : it->second;
// 		}
// 
// 		static inline std::map<std::string, std::string> urlParam(
// 			const std::string& url, 
// 			const std::string& rule,
// 			std::string::size_type offset = 0)
// 		{
// 			std::map<std::string, std::string> param;
// 			
// 			int usize = static_cast<int>(url.size()) - static_cast<int>(offset);
// 			if( usize <=0 || rule.empty())
// 				return std::move(param);
// 
// 			const char* u = url.data() + offset;
// 			const char* r = rule.data();
// 			while (*u == '/')u++;
// 			const char* uend = NULL;
// 			const char* rend = NULL;
// 
// 			while (uend && rend &&*u && *r) {
// 				uend = strchr(u, '/');
// 				rend = strstr(r, "/:");
// 				param[rend ? std::string(r, rend) : std::string(r)] =
// 					uend ? std::string(u, uend) : std::string(u);
// 
// 				u = uend+1;
// 				r = rend+2;
// 			};
// 			return std::move(param);
// 		}
// 		
// 
// 		inline void urlParam__test()
// 		{
// 			std::map<std::string, std::string> param =	urlParam("/1/2/3", "/a/b/c");
// 			assert(param["a"] == "1");
// 			assert(param["b"] == "2");
// 			assert(param["c"] == "3");
// 			param.clear();
// 			param = urlParam("/1/22/3", "/a/bb/");
// 			assert(param["a"]  == "1");
// 			assert(param["bb"] == "22");
// 			assert(param.size() ==2 );
// 
// 			param = urlParam("/123456789/2/", "/a/abcdefg/-");
// 			assert(param["a"] == "123456789");
// 			assert(param["abcdefg"] == "2");
// 			assert(param.size() == 2);
// 
// 		}
	}
}





#endif