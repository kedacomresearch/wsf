#include <wsf/transport/transport.hpp>
#include <wsf/transport/http/IProxy.hpp>
#include <wsf/transport/http/IService.hpp>
#include <wsf/transport/websocket/IBroker.hpp>
#include <wsf/transport/websocket/IProxy.hpp>
#ifdef _WIN32
#include <Windows.h>
#endif
namespace wsf
{
    namespace transport
    {
        LOGGER("wsf.transport");
        namespace
        {
            std::string role_map[] = {"Service", "Proxy", "Broker"};

            ITransport *transports_service_[256] = {0};
            ITransport *transports_proxy_[256] = {0};
            ITransport *transports_broker_[256] = {0};

            std::string error_;
        }

        static const int MAXSIZE_INSTANTIATOR = 255;
        static IInstantiator *instantiators[MAXSIZE_INSTANTIATOR] = {0};
        static std::list<ITransport *> transport_list_;
        bool IInstantiator::regist()
        {
            for (int i = 0; i < MAXSIZE_INSTANTIATOR; ++i)
            {
                if (instantiators[i] == NULL)
                {
                    instantiators[i] = this;
                    return true;
                }
            }
            return false;
        }
        bool is_init = false;
        bool is_term = false;
        std::mutex mutex;
    }

    /******************************************************************************/
    /*  Global interfaces.                                                        */
    /******************************************************************************/
    namespace transport
    {
        bool Initialize(const std::string &config)
        {
			{
				using namespace log4cplus;
				Logger root = Logger::getInstance("");
				root.setLogLevel(INFO_LOG_LEVEL);

				SharedAppenderPtr consoleAppender(new ConsoleAppender());
				std::auto_ptr<Layout> simpleLayout(new SimpleLayout());
				consoleAppender->setLayout(simpleLayout);
				root.addAppender(consoleAppender);
			}
            mutex.lock();
            if (is_init)
            {
                mutex.unlock();
                return true;
            }
            mutex.unlock();
            ::wsf::transport::regist_transport< ::wsf::transport::http::IProxy>();
            ::wsf::transport::regist_transport< ::wsf::transport::http::IService>();
            ::wsf::transport::regist_transport< ::wsf::transport::websocket::IProxy>();
            ::wsf::transport::regist_transport< ::wsf::transport::websocket::IBroker>();

#ifdef _WIN32
            WORD wVersionRequested;
            WSADATA wsaData;

            wVersionRequested = MAKEWORD(2, 2);
            (void)WSAStartup(wVersionRequested, &wsaData);
#endif
            for (int i = 0; i < MAXSIZE_INSTANTIATOR; ++i)
            {
                IInstantiator *instantiator = instantiators[i];
                if (instantiator != NULL)
                {
                    ITransport *t = instantiator->instantiate();
                    if (t == NULL || !t->Initialize())
                    {
                        FATAL("Failed to initialize transport instance");
                        return false;
                    }
                    transport_list_.push_back(t);
                }
            }
            mutex.lock();
            is_init = true;
            mutex.unlock();
            return true;
        }
        void Terminate()
        {
            mutex.lock();
            if (is_term)
            {
                ERR("The ::wsf::transport::Terminate() is invoked again!");
                mutex.unlock();
                return;
            }
            mutex.unlock();
#ifdef _WIN32
            WSACleanup();
#endif
            for (std::list<ITransport *>::iterator it = transport_list_.begin();
                 it != transport_list_.end(); ++it)
            {
                if (*it)
                {
                    (*it)->Terminate();
                    delete (*it);
                }
            }

            for (int i = 0; i < MAXSIZE_INSTANTIATOR; ++i)
            {
                if (instantiators[i] != NULL)
                {
                    delete instantiators[i];
                    instantiators[i] = NULL;
                }
            }
            mutex.lock();
            is_term = true;
            mutex.unlock();
        }
    }

    // interface
    namespace transport
    {
        /******************************************************************************/
        /*  ITransport interfaces.                                                    */
        /******************************************************************************/
        ITransport::ITransport(const std::string &schema, role_t role)
            : schema_(schema), name_(role_map[role] + "." + schema), id_(0)
        {
            ITransport *(*transports_)[256];
            switch (role)
            {
                case role_t::ISERVICE:
                {
                    transports_ = &transports_service_;
                    break;
                }
                case role_t::IPROXY:
                {
                    transports_ = &transports_proxy_;
                    break;
                }
                case role_t::IBROKER:
                {
                    transports_ = &transports_broker_;
                    break;
                }
                default:
                    error_ = std::string("Transport: Incorrect registration type!");
                    return;
            }

            for (unsigned int i = 1; i < sizeof(*transports_) / sizeof(*transports_[0]); ++i)
            {
                if ((*transports_)[i] == NULL)
                {
                    (*transports_)[i] = this;
                    this->id_ = (uint8_t)i;

                    logger_ = ::log4cplus::Logger::getInstance(std::string("wsf.transport.") + this->name());

                    return;
                }
                else
                {
                    if ((*transports_)[i]->name() == name_)
                    {
                        error_ = std::string("It is already registered!");
                        return;
                    }
                }
                error_ = std::string("The registered protocols have reached an upper limit!");
            }
        }
        ITransport::~ITransport()
        {
			::log4cplus::threadCleanup();
        }
        /******************************************************************************/
        /*  IService IProxy IBroker                                                   */
        /******************************************************************************/
        IService *IService::getInstance(const std::string &name)
        {

            for (unsigned int i = 1; i < sizeof(transports_service_) / sizeof(transports_service_[0]); ++i)
            {
                if (transports_service_[i] != NULL && transports_service_[i]->name() == ("Service." + name))
                {
                    return static_cast<IService *>(transports_service_[i]);
                }
            }
            return NULL;
        }

        //////////////////////////////////////////////////////////////////////////
        IProxy *IProxy::getInstance(const std::string &name)
        {
            for (unsigned int i = 1; i < sizeof(transports_proxy_) / sizeof(transports_proxy_[0]); ++i)
            {
                if (transports_proxy_[i] != NULL && transports_proxy_[i]->name() == ("Proxy." + name))
                {
                    return static_cast<IProxy *>(transports_proxy_[i]);
                }
            }
            return NULL;
        }

        //////////////////////////////////////////////////////////////////////////
        IBroker *IBroker::getInstance(const std::string &name)
        {
            for (unsigned int i = 1; i < sizeof(transports_broker_) / sizeof(transports_broker_[0]); ++i)
            {
                if (transports_broker_[i] != NULL && transports_broker_[i]->name() == ("Broker." + name))
                {
                    return static_cast<IBroker *>(transports_broker_[i]);
                }
            }
            return NULL;
        }
    }
}
