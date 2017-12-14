#include "helper.hpp"
#include <wsf/transport/http/IService.hpp>

#include <signal.h>

#ifdef WIN32
#include <WinSock2.h>
#else
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>


namespace wsf
{
    namespace transport
    {
        namespace http
        {
            namespace
            {
                enum command_code
                {
                    BIND = 0,
                    UNBIND,
                    TERM
                };
                struct bind_t : public Commander::command_t
                {
                    bind_t(const std::string &url_,
                           request_fn callback_)
                        : command_t(BIND)
                        , params(url_, callback_)
                        , result(0)
                    {
                    }
                    struct parameter_t
                    {
                        request_fn callback;
                        std::string url;

                        parameter_t(const std::string &url_,
                                    request_fn callback_)
                            : callback(callback_)
                            , url(url_)
                        {
                        }
                    };

                    typedef uint8_t return_t;

                    parameter_t params;
                    return_t result;
                };
                struct unbind_t : public Commander::command_t
                {
                    unbind_t(const std::string &url_)
                        : command_t(UNBIND)
                        , params(url_)
                        , result(0)
                    {
                    }
                    struct parameter_t
                    {
                        std::string url;

                        parameter_t(const std::string &url_)
                            : url(url_)
                        {
                        }
                    };

                    typedef uint8_t return_t;

                    parameter_t params;
                    return_t result;
                };

                struct term_t : public Commander::command_t
                {
                    term_t()
                        : command_t(TERM)
                    {
                    }

                    typedef uint8_t return_t;
                };
            }
        }
        namespace http
        {
            bool IService::Initialize()
            {
                base_ = event_base_new();
                if (base_ == NULL)
                {
                    FATAL("Initialize" << this->name() << " Event failed ! ");
                    return false;
                }

                commander_ = new Commander();
                if (!static_cast<Commander *>(commander_)->Initialize())
                {
                    static_cast<Commander *>(commander_)->Terminate();
                    delete static_cast<Commander *>(commander_);
                    commander_ = NULL;
                    FATAL("Initialize" << this->name() << " Commander failed ! ");
                    return false;
                }
                call_t *cmd_ = static_cast<Commander *>(commander_)->command_socket();
                struct bufferevent *cmd_bev;
                cmd_bev = bufferevent_socket_new(base_, cmd_->callee, BEV_OPT_CLOSE_ON_FREE);
                if (!cmd_bev)
                {
                    FATAL("Initialize" << this->name() << " Failed to create bufferevent socket! ");
                    return false;
                }
                bufferevent_setcb(cmd_bev, CmdCall, NULL, NULL, this);
                bufferevent_enable(cmd_bev, EV_WRITE | EV_READ);
                cmd_->buf_event = cmd_bev;

                call_t *msg_ = static_cast<Commander *>(commander_)->message_socket();
                struct bufferevent *msg_bev;
                msg_bev = bufferevent_socket_new(base_, msg_->callee, BEV_OPT_CLOSE_ON_FREE);
                if (!msg_bev)
                {
                    FATAL("Initialize" << this->name() << " Failed to create bufferevent socket! ");
                    return false;
                }
                bufferevent_setcb(msg_bev, MsgCall, NULL, NULL, this);
                bufferevent_enable(msg_bev, EV_WRITE | EV_READ);
                msg_->buf_event = msg_bev;

                thread_ = new std::thread(&::wsf::transport::http::IService::MainLoop, this);
                if (thread_ == NULL)
                {
                    FATAL("Initialize " << this->name()
                                        << " thread Start Failed!");
                    Terminate();

                    return false;
                }
                return true;
            }

            struct cb_arg
            {
                cb_arg(struct event_base *base_,
                       bool &stop_)
                    : base(base_)
                    , stop(stop_)
                {
                }
                struct event_base *base;
                bool &stop;
            };
            static void term_cb(evutil_socket_t sig, short events, void *user_data)
            {
                struct cb_arg *arg = static_cast<struct cb_arg *>(user_data);
                arg->stop = true;
                event_base_loopbreak(arg->base);
            }
            void IService::Terminate()
            {
                //stop_ = true;
                //event_base_loopbreak(base_);
                //term_t term_command;
                //static_cast<Commander *>(commander_)->SendCommand<term_t>(term_command);

                //struct event *signal_event = evsignal_new(
                //	base_, SIGINT, signal_cb, NULL);
                //evsignal_add(signal_event, NULL);
                struct cb_arg arg(base_, stop_);

#ifdef _WIN32
                struct event *term_event = NULL;
                struct timeval tv = {1, 0};

                term_event = evtimer_new(base_, term_cb, &arg);
                evtimer_add(term_event, &tv);
#else
                struct event *term_event = evsignal_new(base_, SIGTERM, term_cb, &arg);
                evsignal_add(term_event, NULL);

                pthread_t tid = thread_->native_handle();
                pthread_kill(tid, SIGTERM);
#endif
                if (!domains_.empty())
                {
                    dealloc_domains();
                }
                if (commander_ != NULL)
                {
                    static_cast<Commander *>(commander_)->Terminate();
                    delete static_cast<Commander *>(commander_);
                    commander_ = NULL;
                }

                thread_->join();
                event_free(term_event);

                if (base_ != NULL)
                {
                    event_base_free(base_);
                    base_ = NULL;
                }
                if (thread_ != NULL)
                {
                    delete thread_;
                    thread_ = NULL;
                }
            }

            void IService::MainLoop(void *const self)
            {
                IService *This = static_cast<IService *>(self);
                This->MainLoop_();
            }

            void IService::MainLoop_()
            {
                while (!stop_)
                {
                    event_base_dispatch(base_);
                }
            }
        }
        //receive message
        namespace http
        {
            void IService::GenericRequest(struct evhttp_request *request, void *domain)
            {
                using namespace ::wsf::transport;

                IService *instance = static_cast<IService *>(getInstance(http::Scheme()));
                domain_t *d = static_cast<domain_t *>(domain);
                //decode url
                const char *uri = evhttp_request_get_uri(request);
                size_t *size_out = NULL;
                char *decode_uri = evhttp_uridecode(uri, 0, size_out);
                if (decode_uri == NULL)
                {
                    evhttp_send_reply(request, 400, "Bad Request", NULL);
                    //LOG4CPLUS_ERROR(instance->Logger_(), instance->name() << " Failed to decode the url: " << uri);
                    free(decode_uri);
                    return;
                }
                //parse url to get path
                struct evhttp_uri *ev_decode_url = evhttp_uri_parse_with_flags(decode_uri, EVHTTP_URI_NONCONFORMANT);
                if (ev_decode_url == NULL)
                {
                    evhttp_send_reply(request, 400, "Bad Request", NULL);
                    //LOG4CPLUS_ERROR(instance->Logger_(), instance->name() << " Failed to parse the url: " << uri);
                    free(decode_uri);
                    evhttp_uri_free(ev_decode_url);
                    return;
                }
                const char *path = evhttp_uri_get_path(ev_decode_url);
                //match path with services list
                bool match = false;
                typedef std::pair<std::string, service_t *> path_service;
                std::for_each(d->services.begin(), d->services.end(),
                              [&](const path_service &curr_service) {
                                  if (!match)
                                  {
                                      std::string::size_type idx = std::string(path).find(curr_service.first);
                                      if (idx != std::string::npos)
                                      {
                                          instance->OnRequest(request, curr_service.second);
                                          match = true;
                                      }
                                  }
                              });
                //mismatch: reply 404
                if (!match)
                {
                    evhttp_send_reply(request, 404, "Not Found", NULL);
                    //LOG4CPLUS_WARN(instance->Logger_(), instance->name() << " 404 Not Found: " << ev_decode_url);
                }
                free(decode_uri);
                evhttp_uri_free(ev_decode_url);
            }
            void IService::SpecificRequest(struct evhttp_request *request, void *service)
            {
                using namespace ::wsf::transport;

                wsf::transport::IService *instance = getInstance(http::Scheme());
                static_cast<IService *>(instance)->OnRequest(request, service);
            }
            void IService::OnRequest(struct evhttp_request *request, void *service)
            {
                service_t *s = static_cast<service_t *>(service);
                if (s == NULL)
                {
                    ERR(this->name() << " OnRequest: the service isn't started up! ");
                    evhttp_send_reply(request, 503, "Service Unavailable", NULL);
                    return;
                }
                // 1.Get request buffer
                evhttp_cmd_type cmd = evhttp_request_get_command(request);

                struct evkeyvalq *header = evhttp_request_get_input_headers(request);
                //const char *content_type = evhttp_find_header(header, "Content-Type");
                //if (content_type == NULL || std::string(content_type) != "protobuf_json")
                //{
                //    ERR(this->name() << style::annotation(" OnRequest: invalid request header! ")
                //                     << content_type);
                //    return;
                //}
                struct evbuffer *buf = evhttp_request_get_input_buffer(request);
                char *data = static_cast<char *>(static_cast<void *>(evbuffer_pullup(buf, -1)));
                size_t size = evbuffer_get_length(buf);

                // 2.Add http data to message_ptr
                const char *uri = evhttp_request_get_uri(request);
                size_t *size_out = NULL;
                char *decode_uri = evhttp_uridecode(uri, 0, size_out);
                if (decode_uri == NULL)
                {
                    evhttp_send_reply(request, 400, "Bad Request", NULL);
                    ERR(this->name() << " Failed to decode the url: " << uri);
                    free(decode_uri);
                    return;
                }
                // When invoking `OnRequest`, it indicates that the url
                // in the evhttp_request is usually valid.
                struct evhttp_uri *ev_decode_url = evhttp_uri_parse_with_flags(decode_uri, EVHTTP_URI_NONCONFORMANT);
                const char *path = evhttp_uri_get_path(ev_decode_url);
                const char *query = evhttp_uri_get_query(ev_decode_url);
                struct evkeyvalq ev_query;
                evhttp_parse_query_str(query, &ev_query);

                // 2.5.set http request msg
                request_ptr msg(new http::request_t);
                //host
                msg->host() = s->domain->host;
                //port
                msg->port() = s->domain->port;
                //method
                switch (cmd)
                {
                    case EVHTTP_REQ_GET:
                        msg->method() = "GET";
                        break;
                    case EVHTTP_REQ_POST:
                        msg->method() = "POST";
                        break;
                    case EVHTTP_REQ_PUT:
                        msg->method() = "PUT";
                        break;
                    case EVHTTP_REQ_DELETE:
                        msg->method() = "DELETE";
                        break;
                    default:
                        break;
                }
                //path
                msg->path().assign(path);
                free(decode_uri);
                evhttp_uri_free(ev_decode_url);
                //query
                struct evkeyval *key_val;
                WSF_TAILQ_FOREACH(key_val, &ev_query, next)
                {
                    std::string key(key_val->key);
                    std::string value(key_val->value);
                    msg->query()[key] = value;
                }
                //header
                WSF_TAILQ_FOREACH(key_val, header, next)
                {
                    std::string key(key_val->key);
                    std::string value(key_val->value);
                    msg->header()[key] = value;
                }
                //body
                msg->content().assign(data, size);

                if (s->callback != NULL)
                {
                    //3.make stub for the send_out msg
                    request_t *req = alloc_request(request, s);
                    //4.Send msg to application
                    context_ptr context(new request_context_t(req));
                    s->callback(context, msg);
                }
				evhttp_clear_headers(&ev_query);

            }
        }
        //send message
        namespace http
        {
            void IService::MsgCall(struct bufferevent *bev, void *const self)
            {
                IService *This = static_cast<IService *>(self);
                static_cast<Commander *>(This->commander_)->ThrowAllEvents(bev);
                This->ActorMessagePolling();
            }
            bool IService::Reply(context_ptr &context, response_ptr &res)
            {
                messages_lock_.lock();
                messages_context_.push_back(std::move(context));
                messages_data_.push_back(std::move(res));
                //messages_[std::move(context)] = std::move(res);
                messages_lock_.unlock();

                return static_cast<Commander *>(commander_)->Trigger();
            }
            void IService::ActorMessagePolling()
            {
                //int size = 0;
                std::vector<response_ptr> messages_data;
                std::vector<context_ptr> messages_context;
                messages_lock_.lock();
                messages_data.swap(messages_data_);
                messages_context.swap(messages_context_);
                messages_lock_.unlock();

                auto context_iter = messages_context.begin();
                auto data_iter = messages_data.begin();
                for (; context_iter != messages_context.end(), data_iter != messages_data.end();
                     ++context_iter, ++data_iter)
                {
                    OnMessage(*context_iter, *data_iter);
                }
            }
            void IService::OnMessage(const context_ptr &context, const response_ptr &res)
            {
                // 1.Get request via serial number of msg
                request_context_t *rc = static_cast<request_context_t *>(context.get());
                if (rc == NULL)
                {
                    ERR(this->name() << " The reply context is null! Or it's an notification!");
                    return;
                }
                request_t *req = rc->request;//context->value<request_t *>();
                // 2.Set Http header
                int code = res->status_code();
                std::string reason(res->reason());
                struct evkeyvalq *output_headers = req->request->output_headers;
                std::for_each(res->header().begin(), res->header().end(),
                              [output_headers](const std::pair<std::string, std::string> &key_val) {
                                  evhttp_add_header(output_headers,
                                                    key_val.first.c_str(),
                                                    key_val.second.c_str());
                              });
                // 3.Send msg
                evbuffer *evb = evbuffer_new();

                void *data = static_cast<void *>(const_cast<char *>(res->content().c_str()));
                size_t size = res->content().size();

                evbuffer_add(evb, data, size);
                evhttp_send_reply(req->request, code, reason.c_str(), evb);
                evbuffer_free(evb);

                dealloc_request(req);
            }
        }
        //command
        namespace http
        {
            void IService::CmdCall(struct bufferevent *bev, void *const self)
            {
                IService *This = static_cast<IService *>(self);

                Commander::command_t *cmd = static_cast<Commander *>(This->commander_)->RecvCommand(bev);
                This->CommandPolling(static_cast<void *>(cmd));
                static_cast<Commander *>(This->commander_)->ReplyCommand(bev);
            }
            void IService::CommandPolling(void *cmd)
            {

                Commander::command_t *command = static_cast<Commander::command_t *>(cmd);
                if (command == NULL)
                {
                    return;
                }
                switch (command->code)
                {
                    case BIND:
                    {
                        bind_t *bind = static_cast<bind_t *>(command);
                        bind->result = OnBind(bind->params.url, bind->params.callback) ? 1 : 0;
                    }
                    break;
                    case UNBIND:
                    {
                        unbind_t *unbind = static_cast<unbind_t *>(command);
                        unbind->result = OnUnBind(unbind->params.url) ? 1 : 0;
                    }
                    break;
                    case TERM:
                    {
                        stop_ = true;
                        event_base_loopbreak(base_);
                    }
                    break;
                    default:
                        break;
                }
            }

            bool IService::Bind(const std::string &url, request_fn callback)
            {
                bind_t command(url, callback);

                static_cast<Commander *>(commander_)->SendCommand<bind_t>(command);
                return command.result ? true : false;
            }
            bool IService::OnBind(const std::string &url, request_fn callback)
            {
                // 1.Parse URL
                bool rc;
                std::string host, path, err;
                uint16_t port;
                rc = ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " OnBind fail to parse url: " << err << url);
                    return false;
                }
                // 2.Alloc domain_t and service_t
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    d = alloc_domain(host, port);

                    // bind to the url
                    struct evhttp *http = evhttp_new(base_);
                    if (http == NULL)
                    {
                        ERR(this->name() << " OnBind: create http socket failed: ");
                        dealloc_domain(d);
                        return false;
                    }
                    d->http = http;
                    int rc = evhttp_bind_socket(d->http, d->host.c_str(), d->port);
                    if (rc != 0)
                    {
                        ERR(this->name() << " OnBind fail to bind socket to url : " << url);
                        dealloc_domain(d);
                        return false;
                    }
                    evhttp_set_allowed_methods(d->http, EVHTTP_REQ_GET | EVHTTP_REQ_PUT | EVHTTP_REQ_POST | EVHTTP_REQ_DELETE);
                    evhttp_set_gencb(d->http, GenericRequest, d);
                }

                service_t *s = find_service(d, path);
                if (s == NULL)
                {
                    s = alloc_service(d, path, callback);
                }
                else
                {
                    ERR(this->name() << "OnBind already in-use url " << url);
                    return false;
                }
                // 3.Set the callback function, invoked when received messages
                int code = evhttp_set_cb(d->http, path.c_str(), SpecificRequest, s);
                if (code == -1)
                {
                    ERR(this->name() << "OnBind fail: the callback existed already!");
                }
                if (code == -2)
                {
                    ERR(this->name() << "OnBind fail to set the callback function!");
                }
                INFO(this->name() << " Bind successfully on url: " << url);
                return true;
            }
            bool IService::UnBind(const std::string &url)
            {
                unbind_t command(url);

                static_cast<Commander *>(commander_)->SendCommand<unbind_t>(command);
                return command.result ? true : false;
            }
            bool IService::OnUnBind(const std::string &url)
            {
                //1.parse url
                bool rc;
                std::string host, path, err;
                uint16_t port;
                rc = ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " OnUnBind fail to parse url: " << err << url);
                    return false;
                }
                //2.find service
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    ERR(this->name() << " OnUnBind fail to find domain: " << url);
                    return false;
                }
                service_t *s = find_service(d, path);
                if (s == NULL)
                {
                    ERR(this->name() << " OnUnBind fail to find service: " << url);
                    return false;
                }

                /* 
				 Note: as `dealloc_service(s)` and `dealloc_domain` will close 
				 the connection immediately, the 501 answer will not be received
				 in the client. Hence, the process will be done in the app layer.
				*/
                //3.send reply if not answered
                //std::for_each(s->requests.begin(), s->requests.end(),
                //              [](std::shared_ptr<request_t> &req) {
                //                  evhttp_send_reply(req->request, 501, "Not Implemented", NULL);
                //              });

                //4.deallocate resources
                dealloc_service(s);
                if (d->services.empty())
                {
                    dealloc_domain(d);
                }
                INFO(this->name() << " UnBind successfully on url: " << url);

                return true;
            }
        }
        //----------------------IMPLEMENT FUNCTIONS------------------------
        namespace http
        {
            IService::domain_t *IService::find_domain(const std::string &host, uint16_t port)
            {
                auto it = std::find_if(domains_.begin(), domains_.end(),
                                       [&host, port](domain_t *current_domain) {
                                           return (current_domain->host == host &&
                                                   current_domain->port == port);
                                       });
                if (it != domains_.end())
                {
                    return *it;
                }
                return NULL;
            }

            IService::domain_t *IService::alloc_domain(const std::string &host, uint16_t port)
            {
                domain_t *d = new domain_t(host, port);
                domains_.push_back(d);
                return d;
            }

            IService::service_t *IService::find_service(domain_t *d, const std::string &path)
            {
                //typedef std::pair<std::string, service_t *> path_service;
                if ((d->services)[path] != NULL)
                {
                    return (d->services)[path];
                }
                return NULL;
            }

            IService::service_t *IService::alloc_service(domain_t *d,
                                                         const std::string &path,
                                                         request_fn callback)
            {
                service_t *s = new service_t(path, callback, d);
                (d->services)[path] = s;

                return s;
            }


            IService::request_t *IService::alloc_request(struct evhttp_request *req, service_t *s)
            {
                request_t *r = new request_t(req, s);
                s->requests.push_back(r);
                return r;
            }
            void IService::dealloc_request(request_t *request, bool all_deallocted)
            {
                // erase from request list and deallocate itself
                std::list<request_t *> &requests = request->service->requests;

                if (!all_deallocted)
                {
                    auto it = std::find_if(requests.begin(), requests.end(),
                                           [request](request_t *current_req) {
                                               return (current_req == request);
                                           });
                    if (it != requests.end())
                    {
                        requests.erase(it);
                    }
                }
                delete request;
            }
            void IService::dealloc_service(service_t *s, bool all_deallocted)
            {
                // deallocate request list
                std::for_each(s->requests.begin(), s->requests.end(),
                              [this](request_t *current_req) {
                                  this->dealloc_request(current_req, true);
                              });

                // erase from service list and deallocate itself
                evhttp_del_cb(s->domain->http, s->path.c_str());

                if (!all_deallocted)
                {
                    typedef std::pair<std::string, service_t *> path_service;
                    auto it = std::find_if((s->domain->services).begin(), (s->domain->services).end(),
                                           [s](const path_service &current_service) {
                                               return (current_service.first == s->path);
                                           });
                    if (it != (s->domain->services).end())
                    {
                        (s->domain->services).erase(it);
                    }
                }

                delete s;
            }
            void IService::dealloc_domain(domain_t *d, bool all_deallocted)
            {
                // deallocate service list
                typedef std::pair<std::string, service_t *> path_service;

                std::for_each((d->services).begin(), (d->services).end(),
                              [this](const path_service &current_service) {
                                  if (current_service.second != NULL)
                                  {
                                      this->dealloc_service(current_service.second, true);
                                  }
                              });

                // erase from domains and deallocate itself
                evhttp_free(d->http);

                if (!all_deallocted)
                {
                    std::list<domain_t *>::iterator it =
                        std::find_if(domains_.begin(), domains_.end(),
                                     [d](domain_t *current_domain) {
                                         return (current_domain->host == d->host &&
                                                 current_domain->port == d->port);
                                     });
                    if (it != domains_.end())
                    {
                        domains_.erase(it);
                    }
                }
                delete d;
            }

            void IService::dealloc_domains()
            {
                std::for_each(domains_.begin(), domains_.end(),
                              [this](domain_t *current_domain) {
                                  this->dealloc_domain(current_domain, true);
                              });
                domains_.clear();
            }
        }
    }
}
//#endif
