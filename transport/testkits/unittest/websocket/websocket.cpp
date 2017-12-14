#include "websocket.hpp"
#include <string>
#include <iostream>
#include <functional>
#include <algorithm>
#include <thread>
#include <mutex>
#include <wsf/transport/transport.hpp>
#include <wsf/util/logger.hpp>
#include <libuv/uv.h>
#include <chrono>

namespace
{
    using namespace ::wsf::transport::websocket;
    const std::string url = "ws://127.0.0.1:8082";
    const std::string url_test = "http://127.0.0.1:8083/transport/ws/test";

    // uv_loop_t *loop;
    // uv_async_t *async;
    // std::thread *t1;
    // unsigned int index = 0;
    // int client = 1;
    // bool stop = false;

    // void run()
    // {
    //     while (!stop)
    //     {
    //         uv_run(loop, UV_RUN_DEFAULT);
    //     }
    // }

    // void print_progress(uv_async_t *handle)
    // {
    //     if (handle->data != NULL)
    //     {
    //         std::string *msg = (static_cast<std::string *>(handle->data));
    //         std::cout << "get msg: " << msg->c_str() << std::endl;
    //         delete msg;
    //     }
    // }

    // uv_async_t close_async;
    // void stop_loop(uv_async_t *handle)
    // {
    //     uv_close((uv_handle_t *)handle, NULL);
    //     uv_stop(handle->loop);
    // }

    // void lws_uv_walk_cb(uv_handle_t *handle, void *arg)
    // {
    //     uv_close(handle, NULL);
    // }


    int count = 0;
    int target = 10;
    std::shared_ptr<::wsf::transport::http::Response> res = NULL;
    std::mutex mutex;
}
namespace wsf
{
    namespace transport
    {
        namespace websocket_test
        {
            bool Initialize()
            {
                //////////////////////////////////////////////////////////////////////////
                std::string msg("hello world!");

                std::string test_msg("start test!");

                auto bind_cb = [&, msg, test_msg](msg_status status, const std::string &peer, const std::string &data) {
                    if (status == OPEN)
                    {
                        return;
                    }
                    if (status == CLOSED)
                    {
                        std::cout << "peer closed: ";
                    }
                    std::cout << peer << "   " << data << std::endl;
                    if (data == test_msg)
                    {
                        std::list<std::string> urls;
                        urls.push_back(peer);
                        for (int i = 0; i < target; ++i)
                        {
                            PushMsg(urls, msg);
                        }
                    }
                    if (data == msg)
                    {
                        ++count;
                    }
                    if (count == target)
                    {
                        mutex.lock();
                        res->status_code() = 200;
                        res->reason() = "OK";
                        res->Reply();
                        res = NULL;
                        count = 0;
                        mutex.unlock();
                    }
                };
                auto connect_cb = [&, msg](msg_status status, const std::string &data) {
                    if (status == OPEN)
                    {
                        return;
                    }
                    if (status == CLOSED)
                    {
                        std::cout << "peer closed: ";
                    }
                    //std::cout << "get msg: " << data << std::endl;
                    if (data == msg)
                    {
                        ++count;
                    }
                    if (count == target)
                    {
                        mutex.lock();
                        res->status_code() = 200;
                        res->reason() = "OK";
                        res->Reply();
                        res = NULL;
                        count = 0;
                        mutex.unlock();
                    }
                };
                //////////////////////////////////////////////////////////////////////////
                //transport initialize


                //////////////////////////////////////////////////////////////////////////



                ::wsf::transport::http::Bind(url_test, [=](http::context_ptr &context, http::request_ptr &req) {

                    mutex.lock();
                    if (res != NULL)
                    {
                        std::shared_ptr<http::Response> res_default(new ::wsf::transport::http::Response(context));
                        res_default->status_code() = 501;
                        res_default->reason() = "Internal Server Error";
                        res_default->Reply();
                        std::cout << " The former test isn't finished!" << std::endl;
                        mutex.unlock();
                        return;
                    }
                    count = 0;
                    res = std::make_shared<::wsf::transport::http::Response>(context);
                    mutex.unlock();

                    std::string test_num = req->query()["test"];
                    //if (test_num.empty())
                    //{
                    //	std::shared_ptr<http::Response> res_default(new ::wsf::transport::http::Response(context));
                    //	res_default->status_code() = 501;
                    //	res_default->reason() = "Internal Server Error";
                    //	res_default->Reply();
                    //	std::cout << " Test parameter is error!" << std::endl;
                    //	return;
                    //}
                    std::cout << std::endl;
                    switch (std::stoi(test_num))
                    {
                        case 0:
                        {
                            if (!req->query()["target"].empty())
                            {
                                target = std::stoi(req->query()["target"]);
                            }
                            Bind(url, bind_cb);
                            Connect(url, connect_cb);

                            std::this_thread::sleep_for(std::chrono::seconds(1));

                            //#ifdef _MSC_VER
                            //Sleep(500);
                            //#else
                            //#endif
                            SendMsg(url, test_msg);
                        }
                            return;
                        case 1:
                        {
                            if (!req->query()["target"].empty())
                            {
                                target = std::stoi(req->query()["target"]);
                            }
                            Bind(url, bind_cb);
                            Connect(url, connect_cb);

                            std::this_thread::sleep_for(std::chrono::seconds(1));

                            for (int i = 0; i < target; ++i)
                            {
                                SendMsg(url, msg);
                            }
                        }
                            return;
                        case 2:
                        {
                            Bind(url, bind_cb);
                            Connect(url, connect_cb);

                            std::this_thread::sleep_for(std::chrono::microseconds(500));

                            UnBind(url);
                        }
                        break;
                        case 3:
                        {
                            Bind(url, bind_cb);
                            Connect(url, connect_cb);

                            std::this_thread::sleep_for(std::chrono::microseconds(500));

                            DisConnect(url);
                        }
                        break;
                        case 4:
                        {
                            UnBind(url);
                            Connect(url, connect_cb);
                        }
                        break;
                        case 5:
                        {
                            DisConnect(url);
                            UnBind(url);
                        }
                        break;
                        default:
                        {
                            std::shared_ptr<http::Response> res_default(new ::wsf::transport::http::Response(context));
                            res_default->status_code() = 501;
                            res_default->reason() = "Internal Server Error";
                            res_default->Reply();
                            std::cout << " Test parameter is error!" << std::endl;
                        }
                            return;
                    }

                    std::this_thread::sleep_for(std::chrono::microseconds(500));

                    mutex.lock();
                    res->status_code() = 200;
                    res->reason() = "OK";
                    res->Reply();
                    res = NULL;
                    count = 0;
                    mutex.unlock();

                });
                //Sleep(1000);
                //UnBind(url);


                return true;
            }
            void Terminate()
            {
                //////////////////////////////////////////////////////////////////////////
                ::wsf::transport::http::UnBind(url_test);
            }
        }
    }
}
