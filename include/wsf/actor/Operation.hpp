#ifndef WSF_ACTOR_OPERATION_HPP
#define WSF_ACTOR_OPERATION_HPP

#include <wsf/util/rest.hpp>
#include "Actor.hpp"

#define RESTAPI(rule_)                                                                      \
    \
public:                                                                                     \
    \
static const char *                                                                         \
    restapi()                                                                               \
    {                                                                                       \
        return rule_;                                                                       \
    }                                                                                       \
    \
static inline bool                                                                          \
    restapi_match(const std::string &req_method,                                            \
                  const char *urlpath, std::map<std::string, std::string> &param)           \
    {                                                                                       \
        static std::string method;                                                          \
        static std::string path;                                                            \
        static std::vector<std::string> names;                                              \
        if (method.empty())                                                                 \
        {                                                                                   \
            ::wsf::util::restapi_pickup(restapi(), method, path, names);                    \
            /*method = std::transform(method.begin(),method.end(),method.begin(),::toupper)*/   \
		}                                                                                   \
        return ::wsf::util::restapi_match(req_method, urlpath, method, path, names, param); \
    }                                                                                       \
    \
virtual::log4cplus::Logger &                                                                \
    Logger_()                                                                              \
    {                                                                                       \
        Actor::Logger_() = ::log4cplus::Logger::getInstance(                                \
            std::string("wsf.Actor.") + rule_);                                             \
        return Actor::Logger_();                                                            \
    }


namespace wsf
{
    namespace actor
    {

        class IOperation : public Actor
        {
		public:
			typedef ::wsf::transport::http::Response Response;
			IOperation()
			{
			}
			virtual ~IOperation()
			{
				DEBUG("operation destroied!\n");
			}

        protected:
			virtual bool OnInitialize(IService*)
			{
				return true;
			}
			void On_(http::request_message_t &msg)
			{
				http::Response res (new ::wsf::transport::http::Response(msg.context));
				this->On(*(msg.message), res);
			}

			virtual void On(const ::wsf::transport::http::request_t& req,
				std::shared_ptr< ::wsf::transport::http::Response> res)
			{
				res->set_status_code(::wsf::http::status_code::Not_Implemented);
				res->Reply();
			}

        private:
			friend class IService;
			friend class ServiceCenter;
        };

    }
}

#endif
