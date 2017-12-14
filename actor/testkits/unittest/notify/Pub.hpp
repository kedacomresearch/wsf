#ifndef WSF_ACTOR_UNITTEST_PUB_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_PUB_SERVICE_HPP

#include <wsf/actor.hpp>

namespace notify
{
    using namespace wsf::actor;
    using nlohmann::json;
    using ::wsf::transport::http::request_t;
    using ::wsf::transport::http::response_t;
    using ::wsf::transport::http::Response;

    class testPublishTopic : public IOperation
    {
    protected:
        void On(const request_t &req, std::shared_ptr<Response> res)
        {
            const std::string &topic_url = get(req.query(), "topic");
            const std::string &op = get(req.query(), "operation");

            ServiceCenter &sc = ServiceCenter::instance();
            bool rc;
            if (op == "publish")
            {
                rc = sc.Publish(topic_url);
            }
            else if (op == "unpublish")
            {
                rc = sc.UnPublish(topic_url);
            }
            else if (op == "push msg")
            {
                const std::string &msg = req.content();
                const std::string &times = get(req.query(), "times");
                int num = std::stoi(times);
                for (int i = 0; i < num; ++i)
                {
                    sc.PushMsg(topic_url, msg);
                }
                rc = true;
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
    };

    class test1PublishTopic : public testPublishTopic
    {
        RESTAPI("PUT /topic/publish/test1");
    };
    class test2PublishTopic : public testPublishTopic
    {
        RESTAPI("PUT /topic/publish/test2");
    };
    class test3PublishTopic : public testPublishTopic
    {
        RESTAPI("PUT /topic/publish/test3");
    };
}
#endif