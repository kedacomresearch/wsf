#include <wsf/transport/http.hpp>
#include <wsf/transport/transport.hpp>
#include <wsf/transport/http/IProxy.hpp>
#include <wsf/transport/http/IService.hpp>


namespace wsf
{
    namespace transport
    {
        namespace http
        {
            using namespace ::wsf::transport;
            LOGGER("wsf.transport.http");

            void ResponseToRemote(context_ptr &context, response_ptr &res)
            {
                ::wsf::transport::IService *http_service = IService::getInstance(http::Scheme());
                if (http_service != NULL)
                {
                    http_service->Reply(context, res);
                }
                else
                {
                    ERR(" ResponseToRemote: Http service instance is not started up! ");
                }
            }
            void RequestToRemote(context_ptr &context, request_ptr &req, response_fn fn)
            {
                ::wsf::transport::IProxy *http_proxy = IProxy::getInstance(http::Scheme());
                if (http_proxy != NULL)
                {
                    http_proxy->Request(context, req, fn);
                }
                else
                {
                    ERR(" RequestToRemote: Http proxy instance is not started up! ");
                }
            }
            bool Bind(const std::string &url_base, request_fn fn)
            {
                ::wsf::transport::IService *http_service = IService::getInstance(http::Scheme());
                if (http_service != NULL)
                {
                    return http_service->Bind(url_base, fn);
                }
                else
                {
                    ERR(" Bind: Http service instance is not started up! ");
                    return false;
                }
            }
            bool UnBind(const std::string &url_base)
            {
                ::wsf::transport::IService *http_service = IService::getInstance(http::Scheme());
                if (http_service != NULL)
                {
                    return http_service->UnBind(url_base);
                }
                else
                {
                    ERR(" UnBind: Http service instance is not started up! ");
                    return false;
                }
            }
        }
    }
}
