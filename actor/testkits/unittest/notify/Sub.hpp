#ifndef WSF_ACTOR_UNITTEST_SUB_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_SUB_SERVICE_HPP

#include <wsf/actor.hpp>

namespace notify
{
    using namespace wsf::actor;
    using nlohmann::json;
    using ::wsf::transport::http::request_t;
    using ::wsf::transport::http::response_t;
    using ::wsf::transport::http::Response;
    //static int num = 0;

    class testSubscribeTopic : public IOperation
    {

    protected:
        virtual std::string RestApi() = 0;
        virtual void NotifyDone(int num) = 0;


        virtual bool OnInitialize(IService *s)
        {
            parent_ = s;
            return true;
        }
        void On(const request_t &req, std::shared_ptr<Response> res)
        {
            //notify msg
            if (has(req.header(), "CSeq"))
            {
                DEBUG(this->name() << " Receive notification: \"" << req.content()
                                   << "\" From topic: <" << get(req.header(), "Topic") << ">");
                NotifyDone(++num_);
                return;
            }

            const std::string &topic_url = get(req.query(), "topic");
            const std::string &op = get(req.query(), "operation");
            std::string this_restapi(RestApi());

            ServiceCenter &sc = ServiceCenter::instance();
            bool rc;
            if (op == "subscribe")
            {
                rc = sc.Subscribe(topic_url, this_restapi, parent_->prefix());
            }
            else if (op == "unsubscribe")
            {
                rc = sc.UnSubscribe(topic_url, this_restapi, parent_->prefix());
            }
            else
            {
                res->set_status_code(::wsf::http::status_code::Bad_Request);
                res->Reply();
                return;
            }
            if (rc)
            {
                res->set_status_code(::wsf::http::status_code::OK);
            }
            else
            {
                res->set_status_code(::wsf::http::status_code::Bad_Request);
            }
            res->Reply();
        }

    private:
        IService *parent_;
        static int num_;
    };
    int testSubscribeTopic::num_ = 0;
    class test1SubscribeTopic : public testSubscribeTopic
    {
        RESTAPI("PUT /topic/subscribe/test1");

        virtual std::string RestApi()
        {
            return restapi();
        }
        virtual void NotifyDone(int num)
        {
            if (num == 40)
            {
                INFO(" Receiving notification OK!");
            }
        }
    };
    class test2SubscribeTopic : public testSubscribeTopic
    {
        RESTAPI("PUT /topic/subscribe/test2");

        virtual std::string RestApi()
        {
            return restapi();
        }
        virtual void NotifyDone(int num)
        {
            if (num == 40)
            {
                INFO(" Receiving notification OK!");
            }
        }
    };
    class test3SubscribeTopic : public testSubscribeTopic
    {
        RESTAPI("PUT /topic/subscribe/test3");

        virtual std::string RestApi()
        {
            return restapi();
        }
        virtual void NotifyDone(int num)
        {
            if (num == 40)
            {
                INFO(" \n\n-----------Receiving notification OK!-------------\n");
            }
        }
    };
}
#endif