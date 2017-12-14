#ifndef WSF_TRANSPORT_HTTP_HELPER_HPP
#define WSF_TRANSPORT_HTTP_HELPER_HPP

#include <libevent/evhttp.h>
#include <list>
#include <algorithm>
#include <type_traits>
#include <wsf/util/uriparser.hpp>

#if defined(__GNUC__)
#define INVALID_SOCKET -1
#endif

#ifndef WSF_TAILQ_FIRST
#define WSF_TAILQ_FIRST(head) ((head)->tqh_first)
#endif
#ifndef WSF_TAILQ_END
#define WSF_TAILQ_END(head) NULL
#endif
#ifndef WSF_TAILQ_NEXT
#define WSF_TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
#endif

#ifndef WSF_TAILQ_FOREACH
#define WSF_TAILQ_FOREACH(var, head, field) \
    for ((var) = WSF_TAILQ_FIRST(head);     \
         (var) != WSF_TAILQ_END(head);      \
         (var) = WSF_TAILQ_NEXT(var, field))
#endif

namespace wsf
{
    namespace transport
    {
        namespace http
        {
            struct call_t
            {
                evutil_socket_t caller;
                evutil_socket_t callee;
                struct bufferevent *buf_event;
                call_t()
                    : caller(INVALID_SOCKET)
                    , callee(INVALID_SOCKET)
                    , buf_event(NULL)
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

                Commander()
                    : cmd_(NULL), msg_(NULL)
                {
                }

                ~Commander()
                {
                }

                bool Initialize()
                {
                    evutil_socket_t evcmd_socket[2];
                    evutil_socket_t evmsg_socket[2];
#ifdef _MSC_VER
                    if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, evcmd_socket) != 0 ||
                        evutil_socketpair(AF_INET, SOCK_STREAM, 0, evmsg_socket) != 0)
#else
                    if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, evcmd_socket) != 0 ||
                        evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, evmsg_socket) != 0)
#endif
                    {
                        return false;
                    }
                    cmd_ = new call_t;
                    msg_ = new call_t;
                    cmd_->caller = evcmd_socket[0];
                    cmd_->callee = evcmd_socket[1];
                    msg_->caller = evmsg_socket[0];
                    msg_->callee = evmsg_socket[1];

                    return true;
                }

                void Terminate()
                {
                    if (cmd_ != NULL)
                    {
                        if (cmd_->callee != INVALID_SOCKET)
                        {
                            evutil_closesocket(cmd_->callee);
                            cmd_->callee = INVALID_SOCKET;
                        }
                        if (cmd_->buf_event)
                        {
                            bufferevent_free(cmd_->buf_event);
                            cmd_->buf_event = NULL;
                        }
                        if (cmd_->caller != INVALID_SOCKET)
                        {
                            evutil_closesocket(cmd_->caller);
                            cmd_->caller = INVALID_SOCKET;
                        }
                        delete cmd_;
                        cmd_ = NULL;
                    }

                    if (msg_ != NULL)
                    {
                        if (msg_->callee != INVALID_SOCKET)
                        {
                            evutil_closesocket(msg_->callee);
                            msg_->callee = INVALID_SOCKET;
                        }
                        if (msg_->buf_event)
                        {
                            bufferevent_free(msg_->buf_event);
                            msg_->buf_event = NULL;
                        }
                        if (msg_->caller != INVALID_SOCKET)
                        {
                            evutil_closesocket(msg_->caller);
                            msg_->caller = INVALID_SOCKET;
                        }
                        delete msg_;
                        msg_ = NULL;
                    }
                }

                template <typename _Command>
                bool SendCommand(_Command &command)
                {
                    uint8_t result = 0;
                    command_t *cmd = static_cast<command_t *>(&command);
                    int len = ::send(cmd_->caller,
                                     static_cast<const char *>(static_cast<void *>(&cmd)),
                                     sizeof(cmd), 0);
                    if (len != sizeof(command_t *))
                    {
                        return false;
                    }
                    ::recv(cmd_->caller,
                           static_cast<char *>(static_cast<void *>(&result)),
                           sizeof(result), 0);
                    return true;
                }

                command_t *RecvCommand(struct bufferevent *bev)
                {
                    command_t *cmd;
                    if (sizeof(command_t *) != bufferevent_read(bev, &cmd, sizeof(command_t *)))
                        return NULL;
                    return cmd;
                }

                void ReplyCommand(struct bufferevent *bev)
                {
                    uint8_t rsp_code = 0XFF;
                    bufferevent_write(bev, &rsp_code, 1);
                }

                bool Trigger(uint8_t event = 0xFF)
                {
                    int len = ::send(msg_->caller,
                                     static_cast<const char *>(static_cast<void *>(&event)),
                                     sizeof(event), 0);
                    return len == sizeof(event) ? true : false;
                }

                void ThrowAllEvents(struct bufferevent *bev)
                {
                    uint8_t event;
                    bufferevent_read(bev, &event, sizeof(event));
                }
                call_t *command_socket()
                {
                    return cmd_;
                }
                call_t *message_socket()
                {
                    return msg_;
                }

            protected:
                call_t *cmd_;
                call_t *msg_;
                // struct event_base *base_;
            };
        }

        namespace http
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
                    port = 80;
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
