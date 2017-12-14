#include "helper.hpp"
#include <wsf/transport/websocket/IProxy.hpp>

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
                    CONNECT = 1,
                    DISCONNECT
                };
                struct connect_t : public Commander::command_t
                {
                    connect_t(const std::string &url_,
                              notify_fn callback_)
                        : command_t(CONNECT)
                        , params(url_, callback_)
                        , result(0)
                    {
                    }
                    struct parameter_t
                    {
                        std::string url;
                        notify_fn callback;

                        parameter_t(const std::string &url_,
                                    notify_fn callback_)
                            : url(url_)
                            , callback(callback_)
                        {
                        }
                    };

                    typedef uint8_t return_t;

                    parameter_t params;
                    return_t result;
                };
                struct disconnect_t : public Commander::command_t
                {
                    disconnect_t(const std::string &url_)
                        : command_t(DISCONNECT)
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
                struct per_session_data
                {
                    typedef std::unique_ptr<std::string> data_ptr;
                    std::vector<data_ptr> msgs;
                };

                ::wsf::transport::websocket::IProxy *ws_proxy = NULL;
                std::string partial_data;

                //std::map<uv_timer_t *, struct lws_context *> proxy_lws_map;
                std::map<uv_timer_t *, struct lws_context *> garbge_collection;

                std::map<struct lws_context *, per_session_data *> session_map;
            }
			std::map<uv_timer_t *, struct lws_context *> proxy_lws_map;

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


                        ///* when the callback is used for client operations --> */

                        case LWS_CALLBACK_CLOSED:
                        {
                            //OnClosed
                            lws_context *context = lws_get_context(wsi);
                            ws_proxy->OnClose(context);
                        }
                        break;
                        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                        {
                            //OnFail
                            // deallocate the domain
                            lws_context *context = lws_get_context(wsi);
                            ws_proxy->OnFail(context);
                        }
                        break;

                        case LWS_CALLBACK_CLIENT_ESTABLISHED:
                        {
                            //OnOpen
                            lws_context *context = lws_get_context(wsi);
                            ws_proxy->OnOpen(context);
                            session_map[context] = peer_session;
                        }

                        break;

                        case LWS_CALLBACK_CLIENT_RECEIVE:
                        {
                            //OnMessage
                            std::string data(len, 0);
                            memcpy(&data[0], in, len);
                            partial_data.append(data);
                            if (lws_is_final_fragment(wsi))
                            {
                                lws_context *context = lws_get_context(wsi);
                                ws_proxy->OnMessage(context, partial_data);
                                partial_data.clear();
                            }
                        }
                        break;

                        case LWS_CALLBACK_CLIENT_WRITEABLE:
                        {
                            ////send sub msg
                            //lws_context *context = lws_get_context(wsi);
                            //if (has_sub_msgs[context])
                            //{
                            //	ws_proxy->OnOpen(wsi);
                            //}
                            if (!peer_session->msgs.empty())
                            {
                                std::for_each(peer_session->msgs.begin(), peer_session->msgs.end(),
                                              [&](std::unique_ptr<std::string> &ptr) {
                                                  std::string &data = *(ptr.get());
                                                  size_t data_len = data.size() - LWS_PRE;
                                                  if (data_len < 0)
                                                  {
                                                      //ERR(ws_proxy->logger_(), ws_proxy->name() << " Write message error: no data! ");
                                                      //lwsl_err("ERROR message: no data\n");
                                                      return;
                                                  }
                                                  int rc = lws_write(wsi, (unsigned char *)&data[LWS_PRE], data_len, LWS_WRITE_BINARY);
                                                  if (rc < 0)
                                                  {
                                                      //lwsl_err("ERROR %d writing to socket, hanging up\n", rc);
                                                      return;
                                                  }
                                                  if ((unsigned int)rc < data_len)
                                                  {
                                                      //lwsl_err("Partial write\n");
                                                      return;
                                                  }
                                              });
                                peer_session->msgs.clear();
                            }
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
                    if (!garbge_collection.empty() &&
                        garbge_collection.find(w) != garbge_collection.end())
                    {
                        uv_timer_t *timeout_watcher = w;
                        struct lws_context *context = garbge_collection[w];
                        uv_loop_t *loop = timeout_watcher->loop;

                        uv_timer_stop(timeout_watcher);
                        uv_close((uv_handle_t *)timeout_watcher, NULL);

                        lws_libuv_stop(context);
                        lws_context_destroy(context);

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
                        garbge_collection.erase(w);
                    }
                    if (proxy_lws_map.find(w) != proxy_lws_map.end())
                    {
                        lws_callback_on_writable_all_protocol(proxy_lws_map[w], &protocols[0]);
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
            void ProxyCmdCall(uv_async_t *handle);
            void ProxyMsgCall(uv_async_t *handle);
            bool IProxy::Initialize()
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
                uv_async_init(static_cast<uv_loop_t *>(loop_), cmd_async, ProxyCmdCall);
                uv_async_t *msg_async = static_cast<websocket::Commander *>(commander_)->GetMsg();
                uv_async_init(static_cast<uv_loop_t *>(loop_), msg_async, ProxyMsgCall);

                lws_set_log_level(1, lwsl_emit_syslog);
                uv_async_init(static_cast<uv_loop_t *>(loop_), &close_async, stop_loop);

                if (!thread_.Start(MainLoop, this))
                {
                    FATAL("Initialize " << this->name() << " thread Start Failed! ");
                    Terminate();

                    return false;
                }
                ws_proxy = this;
                return true;
            }
            void IProxy::Terminate()
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
                for (std::map<uv_timer_t *, struct lws_context *>::iterator it = proxy_lws_map.begin();
                     it != proxy_lws_map.end(); ++it)
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *context = it->second;
                    if (context == NULL)
                    {
                        continue;
                    }
                    uv_loop_t *loop = timeout_watcher->loop;
                    uv_timer_stop(timeout_watcher);
                    uv_close((uv_handle_t *)timeout_watcher, NULL);

                    lws_libuv_stop(context);
                    lws_context_destroy(context);

                    if (loop != NULL)
                    {
                        int e = 5;
                        while (e--)
                        {
                            uv_run(loop, UV_RUN_NOWAIT);
                        }
                    }
                    lws_context_destroy2(context);
                }
                for (std::map<uv_timer_t *, struct lws_context *>::iterator it = garbge_collection.begin();
                     it != garbge_collection.end(); ++it)
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *context = it->second;
                    if (context == NULL)
                    {
                        continue;
                    }
                    uv_loop_t *loop = timeout_watcher->loop;
                    uv_timer_stop(timeout_watcher);
                    uv_close((uv_handle_t *)timeout_watcher, NULL);

                    lws_libuv_stop(context);
                    lws_context_destroy(context);

                    if (loop != NULL)
                    {
                        int e = 5;
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
                        FATAL(this->name() << "uv loop close error "
                                           << uv_strerror(e));
                    }
                    /* PHASE 4: delete allocated resources*/
                    if (commander_ != NULL)
                    {
                        static_cast<websocket::Commander *>(commander_)->Terminate();
                        delete static_cast<Commander *>(commander_);
                        commander_ = NULL;
                    }
                    for (std::map<uv_timer_t *, struct lws_context *>::iterator it = proxy_lws_map.begin();
                         it != proxy_lws_map.end(); ++it)
                    {
                        uv_timer_t *timeout_watcher = it->first;
                        if (timeout_watcher != NULL)
                        {
                            delete timeout_watcher;
                        }
                    }
                    for (std::map<uv_timer_t *, struct lws_context *>::iterator it = garbge_collection.begin();
                         it != garbge_collection.end(); ++it)
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
            void IProxy::MainLoop(void *const self)
            {
                IProxy *This = static_cast<IProxy *>(self);
                This->MainLoop();
            }
            void IProxy::MainLoop()
            {
                while (!stop_)
                {
                    uv_run(static_cast<uv_loop_t *>(loop_), UV_RUN_DEFAULT);
                }
            }
        }
        //send message
        namespace websocket
        {
            void ProxyMsgCall(uv_async_t *handle)
            {
                if (ws_proxy != NULL)
                {
                    ws_proxy->MessagePolling();
                }
            }
            void IProxy::SendMsg(const std::string &url, const std::string &data)
            {
                std::string pre_space(LWS_PRE, 0);
                messages_lock_.lock();
                messages_to_.push_back(url);
                std::unique_ptr<std::string> str(new std::string(pre_space + data));
                messages_data_.push_back(std::move(str));
                messages_lock_.unlock();

                uv_async_t *msg_async = static_cast<websocket::Commander *>(commander_)->GetMsg();
                uv_async_send(msg_async);
                return;
            }
            void IProxy::MessagePolling()
            {
                std::vector<std::string> messages_to;
                std::vector<std::unique_ptr<std::string>> messages_data;
                messages_lock_.lock();
                messages_to.swap(messages_to_);
                messages_data.swap(messages_data_);
                messages_lock_.unlock();

                auto iter_url = messages_to.begin();
                auto iter_data = messages_data.begin();
                for (; iter_url != messages_to.end(), iter_data != messages_data.end();
                     ++iter_url, ++iter_data)
                {
                    OnMessage(*iter_url, *iter_data);
                }
            }
            void IProxy::OnMessage(const std::string &url, std::unique_ptr<std::string> &data)
            {
                bool rc;
                std::string host, path, err;
                uint16_t port;
                rc = ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " SendMsg fail to parse url: " << err << url);
                    return;
                }
                // 2.Find domain
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    ERR(this->name() << " There's no connection to the url: " << url);
                    return;
                }
                struct lws_context *context = static_cast<struct lws_context *>(d->connection->context);
                if (context == NULL)
                {
                    ERR(this->name() << " There's no connection to the url: " << url);
                    return;
                }
                per_session_data *peer_session = session_map[context];
                if (peer_session == NULL)
                {
                    ERR(this->name() << " There's no connection to the url: " << url);
                    return;
                }
                peer_session->msgs.push_back(std::move(data));
            }
        }
        //receive message
        namespace websocket
        {
            void IProxy::OnMessage(void *context, std::string &msg)
            {
                std::list<domain_t *>::iterator iter;
                for (std::list<domain_t *>::iterator it = domains_.begin();
                     it != domains_.end(); ++it)
                {
                    IProxy::connection_t *conn = (*it)->connection;
                    if (conn->context == context)
                    {
                        conn->callback(msg_status::MESSAGE, msg);
                    }
                }
            }
        }

        //command
        namespace websocket
        {
            void ProxyCmdCall(uv_async_t *handle)
            {
                if (ws_proxy != NULL)
                {
                    ws_proxy->CommandPolling();
                }
            }
            void IProxy::CommandPolling()
            {
                Commander::command_t *command = NULL;

                command = static_cast<websocket::Commander *>(commander_)->RecvCommand();
                if (command == NULL)
                {
                    static_cast<websocket::Commander *>(commander_)->ReplyCommand();
                    return;
                }
                switch (command->code)
                {
                    case CONNECT:
                    {
                        connect_t *conn = static_cast<connect_t *>(command);
                        conn->result = OnConnect(conn->params.url, conn->params.callback) ? 1 : 0;
                        static_cast<websocket::Commander *>(commander_)->ReplyCommand();
                    }
                    break;
                    case DISCONNECT:
                    {
                        disconnect_t *disconn = static_cast<disconnect_t *>(command);
                        disconn->result = OnDisConnect(disconn->params.url) ? 1 : 0;
                        static_cast<websocket::Commander *>(commander_)->ReplyCommand();
                    }
                    break;
                    default:
                        break;
                }
                return;
            }
            bool IProxy::Connect(const std::string &url, notify_fn callback)
            {
                connect_t command(url, callback);

                //send connect command
                static_cast<websocket::Commander *>(commander_)->SendCommand<connect_t>(command);
                //block for reply
                static_cast<websocket::Commander *>(commander_)->WaitForReply();

                return command.result ? true : false;
            }
            void IProxy::OnOpen(void *context)
            {
                std::list<domain_t *>::iterator domain;
                for (domain = domains_.begin(); domain != domains_.end(); ++domain)
                {
                    if ((*domain)->connection->context == context)
                    {
                        (*domain)->connection->callback(msg_status::OPEN, std::string(""));

                        INFO(this->name() << " The Connection has been established: "
                                          << (*domain)->host << ":" << (*domain)->port);
                        break;
                    }
                }
            }
            void IProxy::OnFail(void *context)
            {
                //deallocate resources
                for (std::list<domain_t *>::iterator it = domains_.begin(); it != domains_.end(); ++it)
                {
                    if ((*it)->connection->context == context)
                    {
                        ERR(this->name() << " Failed to connect to: "
                                         << (*it)->host << ":" << (*it)->port);

                        (*it)->connection->callback(msg_status::CLOSED, std::string(""));

                        dealloc_domain(*it);
                        break;
                    }
                }
                //stop lws
                struct lws_context *curr_context = static_cast<lws_context *>(context);

				if (stop_)
				{
					return;
				}
                //session_map.erase(curr_context);
                auto it = find_if(proxy_lws_map.begin(), proxy_lws_map.end(),
                                  [&](const std::pair<uv_timer_t *, struct lws_context *> &key_val) {
                                      return key_val.second == curr_context;
                                  });
                if (it != proxy_lws_map.end())
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *this_context = it->second;

                    proxy_lws_map.erase(it);
                    garbge_collection[timeout_watcher] = this_context;

                    return;

                    //{
                    //	//fail_collection[timeout_watcher] = this_context;
                    //					   //return;
                    //					   //close_connection(timeout_watcher, this_context);
                    //	uv_timer_stop(timeout_watcher);
                    //	uv_close((uv_handle_t *)timeout_watcher, NULL);
                    //
                    //	lws_libuv_stop(this_context);
                    //	lws_context_destroy(this_context);
                    //
                    //	uv_loop_t *loop = static_cast<uv_loop_t *>(loop_);
                    //	if (loop != NULL)
                    //	{
                    //		int e = 100;
                    //		while (e--)
                    //		{
                    //			uv_run(loop, UV_RUN_NOWAIT);
                    //		}
                    //	}
                    //	lws_context_destroy2(this_context);
                    //
                    //	if (timeout_watcher != NULL)
                    //	{
                    //		delete timeout_watcher;
                    //	}
                    //}
                }
            }
            void IProxy::OnClose(void *context)
            {
                std::list<domain_t *>::iterator domain;
                for (domain = domains_.begin(); domain != domains_.end(); ++domain)
                {
                    if ((*domain)->connection->context == context)
                    {
                        INFO(this->name() << " The Remote Connection has been closed: "
                                          << (*domain)->host << ":" << (*domain)->port);

                        (*domain)->connection->callback(msg_status::CLOSED, std::string(""));

                        dealloc_domain(*domain);
                        break;
                    }
                }
                //stop lws
                struct lws_context *curr_context = static_cast<lws_context *>(context);

                session_map.erase(curr_context);
                if (stop_)
                {
                    return;
                }
                auto it = find_if(proxy_lws_map.begin(), proxy_lws_map.end(),
                                  [&](const std::pair<uv_timer_t *, struct lws_context *> &key_val) {
                                      return key_val.second == curr_context;
                                  });
                if (it != proxy_lws_map.end())
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *this_context = it->second;

                    proxy_lws_map.erase(it);

                    garbge_collection[timeout_watcher] = this_context;
                    return;
                    //{
                    //	//uv_timer_stop(timeout_watcher);
                    //						//uv_close((uv_handle_t *)timeout_watcher, NULL);
                    //
                    //	lws_libuv_stop(curr_context);
                    //	lws_context_destroy(curr_context);
                    //	//garbge_collection.push_back(curr_context);
                    //	uv_loop_t *loop = static_cast<uv_loop_t *>(loop_);
                    //	if (loop != NULL)
                    //	{
                    //		int e = 100;
                    //		while (e--)
                    //		{
                    //			uv_run(loop, UV_RUN_NOWAIT);
                    //		}
                    //	}
                    //
                    //	lws_context_destroy2(curr_context);
                    //
                    //	if (timeout_watcher != NULL)
                    //	{
                    //		delete timeout_watcher;
                    //	}
                    //
                    //
                    //	//close_connection(timeout_watcher, curr_context);
                    //}
                }
            }

            bool IProxy::OnConnect(const std::string &url, notify_fn callback)
            {
                // 1.Parse URL
                bool rc;
                std::string host, path, err;
                uint16_t port;
                rc = ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " OnConnect fail to parse url: " << err << url);
                    return false;
                }

                // 2.Find domain
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    // 3.Build connection to pub url
                    //create libwebsockets context
                    struct lws_context_creation_info info;
                    memset(&info, 0, sizeof info);
                    int opts = 0;
                    char ads_port[256 + 30];
                    char address[256] = {0};
                    if (host.size() >= 256)
                    {
                        ERR(this->name() << " invalid host: " << host);
                        dealloc_domain(d);
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

                    info.port = CONTEXT_PORT_NO_LISTEN;
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
                        return false;
                    }
                    //client info
                    sprintf(ads_port, "%s:%u", address, port & 65535);

                    struct lws *wsi;
                    struct lws_client_connect_info connect_info;
                    memset(&connect_info, 0, sizeof(connect_info));

                    connect_info.context = context;
                    connect_info.address = address;
                    connect_info.port = port;
                    connect_info.ssl_connection = 0;
                    connect_info.path = path.c_str();
                    connect_info.host = ads_port;
                    connect_info.origin = ads_port;
                    connect_info.protocol = NULL;

                    wsi = lws_client_connect_via_info(&connect_info);
                    if (!wsi)
                    {
                        ERR(this->name() << " Failed to connect to broker! ");
                        lws_libuv_stop(context);
                        lws_context_destroy(context);
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
                        return false;
                    }

                    //callback protocol every 100ms
                    uv_timer_t *timeout_watcher = new uv_timer_t;
                    proxy_lws_map[timeout_watcher] = context;
                    uv_timer_init(lws_uv_getloop(context, 0), timeout_watcher);
                    uv_timer_start(timeout_watcher, uv_timeout_cb, 100, 100);

                    // alloc new domain and there's no connection
                    d = alloc_domain(host, port);
                    connection_t *conn = alloc_connection(d, callback);
                    conn->context = context;
                    //INFO(this->name() << " Create websocket connection completely! ");

                    return true;
                }

                WARN(this->name() << " The conncetion has been established: "
                                  << host << ":" << port);
                return true;
            }

            bool IProxy::DisConnect(const std::string &url)
            {
                disconnect_t command(url);

                //send disconnect command
                static_cast<websocket::Commander *>(commander_)->SendCommand<disconnect_t>(command);
                //block for reply
                static_cast<websocket::Commander *>(commander_)->WaitForReply();

                return command.result ? true : false;
            }
            bool IProxy::OnDisConnect(const std::string &url)
            {
                // 1.Parse URL
                bool rc;
                std::string host, path, err;
                uint16_t port;
                rc = ParseURI(url, host, port, path, err);
                if (!rc)
                {
                    ERR(this->name() << " OnDisConnect fail to parse url: " << err << url);
                    return false;
                }

                // 2.Find domain
                domain_t *d = find_domain(host, port);
                if (d == NULL)
                {
                    WARN(this->name() << " There is no conncetion to the url: " << url
                                      << ". Or the connection has been closed!");
                    return false;
                }

                //3. deallocate domain
                struct lws_context *curr_context = static_cast<lws_context *>(d->connection->context);
                dealloc_domain(d);

                //stop lws
                session_map.erase(curr_context);
				if (stop_)
				{
					return true;
				}
                auto it = find_if(proxy_lws_map.begin(), proxy_lws_map.end(),
                                  [&](const std::pair<uv_timer_t *, struct lws_context *> &key_val) {
                                      return key_val.second == curr_context;
                                  });
                if (it != proxy_lws_map.end())
                {
                    uv_timer_t *timeout_watcher = it->first;
                    struct lws_context *this_context = it->second;

                    proxy_lws_map.erase(it);
                    close_connection(timeout_watcher, this_context);
                }

                INFO(this->name() << " Close the connection: " << host << ":" << port);
                return true;
            }
        }

        //detail implementation
        namespace websocket
        {
            //////////////////////////////////////////////////////////////////////////
            IProxy::domain_t *IProxy::find_domain(const std::string &host, uint16_t port)
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
            IProxy::domain_t *IProxy::alloc_domain(const std::string &host, uint16_t port)
            {
                domain_t *d = new domain_t(host, port);
                domains_.push_back(d);
                return d;
            }
            IProxy::connection_t *IProxy::alloc_connection(domain_t *d, notify_fn callback)
            {
                if (d == NULL)
                {
                    return NULL;
                }

                connection_t *conn = new connection_t(d, callback);
                d->connection = conn;

                return conn;
            }

            void IProxy::close_connection(void *timeout_watcher_, void *context_)
            {
                uv_timer_t *timeout_watcher = static_cast<uv_timer_t *>(timeout_watcher_);
                struct lws_context *context = static_cast<struct lws_context *>(context_);

                uv_timer_stop(timeout_watcher);
                uv_close((uv_handle_t *)timeout_watcher, NULL);

                lws_libuv_stop(context);
                lws_context_destroy(context);

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

                if (timeout_watcher != NULL)
                {
                    delete timeout_watcher;
                }
            }

            void IProxy::dealloc_connection(connection_t *conn)
            {
                if (conn == NULL)
                {
                    return;
                }
                // erase it from domain's connection list
                conn->domain->connection = NULL;
                // deallocate itself
                delete conn;
            }

            void IProxy::dealloc_domain(domain_t *d)
            {
                // deallocate connection list
                dealloc_connection(d->connection);

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
            void IProxy::dealloc_domains()
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
