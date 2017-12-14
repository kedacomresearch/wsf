#include <wsf/actor.hpp>

#include "Framework.hpp"
using nlohmann::json;
using namespace std;
 namespace wsf {
 	namespace actor {
		
		
		using namespace ::wsf::transport::http;
		using namespace ::wsf::transport::websocket;
 		ServiceCenter* ServiceCenter::instance_ = NULL;

		std::map<std::string, IService*> ServiceCenter::services_;
		int notify_serial = 0;
 
 		ServiceCenter::ServiceCenter(const std::string& name)
 		{
			RegisterHandler(this, &ServiceCenter::on_ws_broker_message_handler_);
			RegisterHandler(this, &ServiceCenter::on_ws_proxy_message_handler_);
		}
 
 		ServiceCenter::~ServiceCenter()
 		{
 		}
 
		//template< class _Service >
		//void ServiceCenter::RegistService(const std::string& name)
		//{
		//	IService* s = new _Service();
		//	services_[name] = s;
		//}

		//typedef std::function<void(const context_ptr &, const request_ptr &)> App_Bind_Cb;
 		void ServiceCenter::PostToSC(::wsf::transport::http::context_ptr & context, request_ptr & req)
 		{
 			ServiceCenter& sc = ServiceCenter::instance();
 			Framework& framework = Framework::instance();
			http::request_message_t msg(req,context);
			framework.Send(msg, sc.GetAddress(), sc.GetAddress());
 		}

		//see wsf/actor.hpp
 		bool ServiceCenter::Initialize(const std::string& config)
 		{
			uint16_t port = 80;
			std::string prefix = "/";
			json j;
			if (!config.empty())
			{
				try
				{
					j = json::parse(config);
					auto root = j["service-center"];
					if (!root.is_null())
					{
						auto jport = root["port"];
						if (!jport.is_null())
						{
							port = jport;
						}

						auto jprefix = root["prefix"];
						if (!jprefix.is_null())
						{
							prefix = jprefix;
						}

					}
				}
				catch (...)
				{
					ERR(std::endl
						<< "--------------    Invalid Config    --------------" << std::endl
						<< config << std::endl
						<< "--------------------------------------------------" << std::endl);

					return false;
				}
				
			}
			
			std::string url = "http://0.0.0.0:" + std::to_string(port) + prefix;
			if (prefix.back() != '/')
				prefix.push_back('/');
			prefix_ = prefix;

			if (!Bind(url, ServiceCenter::PostToSC))
			{
				ERR("Init Service Center failed url: " << url);
				return false;
			}
			INFO("Service Center bind at " << url);
			service_url_ = url;

			auto jservices = j["services"];
			for (json::iterator it = jservices.begin(); it != jservices.end(); ++it)
			{
				auto js = *it;
				auto jprototype = js["prototype"];
				if( jprototype.is_null())
				{
					ERR("You missed service prototype name,your config is " << std::endl
						<< js.dump() << std::endl );
					return false;
				}

				std::string name = jprototype;
				map<string, IService*>::iterator s = services_.find(name);
				if (services_.end() == s)
				{
					ERR("Can't find prototype for configed Service <" << name << "> ");
					return false;
				}
				else
				{
					if (!s->second->OnStartup(it->dump()))
					{
						ERR("Startup <" << name << "> failed!");
						return false;
					}
				}

			}
			

			return true;
 		}

		void ServiceCenter::Terminate()
		{
			//unbind service_center itself
			if (!::wsf::transport::http::UnBind(service_url_))
			{
				ERR("Stop Service Center failed url: " << service_url_);
				return;
			}
			INFO("Service Center unbind at " << service_url_);

			//release all operations and send replies
			actors_garbage_collection_lock_.lock();
			std::for_each(actors_.begin(), actors_.end(),
				[](Actor *op) {
				delete op;
			});
			actors_.clear();
			actors_garbage_collection_lock_.unlock();

			//stop and close all services
			std::for_each(services_.begin(), services_.end(),
				[](const std::pair<std::string, IService*> &key_val) {
				if (key_val.second != NULL)
				{
					key_val.second->OnStop();
					delete key_val.second;
				}
			});

			pub_sub_lock_.lock();
			if (!local_subscribers_.empty())
			{
				for (auto &it : local_subscribers_)
				{
					DisConnect(it.first);
					if (it.second != NULL)
					{
						delete it.second;
					}
				}
				local_subscribers_.clear();
			}
			if (!remote_subscribers_.empty())
			{
				for (auto &it : remote_subscribers_)
				{
					::wsf::transport::websocket::UnBind(it.first);
					if (it.second != NULL)
					{
						delete it.second;
					}
				}
			}
			remote_subscribers_.clear();
			pub_sub_lock_.unlock();
		}


		bool ServiceCenter::Dispatch(http::request_message_t& msg)
		{
			ActorGarbageCollection();

			::wsf::transport::http::request_t&  reqest = *msg.message;
			const std::string& path = reqest.path();
			if (!util::startWith(path, prefix_))
			{
				return false;
			}
			
			//@api {post} /service/:name Startup Service
			if (util::startWith(path, "/service/", prefix_.size()-1))
			{

				const char* param = path.data() + prefix_.size() + strlen("service/");

				if (reqest.method() == "POST")
				{
					::set(reqest, "path.name", param);
					std::shared_ptr<transport::http::Response> res(new transport::http::Response(msg.context));

					OnStartupService(reqest, res);
					return true;
				}
				else if (reqest.method() == "DELETE")
				{
					::set(reqest, "path.name", param);
					std::shared_ptr<transport::http::Response> res(new transport::http::Response(msg.context));
					
					OnStopService(reqest, res);
					return true;
				}

			}
			return false;

		}

		void ServiceCenter::On_(Actor::http::request_message_t &message)
		{
			//start or stop services
			if (this->Dispatch(message))
			{
				return;
			}
			
			::wsf::transport::http::request_t&  reqest = *message.message;
			//generate operations
			const std::string& path = reqest.path();
			for (std::map<std::string, IService*>::const_iterator it = services_.cbegin();
				it != services_.cend(); it++){
				const std::string& prefix = it->second->prefix();
				if (util::startWith(path,prefix))
				{
					if (it->second->Dispatch(message)) {
						return;
					}
				}
			}
			//no service for this request. reply bad request
			transport::http::Response res(message.context);
			res.set_status_code(::wsf::http::status_code::Bad_Request);
			res.content() = "{\"reason\":\"invalid service path\"}";
			res.Reply();

		}
		
		void ServiceCenter::OnStartupService(const transport::http::request_t &req, std::shared_ptr< transport::http::Response>& res)
		{
			const string& name = get(req, "path.name");
			if (name.empty())
			{
				res->set_status_code(wsf::http::status_code::Bad_Request);
				res->content() = "{\"reason\":\"no service name\"}";
				res->Reply();
				return;
			}

			map<string, IService*>::iterator is = services_.find(name);
			if (is == services_.end())
			{
				res->set_status_code(wsf::http::status_code::Bad_Request);
				res->content() = "{\"reason\":\"no requested service\"}";
				res->Reply();
				return;
			}

			IService* s = is->second;
			if (s->get_status() == IService::status::RUNING)
			{
				res->set_status_code(wsf::http::status_code::Bad_Request);
				res->content() = "{\"reason\":\"service is running\"}";
				res->Reply();
				return;

			}

			if (!s->OnStartup(req.content()))
			{
				res->set_status_code(wsf::http::status_code::Bad_Request);
				res->content() = "{\"reason\":\"service startup failed\"}";
				res->Reply();
				return;
			}

			res->set_status_code(wsf::http::status_code::OK);
			res->Reply();
		}

		void ServiceCenter::OnStopService(const transport::http::request_t &req, 
			std::shared_ptr< transport::http::Response>& res)
		{
			const string& name = get(req, "path.name");
			if (name.empty())
			{
				res->set_status_code(wsf::http::status_code::Bad_Request);
				res->content() = "{\"reason\":\"no service name\"}";
				res->Reply();
				return;
			}

			map<string, IService*>::iterator is = services_.find(name);
			if (is == services_.end())
			{
				res->set_status_code(wsf::http::status_code::Bad_Request);
				res->content() = "{\"reason\":\"no requested service\"}";
				res->Reply();
				return;
			}

			IService* s = is->second;
			if (s->get_status() == IService::status::IDLE)
			{
				res->set_status_code(wsf::http::status_code::OK);
				res->content() = "{\"reason\":\"service not started\"}";
				res->Reply();
				return;

			}

			s->OnStop();
			res->set_status_code(wsf::http::status_code::OK);
			res->Reply();

		}


		void ServiceCenter::ActorGarbageCollection()
		{
			list<Actor*> agc;
			actors_garbage_collection_lock_.lock();
			agc.swap(actors_garbage_collection_);
			actors_garbage_collection_lock_.unlock();
			if (agc.empty())
				return;
			for (auto a : agc) {
				actors_.remove(a);
				delete a;
			}
		}

		bool ServiceCenter::Publish(const std::string &topic_url)
		{
			/* example: publish at url
			*					ws://127.0.0.1:8845/topic1
			*					ws://127.0.0.1:8845/topic2
			*					ws://127.0.0.1:8846/topic3
			*/
			bool rc=::wsf::transport::websocket::Bind(topic_url,
				[](msg_status status, const std::string &peer,
					const std::string &data) {

				if (status == msg_status::OPEN)
				{
					return;
				}
				ServiceCenter& sc = ServiceCenter::instance();
				Framework& framework = Framework::instance();

				websocket::broker_context_t context(status, peer);
				std::unique_ptr<std::string> p1(new std::string(data));
				std::unique_ptr<websocket::broker_context_t> p2(new websocket::broker_context_t(context));
				websocket::ws_broker_msg_t msg(p1,p2);

				framework.Send(msg, sc.GetAddress(), sc.GetAddress());

			});
			if (!rc)
			{
				ERR(this->name() << " Failed to bind url: " << topic_url);
				return false;
			}
			
			pub_sub_lock_.lock();
			if (remote_subscribers_[topic_url] == NULL)
			{
				remote_subscribers_[topic_url] = new std::list<std::string>();
			}
			pub_sub_lock_.unlock();

			INFO(this->name() << " Publish topic: " << topic_url);
			return true;
		}
		bool ServiceCenter::UnPublish(const std::string &topic_url)
		{
			pub_sub_lock_.lock();
			auto it = remote_subscribers_.find(topic_url);
			if (it != remote_subscribers_.end())
			{
				if (remote_subscribers_[topic_url] != NULL)
				{
					delete remote_subscribers_[topic_url];
				}
				remote_subscribers_.erase(it);
			}
			pub_sub_lock_.unlock();

			::wsf::transport::websocket::UnBind(topic_url);

			INFO(this->name() << " UnPublish topic: " << topic_url);
			return true;
		}
		bool ServiceCenter::Subscribe(const std::string &topic_url, const std::string &restapi, const std::string &service_prefix)
		{
			//parse host:port
			::wsf::util::URIParser parser;

			if (!parser.Parse(topic_url))
			{
				ERR(this->name() << " Failed to parse topic url: " << topic_url);
				return false;
			}
			uint16_t port = parser.port();
			std::string host = parser.host();
			std::string schema = parser.schema();
			if (schema != "ws" && schema != "wss")
			{
				ERR(this->name() << " The schema of the topic url is wrong: " << topic_url);
				return false;
			}
			std::string peer_base_url = schema + "://" + host + ":" + std::to_string(port);

			//subscribe to topic url
			bool rc = ::wsf::transport::websocket::Connect(topic_url,
				[peer_base_url](msg_status status, const std::string &data) {

				if (status != msg_status::OPEN)
				{
					ServiceCenter& sc = ServiceCenter::instance();
					Framework& framework = Framework::instance();

					websocket::proxy_context_t context(status, peer_base_url);
					std::unique_ptr<std::string> p1(new std::string(data));
					std::unique_ptr<websocket::proxy_context_t> p2(new websocket::proxy_context_t(context));
					websocket::ws_proxy_msg_t msg(p1,p2);

					framework.Send(msg, sc.GetAddress(), sc.GetAddress());
				}

			});
			if (!rc)
			{
				ERR(this->name() << " Failed to connect to url: " << topic_url);
				return false;
			}

			//As the connection won't be established immediately, So send the msg after a while
			//The function SendMsg also could be optimized
			SetTimer(500, [=](bool is_canceled) {
				//send subscribe info
				json j;
				j["topic"] = topic_url;

				SendMsg(topic_url, j.dump());
			});

			//get the whole url path of the operation
			std::string api = restapi;
			size_t index = api.find_first_of(" ", 0);
			if (index != std::string::npos)
			{
				size_t len = service_prefix.size();
				if (service_prefix[len - 1] == '/')
				{
					--len;
				}
				api.insert(index + 1, service_prefix.c_str(), len);
			}

			pub_sub_lock_.lock();
			if (local_subscribers_[topic_url] == NULL)
			{
				local_subscribers_[topic_url] = new std::list<std::string>();
			}
			std::list<std::string> &suber = *local_subscribers_[topic_url];

			auto it = std::find(suber.begin(), suber.end(), api);
			if (suber.empty() || it != suber.end())
			{
				suber.push_back(api);
				INFO(this->name() << " Operation: " << api
					<< " subscribes to topic: " << topic_url);
			}
			else
			{
				WARN(this->name() << " Operation: " << api
					<< " has been subscribed to topic: " << topic_url);
			}
			pub_sub_lock_.unlock();

			
			return true;
		}
		bool ServiceCenter::UnSubscribe(const std::string &topic_url, const std::string &restapi, const std::string &service_prefix)
		{
			std::string api = restapi;
			size_t index = api.find_first_of(" ", 0);
			if (index != std::string::npos)
			{
				size_t len = service_prefix.size();
				if (service_prefix[len - 1] == '/')
				{
					--len;
				}
				api.insert(index + 1, service_prefix.c_str(), len);
			}

			pub_sub_lock_.lock();
			auto it = local_subscribers_.find(topic_url);
			if (it != local_subscribers_.end())
			{
				if (local_subscribers_[topic_url] != NULL)
				{
					std::list<std::string> &suber = *local_subscribers_[topic_url];
					auto it = std::find(suber.begin(), suber.end(), api);
					if (it != suber.end())
					{
						local_subscribers_[topic_url]->erase(it);
					}
				}
			}
			pub_sub_lock_.unlock();

			::wsf::transport::websocket::DisConnect(topic_url);

			INFO(this->name() << " Operation: " << api
				<< " unsubscribes topic: " << topic_url);
			return true;
		}

		//just invoked in service center
		void ServiceCenter::on_ws_broker_message_handler_(const websocket::ws_broker_msg_t &message, 
			const Theron::Address from)
		{
			websocket::broker_context_t *context = static_cast<websocket::broker_context_t*>(message.context.get());
			std::string &peer_token = context->peer_token;
			if (context->status == MESSAGE)
			{
				//parse msg data and get topic(peer subscribed to)
				std::string &msg = *message.message.get();
				std::string topic_url;
				try
				{
					json j = json::parse(msg);
					topic_url = j["topic"];
				}
				catch (...)
				{
					ERR(this->name() << " Failed to parse msg: " << msg);
					return;
				}
				//add subscriber to topic
				if (remote_subscribers_.find(topic_url) != remote_subscribers_.end()
					&& remote_subscribers_[topic_url] != NULL)
				{
					std::list<std::string> &suber = *remote_subscribers_[topic_url];
					auto it = std::find(suber.begin(), suber.end(), peer_token);
					if (!suber.empty() && it == suber.end())
					{
						WARN(this->name() << " Peer connection: " << peer_token
							<< " has subscribed to topic: " << topic_url);
						return;
					}
					suber.push_back(peer_token);
					INFO(this->name() << " Peer connection: " << peer_token
						<< " subscribes to topic: " << topic_url);
				}
				else
				{
					ERR(this->name() << " The topic url is not published: " << topic_url);
				}
				
			}
			if (context->status == CLOSED)
			{
				//remove from subscribers
				pub_sub_lock_.lock();
				if (!remote_subscribers_.empty())
				{
					for (auto &it : remote_subscribers_)
					{
						if (it.second != NULL)
						{
							std::list<std::string> &subers = *(it.second);
							auto suber = std::find(subers.begin(), subers.end(), peer_token);
							if (suber != subers.end())
							{
								it.second->erase(suber);
								INFO(this->name() << " Remove subsciber: " << peer_token
									<< " from topic: " << it.first);
							}
						}
					}
				}
				pub_sub_lock_.unlock();
			}
		}
		void ServiceCenter::on_ws_proxy_message_handler_(const websocket::ws_proxy_msg_t &message,
			const Theron::Address from)
		{
			websocket::proxy_context_t *context = static_cast<websocket::proxy_context_t*>(message.context.get());
			

			if (context->status == MESSAGE)
			{
				//1.parse msg data to http msg
				std::string topic;
				std::string cseq;
				std::string data;
				try
				{
					json j = json::parse(*message.message.get());

					topic = j["Topic"];
					cseq = j["CSeq"];
					const std::string &tmp = j["Context"];
					data.assign(tmp.c_str(), tmp.size());
				}
				catch (...)
				{
					ERR(this->name() << " Failed to parse notify msg: " << *message.message.get());
					return;
				}
				//2.post it to every subscriber of the topic_url
				pub_sub_lock_.lock();
				if (local_subscribers_.find(topic) != local_subscribers_.end())
				{
					if (local_subscribers_[topic] != NULL)
					{
						std::list<std::string> &subers = *local_subscribers_[topic];
						std::for_each(subers.begin(), subers.end(),
							[&](std::string &suber_api) {
							::wsf::transport::http::request_ptr req(new ::wsf::transport::http::request_t);
							std::vector<std::string> tmp;
							::wsf::util::restapi_pickup(suber_api, req->method(), req->path(), tmp);
							req->header()["CSeq"] = cseq;
							req->header()["Topic"] = topic;
							req->content().swap(data);

							ServiceCenter& sc = ServiceCenter::instance();
							Framework& framework = Framework::instance();
							::wsf::transport::http::context_ptr context;
							http::request_message_t notification(req, context);
							framework.Send(notification, sc.GetAddress(), sc.GetAddress());
						});
					}
				}
				pub_sub_lock_.unlock();
			}
			if (context->status == CLOSED)
			{
				std::string &topic_url = context->peer_url;
				pub_sub_lock_.lock();
				if (!local_subscribers_.empty())
				{
					for (auto it = local_subscribers_.begin(); it != local_subscribers_.end();)
					{
						bool rc = util::startWith(it->first, topic_url);
						if (rc&& it->second != NULL)
						{
							delete it->second;
							INFO(this->name() << " The connection: " << it->first
								<< " has been closed! No notify msgs will be received!");
							it = local_subscribers_.erase(it);
						}
						else
						{
							++it;
						}
					}
				}
				pub_sub_lock_.unlock();
			}
		}

		void ServiceCenter::PushMsg(const std::string &topic_url, const std::string &data)
		{
			json j;
			j["CSeq"] = std::to_string(notify_serial++);
			j["Topic"] = topic_url;
			j["Context"] = data;

			pub_sub_lock_.lock();
			auto it = remote_subscribers_.find(topic_url);
			if (it != remote_subscribers_.end())
			{
				if (remote_subscribers_[topic_url] != NULL)
				{
					std::list<std::string> &subers = *remote_subscribers_[topic_url];
					//push msg
					::wsf::transport::websocket::PushMsg(subers, j.dump());
				}
			}
			else
			{
				ERR(this->name() << " The topic hasn't been published: " << topic_url);
			}
			pub_sub_lock_.unlock();
		}
	}
}