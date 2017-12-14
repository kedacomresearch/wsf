#ifndef WSF_ACTOR_UNITTEST_NOTIFY_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_NOTIFY_SERVICE_HPP


#include "Pub.hpp"
#include "Sub.hpp"

namespace notify
{
    class Service : public wsf::actor::IService
    {
        RESTAPIs(
            test1PublishTopic,
            test2PublishTopic,
            test3PublishTopic,
            test1SubscribeTopic,
            test2SubscribeTopic,
            test3SubscribeTopic);
    };
}

#endif