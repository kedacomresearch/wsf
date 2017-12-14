
#ifndef WSF_ACTOR_UNITTEST_ECHO_HPP
#define WSF_ACTOR_UNITTEST_ECHO_HPP


#include <wsf/actor.hpp>
namespace echo {
	using namespace wsf::actor;

	using std::string;
	using std::map;
	using std::vector;
	using std::shared_ptr;

	using nlohmann::json;
	using ::wsf::transport::http::request_t;
	using ::wsf::transport::http::response_t;
	using ::wsf::transport::http::Response;

	/**
	* @api {get} /echo
	* @apiVersion 0.1.0
	* @apiName Echo
	* @apiGroup unittest
	*
	* @apiDescription echo request content in response
	*
	*
	* @apiExample Example usage:
	* curl -i http://localhost/unittest/echo
	*
	*/
	class Get : public IOperation
	{
		RESTAPI("GET /echo");
	protected:
		void On(const request_t& req, shared_ptr< Response> res)
		{
			res->content() = req.content();
			res->Reply();
		}
	};
	/**
	* @api {post} /echo
	* @apiVersion 0.1.0
	* @apiName Echo
	* @apiGroup unittest
	*
	* @apiDescription echo request content in response
	*
	*
	* @apiExample Example usage:
	* curl -i http://localhost/unittest/echo
	*
	*/
	class Post : public IOperation
	{
		RESTAPI("POST /echo");
	protected:
		void On(const request_t& req, shared_ptr< Response> res)
		{
			res->content() = req.content();
			res->Reply();
		}
	};

	class Put : public IOperation
	{
		RESTAPI("PUT /echo");
	protected:
		void On(const request_t& req, shared_ptr< Response> res)
		{
			res->content() = req.content();
			res->Reply();
		}
	};

	class Delete : public IOperation
	{
		RESTAPI("DELETE /echo");
	protected:
		void On(const request_t& req, shared_ptr< Response> res)
		{
			res->content() = req.content();
			res->Reply();
		}
	};
}

#endif