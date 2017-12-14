




#ifndef WSF_ACTOR_UNITTEST_ECHO_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_ECHO_SERVICE_HPP


#include "Echo.hpp"

namespace echo {




	class Service : public wsf::actor::IService
	{
		RESTAPIs(Get,Post,Put,Delete);


	};
}

#endif