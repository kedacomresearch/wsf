
/**
 * @api {get} /calc/add Addtion
 * @apiVersion 0.1.0
 * @apiName add
 * @apiGroup calc
 *
 * @apiDescription description
 *
 * @apiParam {Number} a
 * @apiParam {Number} b
 *
 * @apiExample Example usage:
 * curl  -i "http://127.0.0.1:8080/calc/add?a=1&b=2"
 *
 * @apiSuccess {String} result Result value of  a + b
 * @apiSuccessExample {String} Success-Response:
 *    {
 *       "result": "3"
 *    }
 */
 
 /**
 * @api {get} /calc/square/:num Square
 * @apiVersion 0.1.0
 * @apiName power
 * @apiGroup calc
 *
 * @apiDescription description
 *
 * @apiParam {Number} num
 *
 * @apiExample Example usage:
 * curl -i http://localhost/calc/square/10
 *
 * @apiSuccess {Number} result of 10^2
 * 
 */
 
 
 
 /**
 * @api {get} /calc/sum
 * @apiVersion 0.1.0
 * @apiName timer
 * @apiGroup calc
 *
 * @apiDescription description
 *
 * @apiParam {Number[]} numbers array of numbers
 *
 * @apiExample Example usage:
 * curl -i http://localhost/calc/sumber?numbers=[1000,1]
 *
 * @apiSuccess {Number} response after 1000ms
 */
 
#ifndef _CALC_HPP_
#define _CALC_HPP_

#include <wsf/actor.hpp>
using namespace std;
using nlohmann::json;
class Add : public wsf::actor::IOperation{
	RESTAPI("GET /calc/add");

	void On(const ::wsf::transport::http::request_t& req,
		std::shared_ptr< ::wsf::transport::http::Response> res)
	{
		const std::map<std::string, std::string>& query = req.query();
		int a = get(query, "a", 0 );
		int b = get(query, "b", 0 );
		json j;
		j["result"] = a + b;
		res->content() = j.dump();
		res->header()["Content-type"] = "application/json";
		res->Reply();
	}
};

class Square : public wsf::actor::IOperation {
	RESTAPI("GET /calc/square/$num");

	void On(const ::wsf::transport::http::request_t& req,
		std::shared_ptr< ::wsf::transport::http::Response> res)
	{
		const string& num = ::get(req, "path.num");
		if (num.empty())
		{
			res->set_status_code(wsf::http::status_code::Bad_Request);
			res->content() = "{\"reason\":\"num is NULL\"}";
			res->Reply();
			return;
		}
		int n = std::stoi(num);
// 		int num = get(this->param(), "num", 0);
// 		
 		json j;
 		j["result"] = n*n;
 		res->content() = j.dump();
 		res->Reply();
	}
};
// 
// class Sum : public wsf::actor::IOperation {
// 	RESTAPI("GET /calc/sum");
// 
// 	void OnAddResult(const ::wsf::transport::http::response_t& res)
// 	{
// 		auto j = json::parse(res.content());
// 		int value = j["result"];
// 		result_ += value;
// 		
// 		if (!this->nums_.empty())
// 		{
// 			::wsf::transport::http::request_ptr r(new ::wsf::transport::http::request_t);
// 			r->query()["a"] = nums_.back();
// 			r->query()["b"] = result_;
// 			r->host() = "localhost";
// 			r->port() = 80;
// 			r->path() = "/calc/add";
// 			nums_.pop_back();
// 			call(r, [this](auto response) {
// 				this->OnAddResult(response);
// 			});
// 			return;
// 		}
// 		else
// 		{
// 
// 		}
// 	}
// 
// 	void On(const ::wsf::transport::http::request_t& req,
// 		std::shared_ptr< ::wsf::transport::http::Response> res)
// 	{
// 		
// 		auto j = json::parse(req.content());
// 		for (json::iterator it = j.begin(); it != j.end(); ++it) {			
// 			nums_.push_back(*it);
// 		}
// 		result_ = 0;
// 		if (nums_.size() == 1)
// 		{
// 			result_ = nums_[0];
// 		}
// 
// 		if (nums_.size() > 1)
// 		{
// 			::wsf::transport::http::request_ptr r(new ::wsf::transport::http::request_t);
// 			r->query()["a"] = nums_.back(); nums_.pop_back();
// 			r->query()["b"] = nums_.back(); nums_.pop_back();
// 			r->host() = "localhost";
// 			r->port() = 80;
// 			r->path() = "/calc/add";
// 
// 			call(r, [this](auto response) {
// 				this->OnAddResult(response);
// 			});
// 
// 			res_ = res;
// 		}
// 		else
// 		{
// 			res->Reply();
// 		}
// 	}
// 
// protected:
// 	std::vector<int> nums_;
// 	int result_;
// 	std::shared_ptr< ::wsf::transport::http::Response> res_;
// };


class Calc : public wsf::actor::IService
{
	RESTAPIs( Add, Square);
//public:
// 	::wsf::actor::IOperation* restapi_match(const wsf::actor::http::message_t& msg)
// 	{
// 		this->restapi_match_<Add,Power>(msg);
// 	}

};

#endif