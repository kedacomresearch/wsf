#include "http.hpp"
#include <iostream>
#include <functional>
#include <algorithm>
#include <thread>
#include <wsf/transport/transport.hpp>
#include <wsf/util/logger.hpp>
#include <libuv/uv.h>

namespace
{
    using namespace ::wsf::transport::http;
    const std::string url = "http://127.0.0.1:8080/user";
    const std::string url_test = "http://127.0.0.1:8081/transport/http/test";

    uv_loop_t *loop_server;
    uv_loop_t *loop_test;
    uv_async_t *async_server;
    uv_async_t *async_test;
    std::thread *t1;
    std::thread *t2;
    bool stop = false;

    template <typename M, typename C = ::wsf::util::IObject>
    class auto_msg
    {
    public:
        auto_msg(M *m, C *c)
            : message(m)
            , context(c)
        {
        }

        auto_msg(std::unique_ptr<M> &m, std::unique_ptr<C> &c)
            : message(m.release())
            , context(c.release())
        {
        }

        auto_msg(const auto_msg &other)
        {
            auto_msg *m = const_cast<auto_msg *>(&other);
            message = std::move(m->message);
            context = std::move(m->context);
        }

        std::unique_ptr<M> message;
        std::unique_ptr<C> context;

    private:
    };
    typedef auto_msg<request_t> request_message_t;
    typedef auto_msg<response_t> response_message_t;


    void run(uv_loop_t *loop)
    {
        while (!stop)
        {
            uv_run(loop, UV_RUN_DEFAULT);
        }
    }

    void post_to_server(uv_async_t *handle)
    {
        uint16_t port;
        if (handle->data == NULL)
        {
            return;
        }
        request_message_t *msg = static_cast<request_message_t *>(handle->data);

        std::cout << "http server received msg!" << std::endl;
        //request
        std::cout << msg->message->method() << std::endl;
        port = msg->message->port();
        std::cout << msg->message->path() << std::endl;
        std::for_each(msg->message->query().begin(), msg->message->query().end(),
                      [](const std::pair<std::string, std::string> &key_val) {
                          std::cout << key_val.first << " : " << key_val.second << std::endl;
                      });
        std::cout << "Header: " << std::endl;
        std::for_each(msg->message->header().begin(), msg->message->header().end(),
                      [](const std::pair<std::string, std::string> &key_val) {
                          std::cout << key_val.first << " : " << key_val.second << std::endl;
                      });
        std::cout << "Body: " << std::endl;
        std::cout << msg->message->content() << std::endl
                  << std::endl;


        if (port == 8080)
        {
            response_ptr res(new response_t);
            res->status_code() = 200;
            res->reason() = "OK";
            //response
            if (msg->message->method() != "GET" ||
                msg->message->path() != "/user/id" ||
                msg->message->query()["id"] != "2 2" ||
                msg->message->query()["id2"] != "22")
            {
                res->status_code() = 501;
                res->reason() = "Internal Error";
            }

            //set header & body
            res->header()["Content-Type"] = "application/octet-stream";
            res->content().assign("name:kedacom");

            ResponseToRemote(msg->context, res);
        }
        if (port == 8081)
        {
            request_ptr request_msg_(new request_t);
            std::shared_ptr<Response> res2(new ::wsf::transport::http::Response(msg->context));
            response_fn callback = [res2](context_ptr &context, response_ptr &res) {
                //response_message_t msg(res, context);

                //request_message_t *m = const_cast<request_message_t *>(&msg);
                //response_ptr res2 = std::make_unique<response_t>();
                //set header & body
                res2->header() = res->header();
                res2->content() = res->content();
                res2->status_code() = res->status_code();
                res2->reason() = res->reason();
                res2->Reply();
            };
            request_msg_->host() = "127.0.0.1";
            request_msg_->port() = 8080;
            request_msg_->method() = "GET";
            request_msg_->path() = "/user/id";///user?q=tes t&s=some+thing
            request_msg_->query()["id"] = "2+2";
            request_msg_->query()["id2"] = "22";
            context_ptr temp;
            RequestToRemote(temp, request_msg_, callback);
        }

        delete msg;
    }

    uv_async_t close_async_server;
    uv_async_t close_async_test;
    void stop_loop(uv_async_t *handle)
    {
        uv_close((uv_handle_t *)handle, NULL);
        uv_stop(handle->loop);
    }
    void lws_uv_walk_cb(uv_handle_t *handle, void *arg)
    {
        uv_close(handle, NULL);
    }
}
namespace wsf
{
    namespace transport
    {
        namespace http_test
        {
            bool Initialize()
            {
                //////////////////////////////////////////////////////////////////////////
                loop_server = new uv_loop_t;
                uv_loop_init(loop_server);
                loop_test = new uv_loop_t;
                uv_loop_init(loop_test);

                async_server = new uv_async_t;
                uv_async_init(loop_server, async_server, post_to_server);
                async_test = new uv_async_t;
                uv_async_init(loop_test, async_test, post_to_server);

                uv_async_init(loop_server, &close_async_server, stop_loop);
                uv_async_init(loop_test, &close_async_test, stop_loop);

                //////////////////////////////////////////////////////////////////////////
                //transport initialize
                Bind(url, [&](context_ptr &context, request_ptr &req) {
                    //request_message_t msg(req, context);
                    async_server->data = new request_message_t(req, context);
                    uv_async_send(async_server);

                });
                Bind(url_test, [&](context_ptr &context, request_ptr &req) {
                    //request_message_t msg(req, context);
                    async_test->data = new request_message_t(req, context);
                    uv_async_send(async_test);

                });
                //////////////////////////////////////////////////////////////////////////
                t1 = new std::thread(run, loop_server);
                t2 = new std::thread(run, loop_test);
                return true;
            }

            void Terminate()
            {
                //////////////////////////////////////////////////////////////////////////
                UnBind(url);
                UnBind(url_test);

                stop = true;
                uv_async_send(&close_async_server);
                uv_async_send(&close_async_test);

                t1->join();
                t2->join();
                delete t1;
                delete t2;

                uv_close((uv_handle_t *)async_server, NULL);
                uv_close((uv_handle_t *)async_test, NULL);

                int e = 100;
                while (e--)
                {
                    uv_run(loop_server, UV_RUN_NOWAIT);
                    uv_run(loop_test, UV_RUN_NOWAIT);
                }

                /* PHASE 2: close anything remaining */

                uv_walk(loop_server, lws_uv_walk_cb, NULL);
                uv_walk(loop_test, lws_uv_walk_cb, NULL);
                e = 100;
                while (e--)
                {
                    uv_run(loop_server, UV_RUN_NOWAIT);
                    uv_run(loop_test, UV_RUN_NOWAIT);
                }
                uv_loop_close(loop_server);
                uv_loop_close(loop_test);


                delete async_server;
                async_server = NULL;
                delete async_test;
                async_test = NULL;

                delete loop_test;
                loop_test = NULL;
                delete loop_server;
                loop_server = NULL;
            }
        }
    }
}
