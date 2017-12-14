#ifndef WSF_TRANSPORT_HTTP_PROXY_HPP
#define WSF_TRANSPORT_HTTP_PROXY_HPP

#include <list>
#include <tuple>
#include <mutex>
#include <thread>
#include <wsf/transport/inner/transport.hpp>

namespace wsf
{
    namespace transport
    {
        namespace http
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
                    int MAX_CONNECTIONS;
                    std::list<connection_t *> connections;
                    std::list<request_t *> pendings;

                    domain_t(const std::string &host_,
                             uint16_t port_)
                        : host(host_)
                        , port(port_)
                        , MAX_CONNECTIONS(8)
                    {
                    }
                };
                struct connection_t
                {
                    struct evhttp_connection *connection;
                    domain_t *domain;
                    request_t *request;

                    connection_t(struct evhttp_connection *conn_,
                                 domain_t *domain_)
                        : connection(conn_)
                        , domain(domain_)
                        , request(NULL)
                    {
                    }
                };
                struct request_t
                {
                    context_ptr context;
                    request_ptr req;
                    response_fn callback;
                    struct evhttp_request *ev_request;
                    connection_t *connection;

                    request_t(context_ptr &context_,
                              request_ptr &req_,
                              response_fn callback_,
                              struct evhttp_request *ev_request_ = NULL,
                              connection_t *conn_ = NULL)
                        : context(context_.release())
                        , req(req_.release())
                        , callback(callback_)
                        , ev_request(ev_request_)
                        , connection(conn_)
                    {
                    }
                };

            public:
                virtual bool Initialize();
                virtual void Terminate();

                virtual bool Request(context_ptr &context,
                                     request_ptr &req,
                                     response_fn callback);
                virtual bool Connect(const std::string &url, websocket::notify_fn callback)
                {
                    return true;
                }
                virtual bool DisConnect(const std::string &url)
                {
                    return true;
                }
                virtual void SendMsg(const std::string &url, const std::string &data)
                {
                    return;
                }
                IProxy()
                    : ::wsf::transport::IProxy(Scheme())
					, thread_(NULL)
                    , stop_(false)
                    , base_(NULL)
                    , commander_(NULL)
                {
                }

                ~IProxy()
                {
                }

            protected:
                static void MainLoop(void *const self);
                void MainLoop_();

                static void MsgCall(struct bufferevent *bev, void *const self);
                void ActorMessagePolling();
                void OnMessage(context_ptr &context,
                               request_ptr &req,
                               response_fn callback);

                static void SpecificRequest(struct evhttp_request *request, void *conn);
                void OnRequest(struct evhttp_request *request, void *conn);

            private:
                domain_t *find_domain(const std::string &host, uint16_t port);
                domain_t *alloc_domain(const std::string &host, uint16_t port);
                connection_t *alloc_connection(domain_t *d, struct evhttp_connection *ev_conn);
                request_t *alloc_request(domain_t *d,
                                         context_ptr &context,
                                         request_ptr &req,
                                         response_fn callback);

                connection_t *next_idle_connection(domain_t *d);
                void MakeRequest(connection_t *c, request_t *r);
                // 				service_t *find_service(domain_t *d, const std::string &path);
                // 				void requeue_service_in_generic(service_t *s, const std::string  &filter);
                // 				service_t *alloc_service(domain_t *d, const std::string &path, const Address &actor,
                // const
                // std::string &filter);
                //
                // 				void dealloc_service(service_t *s);


                // 				inline request_t *alloc_request();
                // 				void dealloc_request(request_t *request);
                //void dealloc_connection(connection_t *conn);


                //para all_deallocted means weather the domain itself or all
                //the domains in the domains_ need to be deallocated
                void dealloc_domain(domain_t *d, bool all_deallocted = false);
                void dealloc_domains();

                std::thread *thread_;
                bool stop_;

                struct event_base *base_;

                std::list<domain_t *> domains_;
                std::list<std::tuple<context_ptr, request_ptr, response_fn>> request_messages_;
                std::mutex messages_lock_;
                //struct Context_t
                //{
                //	sca::asruntime::Address from_actor;
                //	std::string to_url;
                //	Context_t(const sca::asruntime::Address &from_,
                //		const std::string &url_)
                //		: from_actor(from_)
                //		, to_url(url_.c_str(), url_.size())
                //	{
                //	}
                //};
                //std::vector<std::shared_ptr<Context_t>> message_contexts_;

                void *commander_;
            };
        }
    }
}
WSF_TRANSPORT_DECLARE(::wsf::transport::http::IProxy);

#endif
