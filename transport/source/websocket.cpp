#include <wsf/transport/websocket.hpp>
#include <wsf/transport/transport.hpp>
#include <wsf/transport/websocket/IProxy.hpp>
#include <wsf/transport/websocket/IBroker.hpp>

namespace wsf
{
    namespace transport
    {
        namespace websocket
        {
            using namespace ::wsf::transport;
            LOGGER("wsf.transport.websocket");

            bool Bind(const std::string &url, broker_msg_fn callback)
            {
                ::wsf::transport::IBroker *ws_broker = IBroker::getInstance(websocket::Scheme());
                if (ws_broker != NULL)
                {
                    return ws_broker->Bind(url, callback);
                }
                else
                {
                    ERR(" Bind: Websocket broker instance is not started up! ");
                    return false;
                }
            }
            bool UnBind(const std::string &url)
            {
                ::wsf::transport::IBroker *ws_broker = IBroker::getInstance(websocket::Scheme());
                if (ws_broker != NULL)
                {
                    return ws_broker->UnBind(url);
                }
                else
                {
                    ERR(" UnBind: Websocket broker instance is not started up! ");
                    return false;
                }
            }

            bool Connect(const std::string &url, notify_fn callback)
            {
                ::wsf::transport::IProxy *ws_proxy = IProxy::getInstance(websocket::Scheme());
                if (ws_proxy != NULL)
                {
                    return ws_proxy->Connect(url, callback);
                }
                else
                {
                    ERR(" Connect: Websocket proxy instance is not started up! ");
                    return false;
                }
            }
            bool DisConnect(const std::string &url)
            {
                ::wsf::transport::IProxy *ws_proxy = IProxy::getInstance(websocket::Scheme());
                if (ws_proxy != NULL)
                {
                    return ws_proxy->DisConnect(url);
                }
                else
                {
                    ERR(" Connect: Websocket proxy instance is not started up! ");
                    return false;
                }
            }

            void PushMsg(const std::list<std::string> &peers, const std::string &data)
            {
                ::wsf::transport::IBroker *ws_broker = IBroker::getInstance(websocket::Scheme());
                if (ws_broker != NULL)
                {
                    ws_broker->PushMsg(peers, data);
                }
                else
                {
                    ERR(" PushMsg: Websocket broker instance is not started up! ");
                }
            }
            void SendMsg(const std::string &url, const std::string &data)
            {
                ::wsf::transport::IProxy *ws_proxy = IProxy::getInstance(websocket::Scheme());
                if (ws_proxy != NULL)
                {
                    return ws_proxy->SendMsg(url, data);
                }
                else
                {
                    ERR(" SendMsg: Websocket proxy instance is not started up! ");
                    return;
                }
            }
        }
    }
}