
#ifndef WSF_ACTOR_UNITTEST_TIMER_SETTIMER_HPP
#define WSF_ACTOR_UNITTEST_TIMER_SETTIMER_HPP


#include <wsf/actor.hpp>
namespace timer {
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
	* @api {get} /unittest/timer/settimer Timer test
	* @apiVersion 0.1.0
	* @apiName timer
	* @apiGroup unittest
	*
	* @apiDescription description
	*
	* @apiParam {json} timers json array to indicate timeout value
	* @apiParam {Number} [groups=1] numbers of the timers array
	*
	*
	* @apiExample Example usage:
	* curl -i http://localhost/unittest/timer/settimer?timers=[100,1000,345]
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
	class testSetTimer : public IOperation
	{
		RESTAPI("GET /timer/settimer");
	protected:
		void On(const request_t& req, shared_ptr< Response> res)
		{
			const string& timers = get(req.query(), "timers");
			int groups = get(req.query(), "groups", 1);
			if (timers.empty())
			{
				return;
			}
			auto j = json::parse(timers);

			jres_ = json::array();

			INFO("testSetTimer groups=" << groups << std::endl
				<< timers << std::endl);

			for (int i = 0; i <groups; i++)
			{
				for (json::iterator it = j.begin(); it != j.end(); ++it) {
					uint32_t timeout = *it;

					uint64_t base = current_millisecond();
					//INFO("testSetTimer : timeout =" << timeout);
					SetTimer(timeout, [=](bool is_canceled) {//OnTimer callback
						int actual = (int)(current_millisecond() - base);

						map<string, int> m{ { "expect", timeout },{ "actual",actual } };
						jres_.push_back(m);
						//INFO("testSetTimer : "<<jres_.size() << "   "<<j.size()*groups);
						if (jres_.size() == j.size()*groups)
						{
							res->content() = jres_.dump();
							res->Reply();
						}
					});
				}

			}
		}

		json jres_;
	};

}

#endif