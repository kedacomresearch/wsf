
//DOCUMENT FOR REST API


/**
 * @api {post} /service/:name Startup Service
 * @apiVersion 0.1.0
 * @apiName StartupService
 * @apiGroup ServiceCenter
 * @apiPermission admin
 *
 * @apiDescription description
 *
 * @apiParam {String} name    The name of the service prototype
 * @apiParam {String} config  configuration for the service
 *
 * @apiExample Example usage:
 * curl -i http://localhost/service/calc
 *      -d { "prefix":"/calcuation"}
 *

 * @apiError NoAccessRight Only authenticated Admins can access the data.
 * @apiError NotFound      The <code>service</code> of the User was not found.
 *
 */

 /**
 * @api {delete} /service/:name Stop Service
 * @apiVersion 0.1.0
 * @apiName StopService
 * @apiGroup ServiceCenter
 * @apiPermission admin
 *
 * @apiDescription description
 *
 * @apiParam {String} name    The name of the service prototype
 *
 * @apiExample Example usage:
 * curl -X DELETE -i "http://localhost/service/calc"
 *
 *
 * @apiError NoAccessRight Only authenticated Admins can access the data.
 * @apiError NotFound      The <code>service</code> of the User was not found.
 *
 * @apiErrorExample Response (example):
 *     HTTP/1.1 404 Not Found
 */
 
 

#ifndef WEB_BASED_SERVICE_FRAMEWORK_ACTOR_MODEL_HPP
#define WEB_BASED_SERVICE_FRAMEWORK_ACTOR_MODEL_HPP

#include "actor/Actor.hpp"
#include "actor/Service.hpp"
#include "actor/Operation.hpp"
#include "actor/inner/ServiceCenter.hpp"
#include "actor/inner/main.inl"
#include "util/string.hpp"
#include "util/json.hpp"
#include "util/status_code.hpp"
#include "util/rest.hpp"
#include "util/map.hpp"
#include "util/uriparser.hpp"

namespace wsf {
	namespace actor {
		
		/************************************************************************
		 * config is a json format string
		  {
				 "service-center":{
					 "port":80,
					 "prefix":"/",
				 },

				 "services" : [
				 { 
					 "prototype":"calc", #MUST regist with 
					 "port":80, #default 80
					 "prefix":"/calc", #default same as prototype

				 }]

		  }
		 ***********************************************************************/
		bool Initialize(const std::string& config="");


		template< class _Service >
		bool CreateServicePrototype(const std::string& name);
		void Terminate();

		/*
		  Init example

		  assert( Initialize( "your config") );
		  assert( CreateServicePrototype< YourService1 >() );
		  assert( CreateServicePrototype< YourService2 >() );
		  ....
		  run your code
		  ....

		  Termiate();
		*/
	}
}


#include <sys/timeb.h>
static inline uint64_t current_millisecond()
{
	timeb t;
	ftime(&t);
	return t.time*1000 + t.millitm;
}


#define ASSERT assert
#endif//!WEB_BASED_SERVICE_FRAMEWORK_ACTOR_MODEL_HPP