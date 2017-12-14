

#ifndef WSF_ACTOR_UNITTEST_TIMER_KILLTIMER_HPP
#define WSF_ACTOR_UNITTEST_TIMER_KILLTIMER_HPP


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
	* @api {get} /unittest/timer/settimer SetTimer test
	* @apiVersion 0.1.0
	* @apiName SetTimer
	* @apiGroup unittest
	*
	* @apiDescription description
	*
	* @apiParam {json}   timers json array to indicate timeout value
	* @apiParam {Number} timers.kill_time the time to kill it
	*
	*
	* @apiExample Example usage:
	* curl -i http://localhost/unittest/timer/settimer?timers=[200,300,400,500]&kill_time=350
	*
	* @apiSuccess {Object[]} result json array for the unkilled timeout value
	*
	* @apiSuccessExample {json} Success-Response:
	*     HTTP/1.1 200 OK
	*     [
	*       400,500
	*
	*     ]
	*/
	class testKillTimer : public IOperation
	{
		RESTAPI("GET /timer/killtimer");
	protected:
		void On(const request_t& req, shared_ptr< Response> res)
		{
			const string& timers = get(req.query(), "timers");
			int groups = get(req.query(), "groups", 1);
			if (timers.empty())
			{
				res->content() = "[]";//no operation
				res->Reply();
				return;
			}
			uint32_t kill_time = get(req.query(), "kill_time", 0);
			assert(kill_time);

			auto j = json::parse(timers);

			jres_ = json::array();
			uint32_t maxtime = 0;
			for (json::iterator it = j.begin(); it != j.end(); ++it) {
				uint32_t timeout = *it;

				uint64_t base = current_millisecond();
				maxtime = std::max(timeout, maxtime);

				uint32_t id = SetTimer(timeout, [=](bool canceled) {
					if (canceled) return;

					jres_.push_back(timeout); //record the timeout one
				});

				timer_ids_.push_back(id);
			}

			SetTimer(kill_time, [=](bool canceled) {
				for (auto id : timer_ids_) {
					CancelTimer(id);
				}
			});

			INFO("testSetTimer kill_time=" << kill_time 
				<< ", wait time="<<maxtime <<std::endl
				<< timers << std::endl);
			
			SetTimer(maxtime + 100, [=](bool canceled) {
				res->content() = jres_.dump();
				res->Reply();
			});

		}

		json jres_;
		vector<uint32_t> timer_ids_;
	};

}

#endif
