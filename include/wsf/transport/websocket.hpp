#ifndef WSF_TRANSPORT_WEBSOCKET_HPP
#define WSF_TRANSPORT_WEBSOCKET_HPP

#include <string>
#include <list>
#include <map>
#include <memory>
#include <functional>

#include <wsf/util/object.hpp>
#include <wsf/transport/http.hpp>

//#define UNIT_DEBUG 1
#ifdef UNIT_DEBUG
#include <crtdbg.h>
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

//forward declare

namespace wsf
{
    namespace transport
    {
        namespace websocket
        {
            typedef ::wsf::util::IObject IContext;
            typedef std::unique_ptr<IContext> context_ptr;
			enum msg_status
			{
				INVALID = 0,
				OPEN,
				MESSAGE,
				CLOSED
			};

            typedef std::function<void(msg_status status, const std::string & /*peer token*/,
				const std::string & /* data */)> broker_msg_fn;
            typedef std::function<void(msg_status status, const std::string &)> notify_fn;
        }
    }
}

namespace wsf
{
    namespace transport
    {
        namespace websocket
        {
            /**
			* @brief Bind a socket and listen to receive websocket msg
			*
			* @note The url is only used the host and the port parameters,
			*	the path, query, fragment are not used.
			*
			* @example
			*
			* @param url	url to bind
			*
			* @param callback	invoked when receiving msg in websocket broker or 
			*	one connection is closed by remote. The msg will be post to app.
			*	If peer connection closed, the first param of `broker_msg_fn` 
			*	is true. If invoked when receiving msg, the param error will be false.
			*
			*/
            bool Bind(const std::string &url, broker_msg_fn callback);
            bool UnBind(const std::string &url);
            /**
			* @brief App pushes msg to the websocket Broker and then 
			*	the msg will be sent to the appointed peer connections
			*
			* @note
			*
			* @example
			*
			* @param peers	tokens of peer connections to push data to
			*
			* @param data	msg data to push
			*
			*/
            void PushMsg(const std::list<std::string> &peers, const std::string &data);


            /**
			* @brief Connect to websocket broker
			*
			* @note
			*
			* @example
			*
			* @param peers	
			*
			* @param callback	When the websocket receive msg, it will be invoked 
			*	to deal with the msg, such as sending the msg to app.
			*	If the peer connection is closed is closed by remote, `notify_fn` will
			*	also be invoked and the first param will be true
			*
			*/
            bool Connect(const std::string &url, notify_fn callback);
            bool DisConnect(const std::string &url);

            /**
			* @brief Websocket proxy sends msg to the websocket Broker 
			*
			* @note The msg is one way, it won't get corresponding response as http request
			*
			* @example
			*
			* @param url	url of websocket broker to send msg to
			*
			* @param data	msg data to send
			*
			*/
            void SendMsg(const std::string &url, const std::string &data);
        }
    }
}


#endif