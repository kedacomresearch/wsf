#ifndef WSF_GSTREAMER_SERVICE_HPP
#define WSF_GSTREAMER_SERVICE_HPP

#include <wsf/gstreamer/inner/HttpLayer.hpp>
#include <wsf/util/logger.hpp>
#include <string>

namespace wsf
{
	namespace gstreamer
	{
		class IService : public HttpLayer
		{
		private:
			enum class Status { IDLE = 0, RUNNING };

		protected:
			IService();
			virtual ~IService() {
				::log4cplus::threadCleanup();
			}

		protected:
			void RestApiInitialize_();
			virtual bool RestApiMatch(::wthttp::request_ptr &req);

		private:
			virtual void RestApiInitialize() = 0;
		private:
			virtual void RestApiInitializeBase() {};

		private:
			bool On_(::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);
			virtual bool OnHttpRequest(::wthttp::context_ptr& context, ::wthttp::request_ptr &req);
			virtual bool OnHttpWsRequest(::httpws::context_ptr& context, ::httpws::request_ptr &req);

		private:
			virtual bool OnBase(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) { return false; }
			virtual bool On(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) = 0;
			virtual bool On(const ::httpws::request_t &req, std::unique_ptr< ::httpws::Response> &res) { return false; }

		protected:
			virtual bool OnInitialize(const std::string& config);
			virtual void OnTerminate();

		protected:
			::log4cplus::Logger& Logger_()
			{
				return logger_;
			}

		protected:
			::log4cplus::Logger logger_;

		protected:
			std::vector<std::string> restapis_;

		private:
			Status status_;
		};
	}
}

#define REFLECT_SERVICE(CLASS)                                     \
public:                                                            \
    static HttpLayer* CreateInstance()                             \
    {                                                              \
        return (HttpLayer*) new CLASS();                           \
    }                                                              \


#define RESTAPI_BASE_BEGIN()                                       \
private:                                                           \
	void RestApiInitializeBase()                                   \
    {                                                              \

#define RESTAPI_BASE_END()                                         \
    }                                                              \

#define RESTAPI_BEGIN()							                   \
private:                                                           \
    void RestApiInitialize()                                       \
    {                                                              \

#define REST_API(API)                                               \
        restapis_.push_back( (API) );                              \

#define RESTAPI_END()                                              \
    }                                                              \


namespace wsf
{
	namespace gstreamer
	{
		class Service : public IService
		{
			RESTAPI_BASE_BEGIN()
			REST_API("POST /$/pipeline/$id")
			REST_API("DELETE /$/pipeline/$id")
			RESTAPI_BASE_END()

		public:
			Service();
			virtual ~Service() {}

		private:
			bool OnBase(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);

		private:
			virtual void RestApiInitialize() = 0;
			virtual bool On(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) = 0;
		};
	}
}

#endif