#ifndef WSF_GSTREAMER_PIPELINE_HPP
#define WSF_GSTREAMER_PIPELINE_HPP

#include <wsf/gstreamer/Service.hpp>


namespace wsf
{
	namespace gstreamer
	{

		class IPipeline : public IService
		{
			RESTAPI_BASE_BEGIN()
			REST_API("POST /$/pipeline/$pipelineid/sink/$sinkid")
			REST_API("DELETE /$/pipeline/$pipelineid/sink/$sinkid")
			REST_API("POST /$/pipeline/$pipelineid/webrtc-endpoint/$endpoint_id")
			REST_API("POST /$/pipeline/$pipelineid/webrtc-endpoint/endpoint/$group_id/$endpoint_id")
			REST_API("DELETE /$/pipeline/$pipelineid/webrtc-endpoint/endpoint/$group_id/$endpoint_id")
			RESTAPI_BASE_END()

		protected:
			IPipeline();
			virtual ~IPipeline() {}

		protected:
			GstElement* pipeline() const;
		public:
			bool call(::httpws::request_ptr &req, std::function< void(const ::httpws::response_t &res) > fn);
			std::string &Id() { return id_; }

		private:
			virtual bool RestApiMatch(::wthttp::request_ptr &req);

		private:
			virtual bool On(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) = 0;
			virtual bool On(const ::httpws::request_t &req, std::unique_ptr< ::httpws::Response> &res)
			{
				return true;
			}
			virtual bool OnAddSource(const std::string& config) = 0;
			virtual bool OnAddSink(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) = 0;

		public:
			//virtual bool OnLinkWebRTCSink(GstBin *sink_bin) { return false; }
			//virtual bool OnLinkMultipeerEndpoint(GstElement *sink_bin) { return false; }
			virtual bool OnLinkSink(GstElement *sink_bin) { return false; }

		private:
			virtual bool OnBase(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);
			virtual bool OnRemoveSink(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) { return false; }

		private:
			static gpointer MainLoop_(gpointer data);
			void MainLoop_();
			virtual bool OnInitialize(const std::string& config);
			virtual void OnTerminate();
			virtual bool OnInitialize(const ::wthttp::request_t &req, std::unique_ptr< transport::http::Response>& res);
		protected:
			bool OnStartMainLoop();
			void OnStopMainLoop();

		private:
			GThread *mainloop_thread_;
			GMainLoop *mainloop_;

			std::string id_;

		};
	}
}

#endif