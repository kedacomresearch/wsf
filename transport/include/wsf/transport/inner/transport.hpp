#ifndef WSF_TRANSPORT_INNER_TRANSPORT_HPP
#define WSF_TRANSPORT_INNER_TRANSPORT_HPP

#include <wsf/transport/transport.hpp>
#include <wsf/util/logger.hpp>

/*	brief introduction
	This header file defines the transport layer data struct
	and the relevant global interfaces.
*/

namespace wsf
{
    namespace transport
    {
        enum role_t
        {
            ISERVICE,
            IPROXY,
            IBROKER
        };



        class ITransport
        {
        public:
            virtual bool Initialize() = 0;
            virtual void Terminate() = 0;

            const std::string &schema() const
            {
                return schema_;
            }
            const std::string &name() const
            {
                return name_;
            }
            uint8_t Id() const
            {
                return id_;
            }
            virtual ~ITransport();

        protected:
            ITransport(const std::string &schema, role_t role);
			virtual ::log4cplus::Logger &Logger_()
			{
				return logger_;
			}

        private:
            ::log4cplus::Logger logger_;
            const std::string schema_;// schema in Uri
            const std::string name_;
            uint8_t id_;
        };
        class IService : public ITransport
        {
        protected:
            IService(const std::string &schema)
                : ITransport(schema, ISERVICE)
            {
            }

        public:
            virtual ~IService()
            {
            }

            static IService *getInstance(const std::string &name);

            virtual bool Bind(const std::string &url_base, http::request_fn callback) = 0;

            virtual bool UnBind(const std::string &url_base) = 0;

            virtual bool Reply(http::context_ptr &context, http::response_ptr &res) = 0;
        };

        class IProxy : public ITransport
        {
        protected:
            IProxy(const std::string &schema)
                : ITransport(schema, IPROXY)
            {
            }

        public:
            virtual ~IProxy()
            {
            }

            static IProxy *getInstance(const std::string &name);

            virtual bool Request(http::context_ptr &context,
                                 http::request_ptr &req,
                                 http::response_fn callback) = 0;

            //websocket use
            virtual bool Connect(const std::string &url, websocket::notify_fn callback) = 0;
            virtual bool DisConnect(const std::string &url) = 0;
            virtual void SendMsg(const std::string &url, const std::string &data) = 0;
        };

        class IBroker : public ITransport
        {
        protected:
            IBroker(const std::string &schema)
                : ITransport(schema, IBROKER)
            {
            }

        public:
            virtual ~IBroker()
            {
            }

            static IBroker *getInstance(const std::string &name);

            virtual bool Bind(const std::string &url, websocket::broker_msg_fn callback) = 0;

            virtual bool UnBind(const std::string &url) = 0;

            virtual bool PushMsg(const std::list<std::string> &urls, const std::string &data) = 0;
        };
    }
}
//Transport Instantiator
namespace wsf
{
    namespace transport
    {

        class IInstantiator
        {
        public:
            IInstantiator()
            {
            }
            virtual ~IInstantiator()
            {
            }
            virtual ITransport *instantiate() = 0;


            bool regist();

        protected:
            template <class _Transport>
            _Transport *instantiate()
            {
                _Transport *t = new _Transport;
                return t;
            }
        };


        template <class _Transport>
        class Instantiator : IInstantiator
        {
        };//base
    }
}


/**
* \brief Macro for declare transport inherited classes
*
* \param _Class the name of the class (include namespace)
* \param
*/
#define WSF_TRANSPORT_DECLARE(_Class)                                              \
    namespace wsf                                                                  \
    {                                                                              \
        namespace transport                                                        \
        {                                                                          \
            template <>                                                            \
            class Instantiator<_Class> : public ::wsf::transport::IInstantiator    \
            {                                                                      \
            public:                                                                \
                Instantiator()                                                     \
                {                                                                  \
                }                                                                  \
                ITransport *instantiate()                                          \
                {                                                                  \
                    return ::wsf::transport::IInstantiator::instantiate<_Class>(); \
                }                                                                  \
            };                                                                     \
        }                                                                          \
    }




namespace wsf
{
    namespace transport
    {
        /**
        * \brief 
		*  After declaring the transport classes using the macro `WSF_TRANSPORT_DECLARE`,
		*  user should regist the classes and then it will start when initializing the transport module.
        *
        */
        template <class _Class>
        bool regist_transport()
        {
            ::wsf::transport::IInstantiator *instantiator = new ::wsf::transport::Instantiator<_Class>();
            if (instantiator != NULL)
            {
                if (instantiator->regist())
                {
                    return true;
                }
                delete instantiator;
            }
            return false;
        }
    }
}

#endif
