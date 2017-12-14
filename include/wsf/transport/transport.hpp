#ifndef WSF_TRANSPORT_TRANSPORT_HPP
#define WSF_TRANSPORT_TRANSPORT_HPP

#include <wsf/transport/http.hpp>
#include <wsf/transport/websocket.hpp>
#include <wsf/transport/httpws.hpp>

namespace wsf
{
    namespace transport
    {
		//---------------------------------global interface--------------------------------------
        /**
        * @brief transport module initialize function
        * @param config   configure (no use for now)
        *
        * @return indicate initialize result
        *        -<em>false</em> fail
        *        -<em>true</em> succeed
        */
        bool Initialize(const std::string &config = "");

        /**
        * @brief Terminate transport module
        *
        */
        void Terminate();

		namespace http {
			inline const char *Scheme()
			{
				return "http";
			}
		}
		namespace websocket {
			inline const char *Scheme()
			{
				return "websocket";
			}
		}
    }
}

#endif