
#ifndef _WSF_ACTOR_INNER_MAIN_INLINE_H_
#define _WSF_ACTOR_INNER_MAIN_INLINE_H_

#include "ServiceCenter.hpp"

namespace wsf {
	namespace actor {
		template< class _Service >
		inline bool CreateServicePrototype(const std::string& name)
		{
			//ServiceCenter& sc = ServiceCenter::instance();
			ServiceCenter::RegistService<_Service>(name);
			return true;

		}

	}
}

#endif