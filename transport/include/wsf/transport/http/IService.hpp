#ifndef WSF_TRANSPORT_HTTP_SERVICE_HPP
#define WSF_TRANSPORT_HTTP_SERVICE_HPP

#include <list>
#include <mutex>
#include <thread>
#include <wsf/transport/inner/transport.hpp>

namespace wsf
{
    namespace transport
    {
        namespace http
        {
            class IService : public ::wsf::transport::IService
            {

            public:
                struct request_t;
                struct service_t;
                struct domain_t
                {
                    std::string host;
                    uint16_t port;
                    struct evhttp *http;
                    std::map<std::string /*path*/, service_t *> services;

                    domain_t(const std::string &host_,
                             uint16_t port_,
                             struct evhttp *http_ = NULL)
                        : host(host_)
                        , port(port_)
                        , http(http_)
                    {
                    }
                };

                struct service_t
                {
                    std::string path;
                    request_fn callback;
                    domain_t *domain;
                    std::list<request_t *> requests;

                    service_t(const std::string path_,
                              request_fn callback_,
                              domain_t *domain_)
                        : path(path_)
                        , callback(callback_)
                        , domain(domain_)
                    {
                    }
                };

                struct request_t
                {
                    struct evhttp_request *request;
                    service_t *service;
                    request_t(struct evhttp_request *req_,
                              service_t *service_)
                        : request(req_)
                        , service(service_)
                    {
                    }
                };

                struct request_context_t : public util::IObject
                {
                    request_context_t(request_t *req)
                        : request(req)
                    {
                    }
                    request_t *request;
                };

            public:
                virtual bool Initialize();
                virtual void Terminate();

                virtual bool Bind(const std::string &url, request_fn callback);
                virtual bool UnBind(const std::string &url_base);

                virtual bool Reply(context_ptr &context, response_ptr &res);

                IService()
                    : ::wsf::transport::IService(Scheme())
					, thread_(NULL)
                    , stop_(false)
                    , base_(NULL)
                    , commander_(NULL)
                {
                }

                ~IService()
                {
                }

            protected:
                static void MainLoop(void *const self);
                void MainLoop_();


                static void CmdCall(struct bufferevent *bev, void *const self);
                void CommandPolling(void *cmd);
                bool OnBind(const std::string &url, request_fn callback);
                bool OnUnBind(const std::string &url);


                static void MsgCall(struct bufferevent *bev, void *const self);
                void ActorMessagePolling();
                void OnMessage(const context_ptr &context, const response_ptr &res);

                static void SpecificRequest(struct evhttp_request *request, void *service);
                static void GenericRequest(struct evhttp_request *request, void *domain);

                void OnRequest(struct evhttp_request *request, void *service);

            private:
                domain_t *find_domain(const std::string &host, uint16_t port);
                domain_t *alloc_domain(const std::string &host, uint16_t port);

                service_t *find_service(domain_t *d, const std::string &path);
                void requeue_service_in_generic(service_t *s, const std::string &filter);
                service_t *alloc_service(domain_t *d,
                                         const std::string &path,
                                         request_fn callback);
                //para all_deallocted means weather the service itself or all
                //the services in the domain need to be deallocated
                void dealloc_service(service_t *s, bool all_deallocted = false);
                void dealloc_domain(domain_t *d, bool all_deallocted = false);

                inline request_t *alloc_request(struct evhttp_request *req, service_t *s);
                void dealloc_request(request_t *request, bool all_deallocted = false);

                void dealloc_domains();

                std::thread *thread_;
                bool stop_;

                struct event_base *base_;

                std::list<domain_t *> domains_;
                //std::vector<context_ptr, response_ptr> messages_;
                std::vector<response_ptr> messages_data_;
                std::vector<context_ptr> messages_context_;
                std::mutex messages_lock_;

                void *commander_;
            };
        }
    }
}
WSF_TRANSPORT_DECLARE(::wsf::transport::http::IService);

#endif
