




#ifndef WSF_ACTOR_UNITTEST_CALLER_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_CALLER_SERVICE_HPP


#include "Caller.hpp"

namespace call {




	class Service : public wsf::actor::IService
	{
		RESTAPIs(
			testCallConcurrencyGet,
			testCallConcurrencyPost,
			testCallConcurrencyPut,
			testCallConcurrencyDelete
			);


	};
}

#endif