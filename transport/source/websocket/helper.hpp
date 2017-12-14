#ifndef WSF_TRANSPORT_WEBSOCKET_HELPER_HPP
#define WSF_TRANSPORT_WEBSOCKET_HELPER_HPP

#include <libevent/evhttp.h>
#include <libwebsockets/libwebsockets.h>
#include <libuv/uv.h>
#include <list>
#include <algorithm>
#include <type_traits>
#include <wsf/util/uriparser.hpp>

#if defined(__GNUC__)
#define INVALID_SOCKET -1
#endif


namespace wsf
{
    namespace transport
    {
        namespace websocket
        {
            struct call_t
            {
                evutil_socket_t caller;
                evutil_socket_t callee;
                call_t()
                    : caller(INVALID_SOCKET)
                    , callee(INVALID_SOCKET)
                {
                }
            };

            class Commander
            {
            public:
                struct command_t
                {
                    command_t(uint8_t code_ = 0)
                        : code(code_)
                    {
                    }
                    uint8_t code;
                };

                Commander(uv_loop_t *loop)
                    : loop_(loop)
                    , cmd_(NULL)
                    , msg_(NULL)
                    , socket_cmd_(NULL)
                {
                }

                ~Commander()
                {
                }

                bool Initialize()
                {
                    evutil_socket_t evcmd_socket[2];
#ifdef _MSC_VER
                    if (0 != evutil_socketpair(AF_INET, SOCK_STREAM, 0, evcmd_socket))
#else
                    if (0 != evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, evcmd_socket))
#endif
                    {
                        return false;
                    }
                    socket_cmd_ = new call_t;
                    socket_cmd_->caller = evcmd_socket[0];
                    socket_cmd_->callee = evcmd_socket[1];

                    cmd_ = new uv_async_t;
                    msg_ = new uv_async_t;

                    return true;
                }

                void Terminate()
                {
                    if (socket_cmd_ != NULL)
                    {
                        if (socket_cmd_->callee != INVALID_SOCKET)
                        {
                            evutil_closesocket(socket_cmd_->callee);
                            socket_cmd_->callee = INVALID_SOCKET;
                        }
                        if (socket_cmd_->caller != INVALID_SOCKET)
                        {
                            evutil_closesocket(socket_cmd_->caller);
                            socket_cmd_->caller = INVALID_SOCKET;
                        }
                        delete socket_cmd_;
                        socket_cmd_ = NULL;
                    }

                    if (cmd_ != NULL)
                    {
                        //uv_close((uv_handle_t *)cmd_, NULL);
                        delete cmd_;
                        cmd_ = NULL;
                    }
                    if (msg_ != NULL)
                    {
                        //uv_close((uv_handle_t *)msg_, NULL);
                        delete msg_;
                        msg_ = NULL;
                    }
                }

                template <typename _Command>
                bool SendCommand(_Command &command)
                {
                    //uint8_t result = 0;
                    //send pointer
                    command_t *cmd = static_cast<command_t *>(&command);
                    int len = ::send(socket_cmd_->caller,
                                     static_cast<const char *>(static_cast<void *>(&cmd)),
                                     sizeof(cmd), 0);
                    if (len != sizeof(command_t *))
                    {
                        return false;
                    }
                    //inter-thread communication through libuv
                    uv_async_send(cmd_);
                    return true;
                }

                command_t *RecvCommand()
                {
                    command_t *cmd;
                    int len = ::recv(socket_cmd_->callee,
                                     static_cast<char *>(static_cast<void *>(&cmd)),
                                     sizeof(cmd), 0);
                    if (sizeof(command_t *) != len)
                    {
                        return NULL;
                    }
                    return cmd;
                }

                void ReplyCommand()
                {
                    uint8_t rsp_code = 0XFF;
                    ::send(socket_cmd_->callee,
                           static_cast<const char *>(static_cast<void *>(&rsp_code)),
                           sizeof(rsp_code), 0);
                }
                void WaitForReply()
                {
                    uint8_t result = 0;
                    ::recv(socket_cmd_->caller,
                           static_cast<char *>(static_cast<void *>(&result)),
                           sizeof(result), 0);
                }
                uv_async_t *GetCmd()
                {
                    return cmd_;
                }
                uv_async_t *GetMsg()
                {
                    return msg_;
                }

            private:
                uv_loop_t *loop_;
                uv_async_t *cmd_;
                uv_async_t *msg_;
                call_t *socket_cmd_;
            };
        }

        namespace websocket
        {
            inline bool ParseURI(const std::string &url,
                                      std::string &host,
                                      uint16_t &port,
                                      std::string &path,
                                      std::string &err)
            {
                if (url.empty())
                {
                    err = "The URL is empty.";
                    return false;
                }
                ::wsf::util::URIParser parser;

                if (!parser.Parse(url))
                {
                    err = "The URL can't be parsed.";
                    return false;
                }

                port = parser.port();
                if (port == 0)
                {
                    err = "The port is empty.";
                    return false;
                }

                host = parser.host();
                if (host.empty())
                {
                    err = "The host is empty.";
                    return false;
                }

                path = parser.path();
                if (path.empty())
                {
                    path = "/";
                }
                return true;
            }
        }
    }
}

#endif
