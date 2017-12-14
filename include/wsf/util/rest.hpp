
#ifndef WSF_UTIL_REST_HPP
#define WSF_UTIL_REST_HPP
#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include "status_code.hpp"
namespace wsf {
	namespace util {

		// if matched the param in url.path will be stored in param
		static inline bool restapi_match(const std::string& method, //rest method: get put post delete 
			const char* path,// path removed prefix
			const std::string& api_method,
			const std::string& api_path,
			const std::vector<std::string>& api_param_list,
			std::map<std::string, std::string>& param
		)
		{
			if (api_method != method)
				return false;
			size_t n = (size_t)api_path.size();
			
			//the path include api-path
			if (strncmp(path, api_path.data(), n) != 0)
				return false;
			//path="/ABC" api_path="/AB" not match
			//path="/ABC" api_path="/ABC" matched
			//path="/AB/C" api_path="/AB" match, C will take as param
			

			if (path[n] == '\0' || path[n] == '/')
			{
				const char* u = path + api_path.size();
				while (*u == '/')u++;
				const char* uend = strchr(u, '/');
				for (std::vector<std::string>::const_iterator
					it = api_param_list.cbegin();
					it != api_param_list.cend();
					it++)
				{
					param[*it] = uend ? std::string(u, uend - u) : std::string(u);
					if (uend == NULL) break;
					u = uend + 1;
				}
				return true;
			}
			return false;
		}

		//pickup token according api def. get /user/$id => method=get path=/user params=['id']
 		static inline void restapi_pickup(const std::string& api, std::string& method, std::string& path,
 			std::vector<std::string>& params)
 		{
 			const char*	begin = api.data();
 			// const char* end = begin + api.size();
 			const char* p = begin;
 
 			while (*p == ' ')p++;
 			const char* m = strchr(p, ' ');
 			assert(m);
 			method.assign(p, m - p);
 
 			while (*m == ' ')m++;
 
 			p = strstr(m, "/$");
 			if (p) {
 				assert(p > m);
 				path.assign(m, p - m);
 
 				p += 2;
 				while (p && *p) {
 					const char* pend = strstr(p, "/$");
 					if (pend) {
 						params.push_back(std::string(p, pend));
 						p = pend + 2;
 					}
 					else {
 						params.push_back(p);
 						break;
 					}
 				}
 			}
 			else {
 				path.assign(m);
 			}
 
 		}



		//forward declare
		template<typename... Args>
		struct RestMatcher;
		//basic
		template<typename T, typename... Rest>
		struct RestMatcher<T, Rest...>
		{
			static void* match(const std::string& method, const char* path,
				std::map<std::string, std::string>& param)
			{
				if( T::restapi_match(method, path, param) )
					return new T();
				return RestMatcher< Rest... >::match(method, path, param);
			}
		};

		//terminate
		template< typename T>
		struct RestMatcher< T>
		{
			static void* match(const std::string& method, const char* path,
				std::map<std::string, std::string>& param)
			{
				if (T::restapi_match(method, path, param))
				{
					return new T();
				}
				return NULL;
			}

		};

	}
}

#endif