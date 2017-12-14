#include "helper.hpp"
#include <wsf/transport/http/IProxy.hpp>

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
            bool IProxy::Initialize()
            {
                base_ = event_base_new();
                if (base_ == NULL)
                {
                    FATAL("Initialize" << this->name() << " Event failed! ");
                    return false;
                }
                commander_ = new Commander();
                if (!static_cast<Commander *>(commander_)->Initialize())
                {
                    static_cast<Commander *>(commander_)->Terminate();
                    delete static_cast<Commander *>(commander_);
                    commander_ = NULL;
                    FATAL("Initialize" << this->name() << " Commander failed! ");
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
                bufferevent_setcb(cmd_bev, NULL, NULL, NULL, this);
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
                thread_ = new std::thread(&::wsf::transport::http::IProxy::MainLoop, this);
                if (thread_ == NULL)
                {
                    FATAL("Initialize " << this->name() << " thread Start Failed!");
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
            void IProxy::Terminate()
            {
                //stop_ = true;
                //event_base_loopbreak(base_);
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
                    // base freed at last
                    event_base_free(base_);
                    base_ = NULL;
                }
                if (thread_ != NULL)
                {
                    delete thread_;
                    thread_ = NULL;
                }
            }
            void IProxy::MainLoop(void *const self)
            {
                IProxy *This = static_cast<IProxy *>(self);
                This->MainLoop_();
            }
            void IProxy::MainLoop_()
            {
                while (!stop_)
                {
                    event_base_dispatch(base_);
                }
            }
        }
        //send message
        namespace http
        {
            void IProxy::MsgCall(struct bufferevent *bev, void *const self)
            {
                IProxy *This = static_cast<IProxy *>(self);
                static_cast<Commander *>(This->commander_)->ThrowAllEvents(bev);
                This->ActorMessagePolling();
            }
            bool IProxy::Request(context_ptr &context,
                                 request_ptr &req,
                                 response_fn callback)
            {
                messages_lock_.lock();
                request_messages_.push_back(make_tuple(std::move(context), std::move(req), callback));
                messages_lock_.unlock();

                return static_cast<Commander *>(commander_)->Trigger();
            }
            void IProxy::ActorMessagePolling()
            {
                std::list<std::tuple<context_ptr, request_ptr, response_fn>> messages;
                messages_lock_.lock();
                request_messages_.swap(messages);
                messages_lock_.unlock();

                std::for_each(messages.begin(), messages.end(),
                              [this](std::tuple<context_ptr, request_ptr, response_fn> &msg) {
                                  this->OnMessage(std::get<0>(msg), std::get<1>(msg), std::get<2>(msg));
                              });
            }
            void IProxy::OnMessage(context_ptr &context,
                                   request_ptr &req,
                                   response_fn callback)
            {
                // 1.Parse URL
                std::string &host = req->host();
                uint16_t port = req->port();

                // 2.Find or build connections in domain_t
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    d = alloc_domain(host, port);
                }
                if ((d->connections).empty())
                {
                    // build connections
                    for (int i = 0; i < d->MAX_CONNECTIONS; ++i)
                    {
                        struct evhttp_connection *ev_conn = evhttp_connection_base_new(base_, NULL, host.c_str(), port);
                        evhttp_connection_set_timeout(ev_conn, 30);//30s timeout

                        alloc_connection(d, ev_conn);
                    }
                }
                // 3.Generate a request and push it into the pending list of domain_t
                request_t *r = alloc_request(d, context, req, callback);
                // 4.Get next pending connection, if found, sent the request via the connection; if not found, wait.
                connection_t *conn = next_idle_connection(d);
                if (conn != NULL)
                {
                    MakeRequest(conn, r);
                }
            }
        }
        //receive message
        namespace http
        {
            void IProxy::SpecificRequest(struct evhttp_request *request, void *conn)
            {
                using namespace ::wsf::transport;

                wsf::transport::IProxy *instance = getInstance(http::Scheme());
                static_cast<IProxy *>(instance)->OnRequest(request, conn);
            }
            void IProxy::OnRequest(struct evhttp_request *request, void *conn)
            {
                connection_t *c = static_cast<connection_t *>(conn);
                response_ptr res(new response_t);
                // 1.Receive msg and set message_ptr response
                if (request == NULL)
                {
                    std::string dest_addr(std::string("http://") +
                                          c->domain->host + ":" +
                                          std::to_string(c->domain->port));
                    ERR(this->name() << " No messages received: Timeout! It may be disconnected! " << dest_addr);

                    //make response line 408 timeout
                    res->status_code() = 408;
                    res->reason() = "Request Timeout";
                    res->header().clear();
                    res->content().clear();

                    //remake the libevent http connection
                    evhttp_connection_free(c->connection);
                    c->connection = evhttp_connection_base_new(base_, NULL, c->domain->host.c_str(), c->domain->port);
                    evhttp_connection_set_timeout(c->connection, 30);//30s timeout
                }
                else
                {
                    //response line
                    int code = evhttp_request_get_response_code(request);
                    const char *code_line = evhttp_request_get_response_code_line(request);
                    res->status_code() = code;
                    res->reason() = std::string(code_line);
                    //header
                    struct evkeyvalq *header = evhttp_request_get_input_headers(request);
                    res->header().clear();
                    struct evkeyval *key_val;
                    WSF_TAILQ_FOREACH(key_val, header, next)
                    {
                        std::string key(key_val->key);
                        std::string value(key_val->value);
                        res->header()[key] = value;
                    }
                    //body
                    evbuffer *buf = evhttp_request_get_input_buffer(request);
                    size_t len = evbuffer_get_length(buf);
                    char *data = static_cast<char *>(static_cast<void *>(evbuffer_pullup(buf, -1)));
                    res->content().clear();
                    res->content().assign(data, len);
                }
                // 2.Send response back
                response_fn callback = c->request->callback;
                callback(c->request->context, res);
                // 3.Send other messages in pending requests of domain_t
                if (c->request != NULL)
                {
                    delete c->request;
                    c->request = NULL;
                }
                if (!(c->domain->pendings).empty())
                {
                    request_t *req = (c->domain->pendings).front();
                    MakeRequest(c, req);
                }
            }
        }
        //detail implementations
        namespace http
        {
            //////////////////////////////////////////////////////////////////////////
            IProxy::domain_t *IProxy::find_domain(const std::string &host, uint16_t port)
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
            IProxy::domain_t *IProxy::alloc_domain(const std::string &host, uint16_t port)
            {
                domain_t *d = new domain_t(host, port);
                domains_.push_back(d);
                return d;
            }
            IProxy::connection_t *IProxy::alloc_connection(domain_t *d, struct evhttp_connection *ev_conn)
            {
                connection_t *conn = new connection_t(ev_conn, d);
                d->connections.push_back(conn);
                return conn;
            }
            IProxy::request_t *IProxy::alloc_request(domain_t *d,
                                                     context_ptr &context,
                                                     request_ptr &req,
                                                     response_fn callback)
            {
                request_t *pending_req = new request_t(context, req, callback);
                d->pendings.push_back(pending_req);
                return pending_req;
            }
            IProxy::connection_t *IProxy::next_idle_connection(domain_t *d)
            {
                std::list<connection_t *>::iterator it =
                    std::find_if(d->connections.begin(), d->connections.end(),
                                 [](connection_t *current_conn) {
                                     return current_conn->request == NULL;
                                 });
                if (it != d->connections.end())
                {
                    return *it;
                }

                return NULL;
            }
            void IProxy::MakeRequest(connection_t *c, request_t *r)
            {
                if (c->request != NULL)
                {
                    ERR(this->name() << " MakeRequest: the connection is busy, can't handle new request! ");
                    return;
                }
                // 1.Erase the request from the pending list in domain_t
                // Push the request into the connection, via which the request will be sent
                std::list<request_t *> &requests = c->domain->pendings;
                auto it = std::find_if(requests.begin(), requests.end(),
                                       [r](request_t *current_req) {
                                           return r == current_req;
                                       });

                if (it != requests.end())
                {
                    c->request = *it;
                    (*it)->connection = c;
                    requests.erase(it);
                    //2. check method
                    enum evhttp_cmd_type method;
                    if (r->req->method() == "GET")
                    {
                        method = EVHTTP_REQ_GET;
                    }
                    else if (r->req->method() == "PUT")
                    {
                        method = EVHTTP_REQ_PUT;
                    }
                    else if (r->req->method() == "POST")
                    {
                        method = EVHTTP_REQ_POST;
                    }
                    else if (r->req->method() == "DELETE")
                    {
                        method = EVHTTP_REQ_DELETE;
                    }
                    else
                    {
                        //unsupported method
                        response_ptr msg(new response_t);
                        msg->status_code() = 501;
                        msg->reason() = std::string("Not Implemented");
                        msg->header().clear();
                        msg->content().clear();
                        response_fn callback = r->callback;
                        callback(r->context, msg);
                        c->request = NULL;
                        return;
                    }

                    // 3.Send http msg
                    r->ev_request = evhttp_request_new(SpecificRequest, c);
                    //body
                    evbuffer *buf = evhttp_request_get_output_buffer(r->ev_request);
                    void *data = static_cast<void *>(const_cast<char *>(r->req->content().c_str()));
                    size_t size = r->req->content().size();
                    evbuffer_add(buf, data, size);
                    //header
                    struct evkeyvalq *output_headers = r->ev_request->output_headers;
                    std::for_each(r->req->header().begin(), r->req->header().end(),
                                  [output_headers](const std::pair<std::string, std::string> &key_val) {
                                      evhttp_add_header(output_headers, key_val.first.c_str(), key_val.second.c_str());
                                  });
                    //encode uri
                    //1.base_path
                    std::string &path = r->req->path();
                    if (path.empty())
                    {
                        path = std::string("/");
                    }
                    if (!r->req->query().empty())
                    {
                        const std::string &key = r->req->query().begin()->first;
                        const std::string &val = r->req->query().begin()->second;
                        char *encode_key = evhttp_uriencode(key.c_str(), key.size(), 0);
                        char *encode_val = evhttp_uriencode(val.c_str(), val.size(), 0);

                        path.append("?").append(encode_key).append("=").append(encode_val);

                        free(encode_key);
                        free(encode_val);
                        //2.base_path + query
                        if (r->req->query().begin() != r->req->query().end())
                        {
                            std::for_each(++(r->req->query().begin()), r->req->query().end(),
                                          [&path](const std::pair<std::string, std::string> &key_val) {
                                              char *encode_key = evhttp_uriencode(key_val.first.c_str(), key_val.first.size(), 0);
                                              char *encode_val = evhttp_uriencode(key_val.second.c_str(), key_val.second.size(), 0);

                                              path.append("&").append(encode_key).append("=").append(encode_val);

                                              free(encode_key);
                                              free(encode_val);
                                          });
                        }
                    }
                    //3.base_path + query + fragment
                    if (!r->req->fragment().empty())
                    {
                        path.append("#").append(r->req->fragment());
                    }


                    int rc = evhttp_make_request(c->connection,
                                                 r->ev_request,
                                                 method,
                                                 path.c_str());
                    if (rc != 0)
                    {
                        ERR(this->name() << " evhttp_make_request for uri" << path);
                        return;
                    }
                }
                else
                {
                    ERR(this->name() << " MakeRequest: the request isn't in the pending list! ");
                    return;
                }
            }

            void IProxy::dealloc_domain(domain_t *d, bool all_deallocted)
            {

                // deallocate connections
                std::for_each(d->connections.begin(), d->connections.end(),
                              [](connection_t *cur_conn) {
                                  if (cur_conn->request != NULL)
                                  {
                                      response_ptr res(new response_t);
                                      res->status_code() = 503;
                                      res->reason() = std::string("Service Unavailable");
                                      res->header().clear();
                                      res->content().clear();
                                      response_fn callback = cur_conn->request->callback;
                                      callback(cur_conn->request->context, res);

                                      delete cur_conn->request;
                                      cur_conn->request = NULL;
                                  }
                                  evhttp_connection_free(cur_conn->connection);
                                  cur_conn->connection = NULL;

                                  delete cur_conn;
                                  cur_conn = NULL;
                              });
                //deallocate pending requests
                std::for_each(d->pendings.begin(), d->pendings.end(),
                              [](request_t *cur_req) {
                                  response_ptr res(new response_t);
                                  res->status_code() = 503;
                                  res->reason() = std::string("Service Unavailable");
                                  res->header().clear();
                                  res->content().clear();
                                  response_fn callback = cur_req->callback;
                                  callback(cur_req->context, res);

                                  delete cur_req;
                                  cur_req = NULL;
                              });

                if (!all_deallocted)
                {
                    auto it = std::find_if(domains_.begin(), domains_.end(),
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
            void IProxy::dealloc_domains()
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
