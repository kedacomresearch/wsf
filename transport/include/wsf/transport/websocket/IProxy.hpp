#ifndef WSF_TRANSPORT_WEBSOCKET_PROXY_HPP
#define WSF_TRANSPORT_WEBSOCKET_PROXY_HPP

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
            class IProxy : public ::wsf::transport::IProxy
            {
            public:
                struct connection_t;
                struct request_t;
                struct domain_t
                {
                    std::string host;
                    uint16_t port;
                    connection_t *connection;

                    domain_t(const std::string &host_,
                             uint16_t port_)
                        : host(host_)
                        , port(port_)
                        , connection(NULL)
                    {
                    }
                };
                struct connection_t
                {
                    domain_t *domain;
                    notify_fn callback;
                    void *context;
                    //std::list<request_t *> requests;//pub-sub

                    connection_t(domain_t *domain_,
                                 notify_fn callback_,
                                 void *socket_ = NULL)
                        : domain(domain_)
                        , callback(callback_)
                        , context(socket_)
                    {
                    }
                };
                //struct request_t
                //{
                //	connection_t *connection;
                //	sca::asruntime::auto_msg msg;
                //	std::string topic;
                //	sca::asruntime::Address from_actor;

                //	request_t(sca::asruntime::auto_msg &m,
                //		const sca::asruntime::Address &from_,
                //		const std::string &topic_,
                //		connection_t *conn_ = NULL)
                //		: msg(m)
                //		, from_actor(from_)
                //		, topic(topic_.c_str(), topic_.size())
                //		, connection(conn_)
                //	{
                //	}
                //};

            public:
                virtual bool Initialize();
                virtual void Terminate();
                virtual bool Request(http::context_ptr &context,
                                     http::request_ptr &req,
                                     http::response_fn callback)
                {
                    return true;
                }
                virtual bool Connect(const std::string &url, notify_fn callback);
                virtual bool DisConnect(const std::string &url);
                virtual void SendMsg(const std::string &url, const std::string &data);

                void CommandPolling();
                void MessagePolling();


                void OnFail(void *context);
                void OnOpen(void *context);
                void OnMessage(void *context, std::string &data);
                void OnClose(void *context);

                IProxy()
                    : ::wsf::transport::IProxy(Scheme())
                    , stop_(false)
                    , loop_(NULL)
                    , commander_(NULL)
                {
                }

                virtual ~IProxy()
                {
                }


            protected:
                static void MainLoop(void *const self);
                void MainLoop();

                bool OnConnect(const std::string &url, notify_fn callback);
                bool OnDisConnect(const std::string &url);

                void OnMessage(const std::string &url, std::unique_ptr<std::string> &data);

            private:
                domain_t *find_domain(const std::string &host, uint16_t port);
                domain_t *alloc_domain(const std::string &host, uint16_t port);

                connection_t *alloc_connection(domain_t *d, notify_fn callback);
                void dealloc_connection(connection_t *conn);
                void close_connection(void *timeout_watcher, void *context);

                void dealloc_domain(domain_t *d);
                void dealloc_domains();

                Theron::Detail::Thread thread_;
                bool stop_;

                std::list<domain_t *> domains_;
                std::vector<std::unique_ptr<std::string>> messages_data_;
                std::vector<std::string> messages_to_;
                std::mutex messages_lock_;

                void *loop_;
                void *commander_;
            };
        }
    }
}
WSF_TRANSPORT_DECLARE(::wsf::transport::websocket::IProxy);
#endif
