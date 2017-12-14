#ifndef WSF_TRANSPORT_WEBSOCKET_BROKER_HPP
#define WSF_TRANSPORT_WEBSOCKET_BROKER_HPP

#include <list>
#include <mutex>
#include <wsf/transport/inner/transport.hpp>
#include <Theron/Detail/Threading/Thread.h>

namespace wsf
{
    namespace transport
    {
        namespace websocket
        {
            class IBroker : public ::wsf::transport::IBroker
            {

            public:
                struct domain_t
                {
                    std::string host;
                    uint16_t port;
                    void *context;
                    void *subscribers; /*std::map<std::string, per_session_data *>*/
                    broker_msg_fn callback;

                    domain_t(const std::string &host_,
                             uint16_t port_)
                        : host(host_)
                        , port(port_)
                        , context(NULL)
                        , subscribers(NULL)
                        , callback(NULL)
                    {
                    }
                };

            public:
                virtual bool Initialize();
                virtual void Terminate();

                virtual bool Bind(const std::string &url, broker_msg_fn callback);
                virtual bool UnBind(const std::string &url);

                virtual bool PushMsg(const std::list<std::string> &peers, const std::string &data);

                void CommandPolling();
                void MessagePolling();


                void OnMessage(std::string &data, void *context, void *session);
                void OnClose(void *context, void *session);
                void OnOpen(void *context, void *session);

                IBroker()
                    : ::wsf::transport::IBroker(Scheme())
                    , stop_(false)
                    , loop_(NULL)
                    , commander_(NULL)
                {
                }

                virtual ~IBroker()
                {
                }

            protected:
                static void MainLoop(void *const self);
                void MainLoop();

                bool OnBind(const std::string &url, broker_msg_fn callback);
                bool OnUnBind(const std::string &url);

                void OnMessage(const std::list<std::string> &peers, const std::string &data);

            private:
                //void SendMsgToSubscribers();
                domain_t *find_domain(const std::string &host, uint16_t port);
                domain_t *alloc_domain(const std::string &host, uint16_t port);

                //service_t *find_service(domain_t *d, const std::string &topic);
                //service_t *alloc_service(domain_t *d, const std::string &topic);

                //void dealloc_service(service_t *s);
                void dealloc_domain(domain_t *d);

                void dealloc_domains();

                Theron::Detail::Thread thread_;
                bool stop_;

                std::list<domain_t *> domains_;
                std::vector<std::list<std::string /*peer connection*/>> messages_to_;
                std::vector<std::string /*data*/> messages_data_;
                std::mutex messages_lock_;

                //std::list<std::pair<sca::asruntime::auto_msg, boost::shared_ptr<std::list<service_t *>>>> pub_msgs_;
                //std::map<std::string /*topic*/, service_t *> topics_map_;

                void *loop_;
                void *commander_;
            };
        }
    }
}
WSF_TRANSPORT_DECLARE(::wsf::transport::websocket::IBroker);

#endif
