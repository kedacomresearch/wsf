#include <wsf/actor.hpp>
#include "Framework.hpp" 
using nlohmann::json;
using namespace std;

 namespace wsf {
 	namespace actor {
		LOGGER("");
 		IService::IService()
			: status_(status::IDLE)
 		{
 		}
 
 		IService::~IService()
 		{
 
 		}

		bool IService::OnStartup(const std::string& config)
		{
			
			uint16_t port = 80;
			std::string proto_name = ServiceCenter::get_prototype_name(this);
			std::string prefix = "/" + proto_name;
			if (!config.empty())
			{
				auto root = json::parse(config);
				auto jport = root["port"];
				if (!jport.is_null())
				{
					port = jport;
				}

				auto jprefix = root["prefix"];
				if (!jprefix.is_null())
				{
					prefix = jprefix;
					if (prefix[0] != '/')
						prefix = "/" + prefix;
				}
			}
			std::string url = "http://0.0.0.0:" + std::to_string(port) + prefix;
			if (!transport::http::Bind(url, ServiceCenter::PostToSC))
			{
				ERR("Init Service <" << proto_name << "> failed url=" << url);
				return false;
			}
			if (prefix.back() != '/')
				prefix.push_back('/');
			prefix_ = prefix;
			INFO("Init Service <"<< proto_name <<"> Success at url=" << url);
			status_ = status::RUNING;
			service_url_ = url;
			return true;
		}
		bool IService::OnStop()
		{
			if (status_ == status::RUNING)
			{
				std::string proto_name = ServiceCenter::get_prototype_name(this);

				if (!transport::http::UnBind(service_url_))
				{
					ERR("Stop Service <" << proto_name << "> failed url=" << service_url_);
					return false;
				}
				status_ = status::IDLE;
				INFO("Stop Service <" << proto_name << "> Success at url=" << service_url_);
			}

			return true;
		}
		bool IService::Dispatch(Actor::http::request_message_t& msg)
		{
			IOperation *op = restapi_match(msg);
			if (op)
			{
				if (!op->OnInitialize(this))
				{
					delete op;
					transport::http::response_t* res =new transport::http::response_t;
					res->set_status_code(http::status_code::Internal_Server_Error);
					res->content() = "{\"reason\":\"initialize failed.\"}";
					transport::http::response_ptr response(res);
					transport::http::ResponseToRemote(msg.context, response);
				
					return false;
				}

				ServiceCenter::instance().AddActor(op);
				Framework::instance().Send(msg, op->GetAddress(), op->GetAddress());
				return true;
			}
			return false;

		}
 	}
 }