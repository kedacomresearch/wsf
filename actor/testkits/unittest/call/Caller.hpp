
#ifndef WSF_ACTOR_UNITTEST_CALL_SERVICE_HPP
#define WSF_ACTOR_UNITTEST_CALL_SERVICE_HPP


#include <wsf/actor.hpp>
namespace call {
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
	* @api {get} /call-tester Timer test
	* @apiVersion 0.1.0
	* @apiName timer
	* @apiGroup unittest
	*
	* @apiDescription description
	*
	* @apiParam {String} echo_url echo service url (to be receive and reply )
	* @apiParam {Number} times    requests to be sent to echo service
	*
	*
	* @apiExample Example usage:
	* curl -i  -G --data-urlencode "times=10"\
	*      --data-urlencode "description=http://localhost/unittest/echo" \
	*      http://localhost/unittest/call-tester
	*
	* @apiSuccess {Object[]} result
	* @apiSuccess {Number}   result.expect  expect timeout which you set in request
	* @apiSuccess {Number}   result.actual  actual timeout value which you set in request
	*
	* @apiSuccessExample {json} Success-Response:
	*     HTTP/1.1 200 OK
	*     [
	*       {"expect":100, "actual": 80},
	*       {"expect":1000, "actual": 1000},
	*       {"expect":345, "actual": 300},
	*
	*     ]
	*/
	class testCallTester : public IOperation
	{
		RESTAPI("GET /call");
	protected:
		bool OnInitialize(IService* parent)
		{
			prefix_ = parent->prefix() + "/call";

		}
		void On(const request_t& req, shared_ptr< Response> res)
		{
			
//			int n = req.path().find_first_of(prefix_);

	//		string param(req.path(), n);

			
// 			const string& timers = get(req.query(), "timers");
// 			int groups = get(req.query(), "groups", 1);
// 			if (timers.empty())
// 			{
// 				return;
// 			}

		}

		json jres_;
		std::string prefix_;
	};

	class CallConcurrency : public IOperation
	{
	protected:

		void On(const request_t& req, shared_ptr< Response> res)
		{
			int n = get(req.query(), "times", 1);
			string url = get(req.query(), "echo-service");
			uint64_t base = current_millisecond();
			count_ = n;
			for (int i = 0; i < n; i++) {

				http::request_ptr r(new request_t);

				r->set_url(url);
				r->query() = req.query();
				r->content() = req.content();
				call(r, [=](const response_t&) {
					count_--;
					if (count_ == 0) {
						uint64_t consume = current_millisecond() - base;
						json j;
						j["microsecond-per-request"] = consume*1000 / n;
						res->content() = j.dump();
						res->Reply();
					}

				});

			}

		}
		int  count_;
	};

	class testCallConcurrencyGet : public CallConcurrency
	{
		RESTAPI("GET /call-concurrency");
	};
	class testCallConcurrencyPost : public CallConcurrency
	{
		RESTAPI("POST /call-concurrency");
	};
	class testCallConcurrencyPut : public CallConcurrency
	{
		RESTAPI("PUT /call-concurrency");
	};
	class testCallConcurrencyDelete : public CallConcurrency
	{
		RESTAPI("DELETE /call-concurrency");
	};

}

#endif