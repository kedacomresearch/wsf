#ifndef WSF_GSTREAMER_PIPELINE_TRAITS_HPP
#define WSF_GSTREAMER_PIPELINE_TRAITS_HPP

#include <string>
#include <functional>
#include <wsf/util/json.hpp>

using json = nlohmann::json;

namespace wsf
{
	namespace gstreamer
	{
		class IPipeline;
		struct Source
		{
			struct param_t
			{
				std::string url;
				bool extract(const std::string& config)
				{
					auto param = json::parse(config.c_str());
					const std::string &url_ = param["SourceURL"];
					this->url = url_;
					return true;
				}
			};
			typedef param_t Param;
		};

		struct RtspSource : public Source
		{
		};

		struct HlsSource : public Source
		{
		};

		struct WebrtcSource : public Source
		{
		};

		struct ReplaySource
		{
			struct param_t : public Source::param_t
			{
				int64_t begin;
				int64_t end;
				bool extract(const std::string& config)
				{
					return true;
				}
			};

			typedef param_t Param;
		};

		template <typename Source>
		struct SourceTraits;

	}
}

#define PIPELINE_SOURCE_DECLARE(SOURCE)                                                                 \
namespace wsf                                                                                           \
{                                                                                                       \
    namespace gstreamer                                                                                 \
    {                                                                                                   \
        template <>                                                                                     \
        struct SourceTraits<SOURCE>                                                                     \
        {                                                                                               \
            static const char* const Name;                                                              \
            typedef typename SOURCE Type;                                                               \
            static IPipeline *originate();                                                              \
            static bool extract(const std::string&, typename SOURCE::Param& );                          \
        };                                                                                              \
    }                                                                                                   \
}                                                                                                       \

#define PIPELINE_SOURCE_DEFINE(SOURCE)                                                                  \
const char* const ::wsf::gstreamer::SourceTraits<SOURCE>::Name = #SOURCE;                               \
namespace wsf                                                                                           \
{                                                                                                       \
	namespace gstreamer                                                                                 \
	{                                                                                                   \
		IPipeline * SourceTraits<SOURCE>::originate()                                                   \
        {                                                                                               \
            return new Pipeline<SourceTraits<SOURCE>::Type>();                                          \
        }                                                                                               \
        bool SourceTraits<SOURCE>::extract(const std::string& config, typename SOURCE::Param& param)    \
        {                                                                                               \
            return param.extract(config);                                                               \
        }                                                                                               \
    }                                                                                                   \
}                                                                                                       \

PIPELINE_SOURCE_DECLARE(::wsf::gstreamer::RtspSource)
PIPELINE_SOURCE_DECLARE(::wsf::gstreamer::HlsSource)
PIPELINE_SOURCE_DECLARE(::wsf::gstreamer::WebrtcSource)
PIPELINE_SOURCE_DECLARE(::wsf::gstreamer::ReplaySource)


//--------------------------------------------------------------------------------------

namespace wsf
{
	namespace gstreamer
	{
		struct Sink
		{
			struct param_t
			{
				std::string url;
				bool extract(const std::string& config)
				{
					auto param = json::parse(config.c_str());
					const std::string &url_ = param["url"];
					this->url = url_;
					return true;
				}
			};
			typedef param_t Param;
		};

		struct RtspSink : public Sink
		{
		};

		struct HlsSink : public Sink
		{
		};

		struct WebrtcSink : public Sink
		{
		};

		struct ReplaySink
		{
			struct param_t : public Sink::param_t
			{
				std::string fragment_location;
				std::string slice_location;
				bool extract(const std::string& config)
				{
					auto param = json::parse(config.c_str());
					const std::string& fl = param["fragment_location"];
					fragment_location = fl;
					return true;
				}
			};
			typedef param_t Param;
		};

		template <typename Sink>
		struct SinkTraits;

	}
}

#define PIPELINE_SINK_DECLARE(SINK)                                                               \
namespace wsf                                                                                     \
{                                                                                                 \
    namespace gstreamer                                                                           \
    {                                                                                             \
        template <>                                                                               \
        struct SinkTraits<SINK>                                                                   \
        {                                                                                         \
            static const char* const Name;                                                        \
            typedef typename SINK Type;                                                           \
            static bool extract(const std::string&, typename SINK::Param& );                      \
        };                                                                                        \
    }                                                                                             \
}                                                                                                 \

#define PIPELINE_SINK_DEFINE(SINK)                                                                \
const char* const ::wsf::gstreamer::SinkTraits<SINK>::Name = #SINK;                               \
namespace wsf                                                                                     \
{                                                                                                 \
    namespace gstreamer                                                                           \
    {                                                                                             \
        bool SinkTraits<SINK>::extract(const std::string& config, typename SINK::Param& param)    \
        {                                                                                         \
            return param.extract(config);                                                         \
        }                                                                                         \
    }                                                                                             \
}                                                                                                 \

PIPELINE_SINK_DECLARE(::wsf::gstreamer::RtspSink)
PIPELINE_SINK_DECLARE(::wsf::gstreamer::HlsSink)
PIPELINE_SINK_DECLARE(::wsf::gstreamer::WebrtcSink)
PIPELINE_SINK_DECLARE(::wsf::gstreamer::ReplaySink)

#endif