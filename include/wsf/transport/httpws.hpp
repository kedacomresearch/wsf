#ifndef WSF_TRANSPORT_HTTP_OVER_WEBSOCKET_HPP
#define WSF_TRANSPORT_HTTP_OVER_WEBSOCKET_HPP

#include "http.hpp"
#include "websocket.hpp"
//http over websocket
namespace wsf
{
    namespace transport
    {
        namespace httpws
        {
            struct request_t;
            struct response_t;

            typedef ::wsf::util::IObject IContext;
            typedef std::unique_ptr<IContext> context_ptr;
            typedef std::unique_ptr<request_t> request_ptr;
            typedef std::unique_ptr<response_t> response_ptr;
        }
        namespace httpws
        {
            //http over websocket
            typedef std::function<void(context_ptr &, request_ptr &)> request_fn;
            typedef std::function<void(context_ptr &, response_ptr &)> response_fn;

            /**
			* @brief Bind on a url like http does, but the inner implementation is
			*	that websocket server bind and listen on a url.it will receive 
			*	websocket message, but can be parsed to http request or response.
			*
			* @note type of the received message
			*	<request>  => `request_fn`
			*	<response> => CSeq->RequestToRemote::`response_fn`
			*
			* @example
			*
			* @param url   the url bind to listen on
			*
			* @param fn   invoked when received a http request message
			*
			*/
            bool Bind(const std::string &url, request_fn fn);

            /**
			* @brief Connect to a websocket server.it will receive websocket 
			*	message, but can be parsed to http request or response.
			*
			* @note type of the received message
			*	<request>  => `request_fn`
			*	<response> => CSeq->RequestToRemote::`response_fn`
			*
			* @example
			*
			* @param url   the url bind to listen on
			*
			* @param callback   invoked when received a http request message
			*
			*/
            bool Connect(const std::string &url, request_fn callback);

            void DisConnect(const std::string &url);

            /**
			* @brief Reply a http response via websocket. It must be pointed 
			*	out clearly weather to use websocket client or server to send 
			*	the response message. (`content_role` in common_context_t)
			*
			* @note context->content_role
			*	WS_SERVER => websocket::PushMsg
			*	WS_CLIENT => websocket::SendMsg
			*
			* @example
			*
			* @param context   `common_context_t` including the extra information
			*
			* @param res   response message in http format, it will be serialized 
			*	to websocket message format in the function details
			*
			*/
            void ResponseToRemote(context_ptr &context, response_ptr &res);


            /**
			* @brief Send a http request via websocket.It must be pointed out 
			*	clearly weather to use websocket client or server to send the 
			*	request message. (`content_role` in common_context_t) If use 
			*	websocket client, you should invoke `Connect` firstly.
			*
			* @note context->content_role
			*	WS_SERVER => websocket::PushMsg
			*	WS_CLIENT => websocket::SendMsg
			*
			* @example
			*
			* @param context   `common_context_t` including the extra information
			*
			* @param req   request message in http format, it will be serialized
			*	to websocket message format in the function details
			*
			* @param fn   invoked when receive the http response
			*/
            void RequestToRemote(context_ptr &context, request_ptr &req, response_fn fn);
        }
        namespace httpws
        {
            enum role
            {
                WS_SERVER = 0,
                WS_CLIENT
            };
            struct common_context_t : public ::wsf::transport::websocket::IContext
            {
                common_context_t(const std::string &peer,
                                 role content_role_,
                                 ::wsf::transport::websocket::msg_status peer_status = websocket::msg_status::MESSAGE,
                                 const std::string &cseq = "")
                    : peer_token(peer)
                    , content_role(content_role_)
                    , status(peer_status)
                    , seq(cseq)
                {
                }

                std::string peer_token;
                role content_role;// use websocket server or client to send message
                ::wsf::transport::websocket::msg_status status;
                std::string seq;
            };

            struct response_function_context_t : public ::wsf::transport::http::IContext
            {
                typedef std::function<void(const response_t &res)> response_fn;
                response_function_context_t(const response_fn &fn)
                    : response_handler(fn)
                {
                }

                response_fn response_handler;
            };

            struct request_t : public ::wsf::transport::http::request_t
            {
            };

            struct response_t : public ::wsf::transport::http::response_t
            {
            };


            class Response : public ::wsf::transport::http::Response
            {
            public:
                Response(context_ptr &context)
                    : ::wsf::transport::http::Response(context)
                {
                }

                Response(context_ptr &context, response_ptr &res)
                    : ::wsf::transport::http::Response(context)
                {
                    ::wsf::transport::http::response_ptr p(res.release());
                    response_.swap(p);
                }

                const websocket::msg_status ws_status()
                {

                    if (context_ != NULL)
                    {
                        return static_cast<common_context_t *>(context_.get())->status;
                    }
                    return websocket::msg_status::INVALID;
                }
                //method
                virtual void Reply()
                {
                    if (context_ != NULL)
                    {
                        ::wsf::transport::http::response_t *http_res = response_.release();

                        ::wsf::transport::httpws::response_ptr r((httpws::response_t *)http_res);
                        ::wsf::transport::httpws::ResponseToRemote(context_, r);
                    }
                    //ResponseToRemote(context_, response_);
                }
            };
        }
    }
}

#endif