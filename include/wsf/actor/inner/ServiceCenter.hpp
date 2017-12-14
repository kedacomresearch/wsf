#ifndef WSF_ACTOR_SERVICE_CENTER_HPP
#define WSF_ACTOR_SERVICE_CENTER_HPP

#include <mutex>
#include <set>
#include <wsf/actor/Actor.hpp>
#include <wsf/actor/Service.hpp>

namespace wsf
{
    namespace actor
    {
		bool Initialize(const std::string& config);
		void Terminate();
        class ServiceCenter : public Actor
        {
        public:
			ServiceCenter(const std::string& name);
			~ServiceCenter();

			template< class _Service >
			static inline void RegistService(const std::string& name)
			{
				IService* s = new _Service();
				ServiceCenter::services_[name] = s;
			}

			bool Initialize(const std::string& config);
			void Terminate();

			static inline ServiceCenter& instance() {
				assert(instance_);
				return *instance_;
			}
			
			static std::string get_prototype_name(IService* is)
			{
				auto find_item = std::find_if(services_.begin(), services_.end(),
					[is](const std::map<std::string, IService*>::value_type item)
				{
					return item.second == is;
				});
				return find_item == services_.end() ? "" : find_item->first;
			}
			static void PostToSC(transport::http::context_ptr & context, transport::http::request_ptr & req);

			inline void AddActor(Actor* actor) {
				//actors_lock_.lock();
				actors_.push_back(actor);
				//actors_lock_.unlock();
			}

			inline void ReleaseActor(Actor* actor) {
				actors_garbage_collection_lock_.lock();
				actors_garbage_collection_.push_back(actor);
				actors_garbage_collection_lock_.unlock();
			}

			bool Publish(const std::string &topic_url);
			bool UnPublish(const std::string &topic_url);
			bool Subscribe(const std::string &topic_url, 
				const std::string &restapi, 
				const std::string &service_prefix);
			bool UnSubscribe(const std::string &topic_url, 
				const std::string &restapi, 
				const std::string &service_prefix);

			void PushMsg(const std::string &topic_url, const std::string &data);

		protected:
			bool Dispatch(Actor::http::request_message_t& msg);

			void OnStartupService(const transport::http::request_t &req, std::shared_ptr< transport::http::Response>& res);

			void OnStopService(const transport::http::request_t &req, std::shared_ptr< transport::http::Response>& res);
			virtual void Release() {};
		private:
			//deal with http request
			void On_(http::request_message_t &message);
			void ActorGarbageCollection();

			static std::map<std::string, IService*> services_;
			
			//std::mutex actors_lock_; 
			std::list<Actor *> actors_;//now only used by IService so lock is no need
			std::mutex actors_garbage_collection_lock_;
			std::list<Actor *> actors_garbage_collection_;//use to store the released actor 

			static ServiceCenter* instance_;
			std::string prefix_;
			std::string service_url_;

			//local Actors subscribe to remote topics
			//<topic_url,restapi>
			std::map<std::string, std::list<std::string> *> local_subscribers_;

			//remote Actors subscribed to local published topics
			//<topic_url, peer token>
			std::map<std::string, std::list<std::string> *> remote_subscribers_;
			std::mutex pub_sub_lock_;
			void on_ws_broker_message_handler_(const websocket::ws_broker_msg_t &message, const Theron::Address from);
			void on_ws_proxy_message_handler_(const websocket::ws_proxy_msg_t &message, const Theron::Address from);
			
			friend bool ::wsf::actor::Initialize(const std::string&);
			friend void ::wsf::actor::Terminate();
			friend class ::wsf::actor::IService;
        };

    }
}


#endif
