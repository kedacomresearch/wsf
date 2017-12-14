




#ifndef WSF_ACTOR_UNITTEST_TIMER_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_TIMER_SERVICE_HPP


#include "SetTimer.hpp"
#include "KillTimer.hpp"

namespace timer {




	class Service : public wsf::actor::IService
	{
		RESTAPIs(testSetTimer
		,testKillTimer
		);


	};
}

#endif