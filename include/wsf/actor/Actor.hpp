#ifndef WSF_ACTOR_ACTOR_HPP
#define WSF_ACTOR_ACTOR_HPP

#include <stdint.h>

#include <new>

//STL
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <functional>
#include <algorithm>

//Theron
#include <Theron/Theron.h>
#include <Theron/Address.h>

#include <wsf/util/logger.hpp>

#include <wsf/transport/transport.hpp>




namespace wsf
{
    namespace actor
    {
		class IOperation;
		class IService;
		class ServiceCenter;

		//Only used for actor internal
		template< typename M, typename C = ::wsf::util::IObject>
		class auto_msg
		{
		public:
			auto_msg(M* m ,C* c)
				: message(m)
				, context(c)
			{}

			auto_msg(std::unique_ptr<M>& m, std::unique_ptr<C>& c)
				: message(m.release())
				, context(c.release())
			{}

			auto_msg(const auto_msg& other) {
				auto_msg* m = const_cast<auto_msg*>(&other);
				message = std::move(m->message);
				context = std::move(m->context);
			}

			std::unique_ptr<M> message;
			std::unique_ptr<C> context;
		private:
		};

        class Actor : public ::Theron::Actor
        {
        public:
			typedef Theron::Address Address;
			
			struct http {
				typedef ::wsf::transport::http::IContext     IContext;
				typedef ::wsf::transport::http::request_t    request_t;
				typedef ::wsf::transport::http::response_t   response_t;
				typedef ::wsf::transport::http::request_ptr  request_ptr;
				typedef ::wsf::transport::http::response_ptr response_ptr;

				struct request_context_t : public ::wsf::transport::http::IContext
				{
					typedef std::function< void(const ::wsf::transport::http::response_t& res) > response_fn;
					request_context_t(const Address& addr, const response_fn& fn)
						: actor(addr), response_handler(fn)
					{}

					Theron::Address actor;
					response_fn     response_handler;//function object (in actor) to handle the response
				};

				typedef auto_msg< request_t  > request_message_t;
				typedef auto_msg< response_t > response_message_t;
				typedef std::shared_ptr< ::wsf::transport::http::Response> Response;

			};
			struct websocket
			{
				//proxy->broker
				struct broker_context_t : public ::wsf::transport::websocket::IContext
				{
					broker_context_t(::wsf::transport::websocket::msg_status peer_status,
						const std::string &peer)
						: status(peer_status)
						, peer_token(peer)
					{}

					::wsf::transport::websocket::msg_status status;
					std::string peer_token;

				};
				typedef auto_msg<std::string, broker_context_t> ws_broker_msg_t;

				//broker->proxy
				struct proxy_context_t : public ::wsf::transport::websocket::IContext
				{
					proxy_context_t(::wsf::transport::websocket::msg_status peer_status, 
						const std::string &peer)
						: status(peer_status)
						, peer_url(peer)
					{}

					::wsf::transport::websocket::msg_status status;
					std::string peer_url;
				};
				typedef auto_msg<std::string, proxy_context_t> ws_proxy_msg_t;
			};

			struct timer_message_t {
				std::function< void(bool)> fn;
				bool is_canceled;
			};

            Actor();
            virtual ~Actor()
            {
				::log4cplus::threadCleanup();
            }

			void call(http::request_ptr& req,std::function< void(const http::response_t& res) > fn);			
			const std::string& name() { return name_; }
        protected:
			typedef uint32_t timerid_t;
			/**
			* @brief set timer
			*
			* @note context and req will be empty after invoked
			*
			* @example
			*
			* @param timeout timeout in milliseconds
			*
			* @param fn  callback when timeout or canceled (indicate in param canceled
			*
			* @return timer id which normally use to cancel 
			*/
			timerid_t SetTimer(timerid_t timeout /*ms*/, std::function< void(bool canceled)> fn);
			void      CancelTimer(timerid_t id);

            virtual ::log4cplus::Logger &Logger_() { return logger_; }
			virtual void OnEnd() {};//this will be called once all the job done by Release
			                        // after that you should not do any think in this actor.
			virtual void Release();

        protected:

			virtual void On_(http::request_message_t &message) {};
			virtual void On_(http::response_message_t &message) {};

        private:
			static void PostToActor(::wsf::transport::http::context_ptr & context,
				::wsf::transport::http::response_ptr & res);
			void on_http_request_message_handler_(const http::request_message_t &message, const Theron::Address from);
			void on_http_response_message_handler_(const http::response_message_t &message, const Theron::Address from);
			void on_timer_message_hander_(const timer_message_t& message, const Theron::Address from);
			

            ::log4cplus::Logger logger_;
			int call_counter_;
			int timer_counter_;
		private:
			std::string name_;
			std::map<std::string, std::string> _properties_;

            friend class IOperation;
            friend class IService;
			friend class ServiceCenter;
        };
    }
}








#endif//!

