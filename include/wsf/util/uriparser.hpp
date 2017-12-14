/*
* parse url like this
*
* schema://username:password@host:port/path?key=value#fragment
* \____/   \______/ \______/ \__/ \__/ \__/ \_______/ \______/
*   |         |        |       |    |    |      |         |
* schema      |     password   |   port  |    query    fragment
*          username          host      path
*
* note:
*   - username, password, port, path, query, fragment is optional.
*   - scheme, host must be setting.
*   - username and password must be paired.
*
*/

#ifndef WSF_UTIL_URI_PARSER_HPP
#define WSF_UTIL_URI_PARSER_HPP
#include <stdint.h>
#include <string>
#include <regex>
//#include <std/lexical_cast.hpp>

namespace wsf
{
    namespace util
    {

        class URIParser
        {
        private:
            enum
            {
                SCHEME = 1,
                USER_INFO,
                USER_NAME,
                PASSWORD,
                HOST,
                COLON_PORT,
                PORT,
                PATH,
                QUERY,
                QUERY_KEY,
                QUERY_VALUE,
                FRAGMENT,
                FRAGMENT_VALUE
            };

        public:
            URIParser()
                : port_(0)
            {
            }
            ~URIParser()
            {
            }
            const std::string &schema()
            {
                return schema_;
            };
            const std::string &username()
            {
                return username_;
            };
            const std::string &password()
            {
                return password_;
            };
            const std::string &host()
            {
                return host_;
            };
            const std::string &path()
            {
                return path_;
            };
            const std::string &query()
            {
                return query_;
            };
            const std::string &fragment()
            {
                return fragment_;
            };
            uint16_t port()
            {
                return port_;
            };

            bool Parse(const std::string &uri)
            {
                /*static const*/ std::regex expression(
                    "^([^:/@?#]+)://(([^:/@?#]+):([^:/@?#]+)@)?([^:/?#]+)(:([^/?#]+))?"
                    "([^?#]*)?(\\?([^=#]+)=([^#]+))?(#(.*))?$");
                bool ret = std::regex_match(uri, field_, expression);
                if (ret)
                {
                    port_ = field_[PORT] == "" ? 0 : std::stoi(field_[PORT]);
                    schema_ = field_[SCHEME];
                    username_ = field_[USER_NAME];
                    password_ = field_[PASSWORD];
                    host_ = field_[HOST];
                    path_ = field_[PATH];
                    query_ = field_[QUERY];
                    fragment_ = field_[FRAGMENT_VALUE];
                }
                return ret;
            }

        protected:
            std::smatch field_;
            uint16_t port_;
            std::string schema_;
            std::string username_;
            std::string password_;
            std::string host_;
            std::string path_;
            std::string query_;
            std::string fragment_;
        };
    }
}

#endif
