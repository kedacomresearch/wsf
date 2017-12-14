#ifndef WSF_GSTREAMER_GSTREAMER_HPP
#define WSF_GSTREAMER_GSTREAMER_HPP

#include <string>
#include <functional>
#include <gst/rtsp-server/rtsp-server.h>


namespace wsf
{
	namespace gstreamer
	{
		bool Initialize(int argc, char * argv[]);
		void Terminate();
		void ConnectSignallingBridge(const std::string &url);

		class ServiceCenter;
		ServiceCenter* GetServiceCenter();


		class HttpLayer;
		namespace servicefactory
		{
			bool RegistService(const std::string& service_klass, std::function<HttpLayer*()>);
			HttpLayer *CreateService(const std::string& service_klass);
		}
	}
}

#define REG_SERVICE(KLASS, SERVICE) ::wsf::gstreamer::servicefactory::RegistService(KLASS, SERVICE::CreateInstance)
#define CREATE_SERVICE(SERVICE) ::wsf::gstreamer::servicefactory::CreateService(SERVICE)


namespace wsf
{
	namespace gstreamer
	{
		template <typename Source>
		struct SourceTraits;
		class IPipeline;

		bool RegistPipelineSource(const std::string& source_type, std::function<IPipeline*()>);
		IPipeline *CreatePipeline(const std::string& source_type);
	}
}


#define REG_SOURCE(SOURCE)    \
::wsf::gstreamer::RegistPipelineSource(::wsf::gstreamer::SourceTraits<SOURCE>::Name, ::wsf::gstreamer::SourceTraits<SOURCE>::originate) \

#define CREATE_PIPELINE(SOURCE_STR) ::wsf::gstreamer::CreatePipeline(SOURCE_STR)

namespace wsf
{
	namespace gstreamer
	{
		GstRTSPServer *RtspServer();
	}
}


#endif