#include "helper.hpp"
#include <wsf/transport/websocket/IBroker.hpp>

namespace wsf
{
    namespace transport
    {
        namespace websocket
        {
            namespace
            {
                enum command_code
                {
                    BIND = 1,
                    UNBIND
                };
                struct bind_t : public Commander::command_t
                {
                    bind_t(const std::string &url_,
                           broker_msg_fn callback_)
                        : command_t(BIND)
                        , params(url_, callback_)
                        , result(0)
                    {
                    }
                    struct parameter_t
                    {
                        std::string url;
                        broker_msg_fn callback;

                        parameter_t(const std::string &url_,
                                    broker_msg_fn callback_)
                            : url(url_)
                            , callback(callback_)
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
            }
            namespace
            {

                void lws_uv_walk_cb(uv_handle_t *handle, void *arg)
                {
                    uv_close(handle, NULL);
                }
                int index = 0;
                struct per_session_data
                {
                    std::list<std::shared_ptr<std::string>> *notify_msg;
                    std::string token;
                };
                std::map<uv_timer_t *, struct lws_context *> lws_map;
                ::wsf::transport::websocket::IBroker *ws_broker = NULL;

                //typedef std::list<per_session_data *> peer_connections;
                std::string partial_data;
                std::map<std::string, per_session_data *> all_peers;
            }
            namespace
            {
                int ws_callback(struct lws *wsi,
                                enum lws_callback_reasons reason,
                                void *user_data,
                                void *in,
                                size_t len)
                {
                    per_session_data *peer_session = static_cast<per_session_data *>(user_data);

                    switch (reason)
                    {
                        case LWS_CALLBACK_ESTABLISHED:
                        {
                            //OnOpen
                            peer_session->notify_msg = NULL;
                            char name[16] = {0};
                            lws_get_peer_simple(wsi, name, 16);

                            // encode the peer host to a certain token
                            peer_session->token = std::to_string(index++) + "@" + std::string(name);

                            lws_context *context = lws_get_context(wsi);
                            ws_broker->OnOpen(context, peer_session);
                            //if (!rc)
                            //{
                            //    //close connection
                            //    std::string close_reason("Only one connection allowed!");
                            //    lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL,
                            //                     reinterpret_cast<unsigned char *>(const_cast<char *>(close_reason.c_str())),
                            //                     close_reason.size());
                            //    return -1;
                            //}
                        }
                        break;

                        case LWS_CALLBACK_SERVER_WRITEABLE:
                        {
                            if (peer_session->notify_msg != NULL && !peer_session->notify_msg->empty())
                            {
                                std::string &data = *(peer_session->notify_msg->front());
                                size_t data_len = data.size() - LWS_PRE;
                                if (data_len < 0)
                                {
                                    //lwsl_err("ERROR message: no data\n");
                                    return -1;
                                }
                                int rc = lws_write(wsi, (unsigned char *)&data[LWS_PRE], data_len, LWS_WRITE_BINARY);
                                if (rc < 0)
                                {
                                    //lwsl_err("ERROR %d writing to socket, hanging up\n", rc);
                                    return -1;
                                }
                                if ((unsigned int)rc < data_len)
                                {
                                    //lwsl_err("Partial write\n");
                                    return -1;
                                }
                                peer_session->notify_msg->pop_front();
                                //lwsl_notice("notify msg length:%d\n", rc);
                            }
                        }
                        break;

                        case LWS_CALLBACK_RECEIVE:
                        {
                            std::string data(len, 0);
                            memcpy(&data[0], in, len);
                            partial_data.append(data);
                            if (lws_is_final_fragment(wsi))
                            {
                                lws_context *context = lws_get_context(wsi);
                                ws_broker->OnMessage(partial_data, context, peer_session);
                                partial_data.clear();
                            }
                        }
                        break;


                        ///* when the callback is used for client operations --> */

                        case LWS_CALLBACK_CLOSED:
                        {
                            //OnClosed
                            lws_context *context = lws_get_context(wsi);
                            ws_broker->OnClose(context, peer_session);
                        }
                        break;

                        default:
                            break;
                    }

                    return 0;
                }


                struct lws_protocols protocols[] = {
                    /* first protocol must always be HTTP handler */

                    {
                        "", /* name - can be overridden with -e */
                        ws_callback,
                        sizeof(struct per_session_data), /* per_session_data_size */
                        1024,
                    },
                    {
                        NULL, NULL, 0 /* End of list */
                    }};

                const struct lws_extension exts[] = {
                    {"permessage-deflate",
                     lws_extension_callback_pm_deflate,
                     "permessage-deflate; client_no_context_takeover; client_max_window_bits"},
                    {"deflate-frame",
                     lws_extension_callback_pm_deflate,
                     "deflate_frame"},
                    {NULL, NULL, NULL /* terminator */}};

                static void uv_timeout_cb(uv_timer_t *w)
                {
                    if (lws_map.find(w) != lws_map.end())
                    {
                        lws_callback_on_writable_all_protocol(lws_map[w], &protocols[0]);
                    }
                }
                uv_async_t close_async;
                void stop_loop(uv_async_t *handle)
                {
                    uv_close((uv_handle_t *)handle, NULL);
                    uv_stop(handle->loop);
                }
            }
        }

        //fundamental
        namespace websocket
        {
            void BrokerCmdCall(uv_async_t *handle);
            void BrokerMsgCall(uv_async_t *handle);
            bool IBroker::Initialize()
            {
                loop_ = new uv_loop_t;
                uv_loop_init(static_cast<uv_loop_t *>(loop_));
                commander_ = new websocket::Commander(static_cast<uv_loop_t *>(loop_));
                if (!static_cast<websocket::Commander *>(commander_)->Initialize())
                {
                    static_cast<websocket::Commander *>(commander_)->Terminate();
                    delete static_cast<Commander *>(commander_);
                    commander_ = NULL;
                    FATAL("Initialize" << this->name() << " Commander failed ! ");
                    return false;
                }
                uv_async_t *cmd_async = static_cast<websocket::Commander *>(commander_)->GetCmd();
                uv_async_init(static_cast<uv_loop_t *>(loop_), cmd_async, BrokerCmdCall);
                uv_async_t *msg_async = static_cast<websocket::Commander *>(commander_)->GetMsg();
                uv_async_init(static_cast<uv_loop_t *>(loop_), msg_async, BrokerMsgCall);

                lws_set_log_level(1, lwsl_emit_syslog);
                uv_async_init(static_cast<uv_loop_t *>(loop_), &close_async, stop_loop);

                if (!thread_.Start(MainLoop, this))
                {
                    FATAL("Initialize " << this->name() << " thread Start Failed! ");
                    Terminate();

                    return false;
                }
                ws_broker = this;
                return true;
            }
            void IBroker::Terminate()
            {
                uv_loop_t *loop = static_cast<uv_loop_t *>(loop_);
                stop_ = true;
                uv_async_send(&close_async);
                thread_.Join();

                if (commander_ != NULL)
                {
                    uv_async_t *cmd_async = static_cast<websocket::Commander *>(commander_)->GetCmd();
                    uv_close((uv_handle_t *)cmd_async, NULL);
                    uv_async_t *msg_async = static_cast<websocket::Commander *>(commander_)->GetMsg();
                    uv_close((uv_handle_t *)msg_async, NULL);
                }

                /* PHASE 1: stop and close things we created
				*          outside of lws */

                //stop every lws_context and its timeout_watcher
                for (std::map<uv_timer_t *, struct lws_context *>::iterator it = lws_map.begin();
                     it != lws_map.end(); ++it)
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *context = it->second;

                    if (context == NULL)
                    {
                        continue;
                    }

                    lws_libuv_stop(context);

                    uv_timer_stop(timeout_watcher);
                    uv_close((uv_handle_t *)timeout_watcher, NULL);
                    lws_context_destroy((struct lws_context *)context);
                    uv_loop_t *loop = static_cast<uv_loop_t *>(loop_);
                    if (loop != NULL)
                    {
                        int e = 100;
                        while (e--)
                        {
                            uv_run(loop, UV_RUN_NOWAIT);
                        }
                    }
                    lws_context_destroy2(context);
                }
                if (!domains_.empty())
                {
                    dealloc_domains();
                }
                //std::for_each(all_peers.begin(), all_peers.end(),
                //              [](const std::pair<std::string, peer_connections *> &key_val) {
                //                  delete key_val.second;
                //              });
                if (loop != NULL)
                {
                    int e = 100;
                    while (e--)
                    {
                        uv_run(loop, UV_RUN_NOWAIT);
                    }

                    /* PHASE 2: close anything remaining */

                    uv_walk(loop, lws_uv_walk_cb, NULL);

                    e = 100;
                    while (e--)
                    {
                        uv_run(loop, UV_RUN_NOWAIT);
                    }

                    /* PHASE 3: close the UV loop itself */

                    e = uv_loop_close(loop);
                    if (e != 0)
                    {
                        FATAL(this->name() << " uv loop close error " << uv_strerror(e));
                    }
                    /* PHASE 4: delete allocated resources*/
                    if (commander_ != NULL)
                    {
                        static_cast<websocket::Commander *>(commander_)->Terminate();
                        delete static_cast<Commander *>(commander_);
                        commander_ = NULL;
                    }
                    for (std::map<uv_timer_t *, struct lws_context *>::iterator it = lws_map.begin();
                         it != lws_map.end(); ++it)
                    {
                        uv_timer_t *timeout_watcher = it->first;
                        if (timeout_watcher != NULL)
                        {
                            delete timeout_watcher;
                        }
                    }
                    delete loop;
                    loop = NULL;
                }
            }
            void IBroker::MainLoop(void *const self)
            {
                IBroker *This = static_cast<IBroker *>(self);
                This->MainLoop();
            }
            void IBroker::MainLoop()
            {
                while (!stop_)
                {
                    uv_run(static_cast<uv_loop_t *>(loop_), UV_RUN_DEFAULT);
                }
            }
        }
        //Receive message
        namespace websocket
        {
            void IBroker::OnOpen(void *context, void *session)
            {
                per_session_data *session_data = static_cast<per_session_data *>(session);

                auto it = std::find_if(domains_.begin(), domains_.end(),
                                       [&](domain_t *d) {
                                           return d->context == context;
                                       });
                if (it != domains_.end())
                {
                    // allocate resources
                    session_data->notify_msg = new std::list<std::shared_ptr<std::string>>;
                    // add connection
                    std::list<per_session_data *> &subscribers =
                        *static_cast<std::list<per_session_data *> *>((*it)->subscribers);
                    subscribers.push_back(session_data);
                    // add connection
                    all_peers[session_data->token] = session_data;

                    (*it)->callback(msg_status::OPEN, session_data->token, std::string(""));

                    INFO(this->name() << " Get connection " << session_data->token);
                    return;
                }
            }
            //#ifdef _MSC_VER
            //#pragma warning(push)
            //#pragma warning(disable : 4129)
            //#endif
            //			void SpecificRequest(std::string &data, struct lws_context *context, per_session_data *session)
            //			{
            //				std::string reg("(.*)\\r\\nMessage:(.*)\\r\\nContent-Type:(.*)\\r\\n");
            //				boost::regex pattern(reg);
            //				boost::smatch result;
            //				bool valid = boost::regex_match(data, result, pattern);
            //				if (valid)
            //				{
            //					std::string &topic = result[1].str();
            //					std::string &message = result[2].str();
            //					std::string &content_type = result[3].str();
            //					//parse message and check if it's a subscribe message
            //					if (message == std::string("Subscribe") &&
            //						content_type == std::string("application\/octet-stream"))
            //					{
            //						ws_broker->OnMessage(topic, context, session);
            //					}
            //				}
            //			}
            //#ifdef _MSC_VER
            //#pragma warning(pop)
            //#endif
            //			bool IsSubscribed(std::list<per_session_data *> &subscribers, per_session_data *session)
            //			{
            //				for (std::list<per_session_data *>::iterator suber = subscribers.begin();
            //					suber != subscribers.end(); ++suber)
            //				{
            //					if (*suber == session)
            //					{
            //						return true;
            //					}
            //				}
            //				return false;
            //			}
            void IBroker::OnMessage(std::string &data, void *context, void *session)
            {
                per_session_data *session_data = static_cast<per_session_data *>(session);

                for (std::list<domain_t *>::iterator domain = domains_.begin();
                     domain != domains_.end(); ++domain)
                {
                    if ((*domain)->context == context)
                    {
                        if ((*domain)->callback != NULL)
                        {
                            (*domain)->callback(msg_status::MESSAGE, session_data->token, data);
                            DEBUG(this->name() << " Receive a msg from peer: " << session_data->token);
                        }
                        break;
                    }
                }
            }
        }
        //send message
        namespace websocket
        {
            bool IBroker::PushMsg(const std::list<std::string> &peers, const std::string &data)
            {
                messages_lock_.lock();
                messages_to_.push_back(peers);
                messages_data_.push_back(data);
                messages_lock_.unlock();

                uv_async_t *msg_async = static_cast<websocket::Commander *>(commander_)->GetMsg();
                uv_async_send(msg_async);
                return true;
            }
            void BrokerMsgCall(uv_async_t *handle)
            {
                if (ws_broker != NULL)
                {
                    ws_broker->MessagePolling();
                }
            }
            void IBroker::MessagePolling()
            {
                std::vector<std::list<std::string>> messages_to;
                std::vector<std::string> messages_data;
                messages_lock_.lock();
                messages_to.swap(messages_to_);
                messages_data.swap(messages_data_);
                messages_lock_.unlock();

                auto iter_urls = messages_to.begin();
                auto iter_data = messages_data.begin();
                for (; iter_urls != messages_to.end(), iter_data != messages_data.end();
                     ++iter_urls, ++iter_data)
                {
                    OnMessage(*iter_urls, *iter_data);
                }
            }
            void IBroker::OnMessage(const std::list<std::string> &peers, const std::string &data)
            {
                // 1.extract string data from msg
                std::string header(LWS_PRE, 0);
                std::shared_ptr<std::string> msg = std::make_shared<std::string>(header + data);
                // 2.Send msg to all subscribers
                std::for_each(peers.begin(), peers.end(),
                              [&](const std::string &peer_token) {
                                  auto it = all_peers.find(peer_token);
                                  if (it != all_peers.end())
                                  {
                                      it->second->notify_msg->push_back(msg);
                                  }
                              });
            }
        }
        //command
        namespace websocket
        {
            void BrokerCmdCall(uv_async_t *handle)
            {
                if (ws_broker != NULL)
                {
                    ws_broker->CommandPolling();
                }
            }
            void IBroker::CommandPolling()
            {
                Commander::command_t *command = NULL;

                command = static_cast<websocket::Commander *>(commander_)->RecvCommand();
                if (command == NULL)
                {
                    static_cast<websocket::Commander *>(commander_)->ReplyCommand();
                    ERR(this->name() << " received an invalid command! ");
                    return;
                }
                switch (command->code)
                {
                    case BIND:
                    {
                        bind_t *bind = static_cast<bind_t *>(command);
                        bind->result = OnBind(bind->params.url, bind->params.callback) ? 1 : 0;
                        static_cast<websocket::Commander *>(commander_)->ReplyCommand();
                    }
                    break;
                    case UNBIND:
                    {
                        unbind_t *unbind = static_cast<unbind_t *>(command);
                        unbind->result = OnUnBind(unbind->params.url) ? 1 : 0;
                        static_cast<websocket::Commander *>(commander_)->ReplyCommand();
                    }
                    break;
                    default:
                        break;
                }
                return;
            }
            bool IBroker::Bind(const std::string &url, broker_msg_fn callback)
            {
                bind_t command(url, callback);
                //send command
                static_cast<websocket::Commander *>(commander_)->SendCommand<bind_t>(command);
                //wait for reply
                static_cast<websocket::Commander *>(commander_)->WaitForReply();

                return command.result ? true : false;
            }
            bool IBroker::OnBind(const std::string &url, broker_msg_fn callback)
            {
                // 1.Parse URL
                bool rc = false;
                std::string host, path, err;
                uint16_t port;
                rc = websocket::ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " Bind fail to parse url: " << err << url);
                    return false;
                }
                // 2.Find and alloc domain
                domain_t *d = find_domain(host, port);
                if (d != NULL)
                {
                    WARN(this->name() << " The socket has been binded: "
                                      << host << ":" << port);
                    return true;
                }

                //create libwebsockets context
                struct lws_context_creation_info info;
                memset(&info, 0, sizeof info);
                int opts = 0;
                //char ads_port[256 + 30];
                char address[256] = {0};
                if (host.size() >= 256)
                {
                    ERR(this->name() << " invalid host: " << host);
                    return false;
                }
                if (host == std::string("127.0.0.1"))
                {
                    std::string tmp("localhost");
                    strncpy(address, tmp.c_str(), tmp.size());
                }
                else
                {
                    strncpy(address, host.c_str(), host.size());
                }

                info.port = port;
                info.iface = NULL;
                info.protocols = protocols;
                info.gid = -1;
                info.uid = -1;
                info.extensions = exts;
                info.max_http_header_pool = 1;
                info.timeout_secs = 5;
                info.options = opts | LWS_SERVER_OPTION_LIBUV;

                struct lws_context *context = lws_create_context(&info);
                if (context == NULL ||
                    lws_uv_initloop(context, static_cast<uv_loop_t *>(loop_), 0) != 0)
                {
                    ERR(this->name() << " Failed to create or initialize lws context! ");
                    lws_libuv_stop(context);
                    lws_context_destroy(context);
                    int e = 5;
                    while (e--)
                    {
                        uv_run(static_cast<uv_loop_t *>(loop_), UV_RUN_NOWAIT);
                    }
                    lws_context_destroy2(context);

                    return false;
                }
                //callback protocol every 100ms
                uv_timer_t *timeout_watcher = new uv_timer_t;
                lws_map[timeout_watcher] = context;
                uv_timer_init(lws_uv_getloop(context, 0), timeout_watcher);
                uv_timer_start(timeout_watcher, uv_timeout_cb, 100, 100);


                // alloc domain
                d = alloc_domain(host, port);
                d->context = context;
                d->callback = callback;
                INFO(this->name() << " Create websocket broker completely! ");

                return true;
            }
            void IBroker::OnClose(void *context, void *session)
            {
                per_session_data *session_data = static_cast<per_session_data *>(session);
                if (session_data->notify_msg != NULL)
                {
                    delete session_data->notify_msg;
                    session_data->notify_msg = NULL;
                }

                auto iter = std::find_if(domains_.begin(), domains_.end(),
                                         [&](domain_t *d) {
                                             return d->context == context;
                                         });
                if (iter != domains_.end())
                {
                    std::list<per_session_data *> &subscribers =
                        *static_cast<std::list<per_session_data *> *>((*iter)->subscribers);
                    subscribers.remove(session_data);

                    (*iter)->callback(msg_status::CLOSED, session_data->token, std::string(""));

                    INFO(this->name() << " Remove DisConnection " << session_data->token);
                }

                all_peers.erase(session_data->token);
                //auto it = all_peers.find(session_data->token);
                //if (it != all_peers.end())
                //{
                //    it->second->remove(session_data->token);
                //}

                //for (std::list<domain_t *>::iterator domain = domains_.begin();
                //     domain != domains_.end(); ++domain)
                //{
                //    if ((*domain)->context == context)
                //    {
                //        for (std::list<service_t *>::iterator service = (*domain)->services.begin();
                //             service != (*domain)->services.end(); ++service)
                //        {
                //            std::list<per_session_data *> &subscribers =
                //                *static_cast<std::list<per_session_data *> *>((*service)->subscribers);

                //            for (std::list<per_session_data *>::iterator session = subscribers.begin();
                //                 session != subscribers.end(); ++session)
                //            {
                //                if (*session == session_data)
                //                {
                //                    if (session_data->notify_msg != NULL)
                //                    {
                //                        delete session_data->notify_msg;
                //                    }
                //                    subscribers.erase(session);
                //                    break;
                //                }
                //            }
                //        }
                //    }
                //}
            }

            bool IBroker::UnBind(const std::string &url)
            {
                unbind_t command(url);
                //send command
                static_cast<websocket::Commander *>(commander_)->SendCommand<unbind_t>(command);
                //wait for reply
                static_cast<websocket::Commander *>(commander_)->WaitForReply();

                return command.result ? true : false;
            }
            bool IBroker::OnUnBind(const std::string &url)
            {
                // 1.Parse URL
                bool rc = false;
                std::string host, path, err;
                uint16_t port;
                rc = websocket::ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " UnBind fail to parse url: " << err << url);
                    return false;
                }
                // 2.find domain
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    WARN(this->name() << " This socket hasn't been binded: "
                                      << host << ":" << port);
                    return false;
                }
                // 3.deallocate domain
                struct lws_context *curr_context = static_cast<lws_context *>(d->context);

                dealloc_domain(d);

                // 4.close connections
                for (std::map<uv_timer_t *, struct lws_context *>::iterator it = lws_map.begin();
                     it != lws_map.end(); ++it)
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *context = it->second;

                    if (curr_context == context)
                    {
                        lws_libuv_stop(context);

                        uv_timer_stop(timeout_watcher);
                        uv_close((uv_handle_t *)timeout_watcher, NULL);
                        lws_context_destroy(context);

                        uv_loop_t *loop = static_cast<uv_loop_t *>(loop_);
                        if (loop != NULL)
                        {
                            int e = 5;
                            while (e--)
                            {
                                uv_run(loop, UV_RUN_NOWAIT);
                            }
                        }
                        lws_context_destroy2(context);

                        if (timeout_watcher != NULL)
                        {
                            delete timeout_watcher;
                        }
                        lws_map.erase(it);
                        break;
                    }
                }
                INFO(this->name() << " Close websocket broker completely! ");
                return true;
            }
        }

        namespace websocket
        {
            //----------------------IMPLEMENT FUNCTIONS------------------------
            IBroker::domain_t *IBroker::find_domain(const std::string &host, uint16_t port)
            {
                for (std::list<domain_t *>::iterator it = domains_.begin(); it != domains_.end(); it++)
                {
                    if ((*it)->host == host && (*it)->port == port)
                    {
                        return *it;
                    }
                }
                return NULL;
            }
            IBroker::domain_t *IBroker::alloc_domain(const std::string &host, uint16_t port)
            {
                domain_t *d = new domain_t(host, port);
                d->subscribers = new std::list<per_session_data *>;
                domains_.push_back(d);
                return d;
            }

            void IBroker::dealloc_domain(domain_t *d)
            {
                // close every connection
                // here is just deallocate the resources
                std::list<per_session_data *> &subscribers = *static_cast<std::list<per_session_data *> *>(d->subscribers);
                if (!subscribers.empty())
                {
                    for (std::list<per_session_data *>::iterator it = subscribers.begin();
                         it != subscribers.end(); ++it)
                    {
                        if ((*it)->notify_msg != NULL)
                        {
                            delete (*it)->notify_msg;
                            (*it)->notify_msg = NULL;
                        }
                        INFO(this->name() << " Close connection " << (*it)->token);
                    }
                    //remove from global table
                    std::for_each(subscribers.begin(), subscribers.end(),
                                  [](per_session_data *peer) {
                                      all_peers.erase(peer->token);
                                  });
                }
                delete static_cast<std::list<per_session_data *> *>(d->subscribers);


                // erase from domains and deallocate itself
                for (std::list<domain_t *>::iterator it = domains_.begin(); it != domains_.end(); ++it)
                {
                    if ((*it)->host == d->host && (*it)->port == d->port)
                    {
                        domains_.erase(it);
                        break;
                    }
                }
                delete d;
            }

            void IBroker::dealloc_domains()
            {
                std::list<domain_t *>::iterator iter;
                for (std::list<domain_t *>::iterator it = domains_.begin(); it != domains_.end();)
                {
                    iter = it++;
                    dealloc_domain(*iter);
                }
                if (!domains_.empty())
                {
                    ERR(this->name() << " dealloc_domains: The deallocated domains list isn't empty! ");
                }
            }
        }
    }
}
// #endif
