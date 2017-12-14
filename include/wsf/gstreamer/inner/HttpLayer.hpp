#ifndef WSF_GSTREAMER_INNER_HTTPLAYER_HPP
#define WSF_GSTREAMER_INNER_HTTPLAYER_HPP

#include <gstreamer-1.0/gst/gst.h>
#include <wsf/transport/transport.hpp>
#include <vector>

namespace wthttp = ::wsf::transport::http;
namespace wtws = ::wsf::transport::websocket;
namespace httpws = ::wsf::transport::httpws;
namespace wshttp = httpws;

namespace wsf
{
	namespace gstreamer
	{
		class Service;
		class ServiceCenter;
		class IPipeline;

		class HttpLayer
		{
		protected:
			HttpLayer();
			virtual ~HttpLayer();

		public:
			bool Initialize(const std::string& config, std::string& error);
			void Terminate();
			inline gboolean Post(GstStructure *stru)
			{
				return gst_bus_post(bus_, gst_message_new_application(GST_OBJECT_CAST(pipeline_), stru));
			}

		protected:
			void OnHttpRequestEntry(::wthttp::context_ptr& context, ::wthttp::request_ptr &req);
			void OnHttpWsRequestEntry(::httpws::context_ptr &context, ::httpws::request_ptr &req);

		private:
			bool Initialize(const transport::http::request_t &req, std::unique_ptr< transport::http::Response>& res);
			virtual bool OnInitialize(const transport::http::request_t &req, std::unique_ptr< transport::http::Response>& res) { return true; }

		private:
			virtual bool OnInitialize(const std::string& config) { return true; }
			virtual void OnTerminate() {}

		private:
			HttpLayer * RestApiDispatcherMatch(::wthttp::request_ptr &req);
			virtual bool RestApiMatch(::wthttp::request_ptr &req) { return false; }

		private:
			static gboolean OnHttp_(GstBus * bus, GstMessage * message, gpointer httplayer);
			virtual bool OnHttpRequest(::wthttp::context_ptr& context, ::wthttp::request_ptr &req) = 0;
			virtual bool OnHttpWsRequest(::httpws::context_ptr& context, ::httpws::request_ptr &req) = 0;
			virtual bool OnHttp(::wthttp::context_ptr& context, ::wthttp::request_ptr &req);

		private:
			void set_klass(const std::string& klass) { klass_ = klass; }

		private:
			virtual gboolean OnBusMessage(GstBus * bus, GstMessage * message) { return false; }

		private:

			GstElement *pipeline_;
			GstBus * bus_;
			guint bus_watcher_;

		private:
			std::vector<HttpLayer*> httplayers_;
			bool stoped_;

		protected:
			std::string klass_;

			friend class Service;
			friend class ServiceCenter;
			friend class IPipeline;
		};
	}
}

#endif