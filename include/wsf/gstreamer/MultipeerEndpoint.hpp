#ifndef WSF_GSTREAMER_MULTIPEER_ENDPOINT
#define WSF_GSTREAMER_MULTIPEER_ENDPOINT

#include "WebRTCEndpoint.hpp"
#include <wsf/gstreamer/Pipeline.hpp>
#include <wsf/gstreamer/gstreamer.hpp>

#include <utility>

namespace wsf
{
	namespace gstreamer
	{
		std::pair<GstElement*, GstElement*> make_pipeline_connector( const std::string& sink_bin="sink_bin", const std::string&source_bin="source_bin" );
		void destroy_pipeline_connector( std::pair<GstElement*, GstElement*>& connector );

		class MultipeerEndpoint
		{
		public:
			explicit MultipeerEndpoint(IPipeline &pipeline);
			~MultipeerEndpoint();
			
			bool OnJoinGroup(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);
			bool OnSessionDescription(const ::wshttp::request_t &req, std::unique_ptr< ::wshttp::Response> &res);
			bool OnLinkSink(GstBin *sink_bin);

			bool OnAddRtspSink(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);
			
			GstElement* OnAddSourceCandidate();
			//void OnSwitchSource(const std::string& source_info);
		private:
			void OnTryLinkSourcePipeline();

		private:
			void OnInitializeAdapter();
			void OnTerminateAdapter();

		private:
			void OnAddRtspSink(const std::string& net_address);
			static void OnLinkRtspSink(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer multipeer_endpoint);

		private:
			IPipeline &pipeline_;

		private:
			std::vector<WebRTCEndPoint> endpoints;

			GstElement *adapter;
			GstElement *input_selector;
			GstElement *tee;
		};
	}
}

#endif