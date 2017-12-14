
#include <wsf/actor.hpp>
#include "Framework.hpp"
#include "timer/ActorTimer.hpp"
namespace wsf {
	namespace actor {
		Actor::Actor()
 			: ::Theron::Actor( Framework::instance())
			, call_counter_(0)
			, timer_counter_(0)
 		{

			RegisterHandler(this, &Actor::on_http_request_message_handler_);
			RegisterHandler(this, &Actor::on_http_response_message_handler_);
			RegisterHandler(this, &Actor::on_timer_message_hander_);
			logger_ = ::log4cplus::Logger::getInstance("");
 		}

		
		void Actor::PostToActor(::wsf::transport::http::context_ptr & context,
				 ::wsf::transport::http::response_ptr & res)
		{
			Framework& framework = Framework::instance();
			ServiceCenter& sc = ServiceCenter::instance();
			http::request_context_t* c = static_cast<http::request_context_t*>(context.get());
			Address addr = c->actor;
			http::response_message_t msg(res,context);
			
			framework.Send(msg, sc.GetAddress(), addr);

		}


		void Actor::call(::wsf::transport::http::request_ptr& req,
			std::function< void(const ::wsf::transport::http::response_t& res) > fn)
		{
			using namespace ::wsf::transport::http;
			//auto context = make_context(http::request_context_t(this->GetAddress(), fn));
			typedef ::wsf::transport::http::context_ptr context_ptr;
			context_ptr context(new http::request_context_t(GetAddress(), fn));
			RequestToRemote(context, req, Actor::PostToActor);

			++call_counter_;
			DEBUG("call  no."<<call_counter_ );
			
		}

		static void actor_timer_function(Timer::parameter_t &param)
		{
			Framework& framework = Framework::instance();
			ServiceCenter& sc = ServiceCenter::instance();
			Actor::timer_message_t msg;
			msg.fn = param.fn;
			msg.is_canceled = param.is_canceled;

			framework.Send(msg,sc.GetAddress(), param.actor);
			//deallocate timer resource
			//Timer& timer = Timer::instance();
			//timer.Del(param.id);
		}


		Actor::timerid_t Actor::SetTimer(uint32_t timeout /*ms*/, std::function< void(bool)> fn)
		{
 			Timer& timer = Timer::instance();
 
 			Timer::parameter_t param;
 			param.actor = GetAddress();
			param.fn = fn;
			++timer_counter_;

			return timer.Add(timeout, actor_timer_function, param);
		}


		void Actor::CancelTimer(timerid_t id)
		{
			Timer& timer = Timer::instance();
			if (timer.Del(id))
			{
				--timer_counter_;
			}
		}

		void Actor::on_http_request_message_handler_(const http::request_message_t &message, const Theron::Address from)
		{
			http::request_message_t* m = const_cast<http::request_message_t*>(&message);
			this->On_(*m);
			Release();

		}

		void Actor::on_http_response_message_handler_(const http::response_message_t &message, const Theron::Address from)
		{
			http::request_context_t* context = static_cast<http::request_context_t*>(message.context.get());

			context->response_handler(*message.message);
			--call_counter_;
			Release();
		}

		void Actor::on_timer_message_hander_(const timer_message_t& message, const Theron::Address from)
		{
			message.fn(message.is_canceled);
			--timer_counter_;

			//try release 
			Release();

		}

		void Actor::Release() {
			if (call_counter_ == 0 && timer_counter_ == 0) {
				this->OnEnd();
				ServiceCenter &sc = ServiceCenter::instance();
				sc.ReleaseActor(this);
			}

		}

	}
}