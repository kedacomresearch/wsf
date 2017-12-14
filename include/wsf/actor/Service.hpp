#ifndef WSF_ACTOR_SERVICE_HPP
#define WSF_ACTOR_SERVICE_HPP

#include "Operation.hpp"
#include <wsf/util/string.hpp>

namespace wsf
{
    namespace actor
    {

        class IService
        {
        public:
			enum class status{ IDLE=0,RUNING};
			status get_status() { return status_; }

            IService();
            virtual ~IService();

			template<typename... Rest>
			inline IOperation* restapi_match_(const Actor::http::request_message_t& msg)
			{
				
				const std::string& path = msg.message->path();
				const std::string& method = msg.message->method();
				if (util::startWith(path,prefix_))
				{
					const char* urlpath = path.data() + prefix_.size()-1;
					std::map<std::string, std::string> param;
					
					IOperation* op = (IOperation*)util::RestMatcher<Rest... >::match(method, urlpath, param);
					if (op) {
						for (auto it : param)
						{
							::set(*msg.message, "path." + it.first, it.second);
						}
						return static_cast<IOperation*>(op);
					}
				}
				return NULL;
			}

			virtual IOperation* restapi_match(const Actor::http::request_message_t& msg)
			{
				assert(0);
				return NULL;
			}
			const std::string& prefix() { return prefix_; }

        protected:
			virtual bool OnStartup(const std::string& config);

			virtual bool OnStop();
    //        {
				//status_ = status::IDLE;
    //            return true;
    //        }

			virtual bool Dispatch(Actor::http::request_message_t& msg);
			std::string prefix_;
			status status_;
			std::string service_url_;
			
		private:

			friend class IOperation;			
			friend class ServiceCenter;
        };
    }
}

//`__VA_ARGS__` means the detailed operations type
//`restapi_match` will invoke `restapi_match_` to match the operation type 
//and create the corresponding operation
#define RESTAPIs(...)                                       \
    \
public:                                                     \
    \
::wsf::actor::IOperation *                                  \
    restapi_match(const ::wsf::actor::Actor::http::request_message_t &msg) \
    {                                                       \
        return this->restapi_match_<__VA_ARGS__>(msg);      \
    }


#endif//!
