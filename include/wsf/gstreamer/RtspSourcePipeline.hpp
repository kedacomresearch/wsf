#ifndef WSF_GSTREAMER_RTSPSOURCE_PIPELINE_HPP
#define WSF_GSTREAMER_RTSPSOURCE_PIPELINE_HPP

#include "PipelineTraits.hpp"
#include <wsf/util/json.hpp>

#include <owr_media_session.h>
#include <owr_audio_payload.h>
#include <owr_video_payload.h>
#include <owr_media_renderer.h>
#include <owr_audio_renderer.h>
#include <owr_video_renderer.h>
#include <owr_transport_agent.h>
#include <owr_local.h>

#include <wsf/gstreamer/sdp/SessionDescription.hpp>
#include <wsf/gstreamer/sdp/candidate.hpp>
#include <wsf/gstreamer/WebRTCEndpoint.hpp>
#include <wsf/gstreamer/MultipeerEndpoint.hpp>

using json = nlohmann::json;

#define ENABLE_PCMA TRUE
#define ENABLE_PCMU TRUE
#define ENABLE_OPUS TRUE
#define ENABLE_H264 TRUE
#define ENABLE_VP8  FALSE

#define ENABLE_PEER2PEER FALSE
#define ENABLE_MULTIPEER FALSE

namespace wsf
{
	namespace gstreamer
	{
		template<>
		class Pipeline<RtspSource> : public wsf::gstreamer::IPipeline
		{
		protected:
			typedef typename Source::Param SourceParam;
			Pipeline() : IPipeline(), replay_sink_pad_(NULL), replay_sink_(NULL) {}
			virtual ~Pipeline() {}

		public:
			virtual bool OnLinkSink(GstElement *sink_bin)
			{
				//GstElement *sink = mpendpoint->OnAddSourceCandidate();
				g_return_val_if_fail( gst_bin_add(GST_BIN_CAST(pipeline()), sink_bin), false );
				GstElement * parse = gst_bin_get_by_name(GST_BIN_CAST(pipeline()), "parse");
				g_return_val_if_fail( gst_element_link(parse, sink_bin), false );

				gst_element_set_state(pipeline(), GST_STATE_PLAYING);

				return true;
			}

		private:
			virtual bool OnAddSource(const SourceParam& param);
			virtual bool OnAddSink(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);
			virtual bool On(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) 
			{ 
				if (ENABLE_PEER2PEER)
				{
					endpoints.push_back(WebRTCEndPoint(*this));
					return endpoints.back().OnJoinGroup(req, res);
				}
				else if (ENABLE_MULTIPEER)
				{
					mpendpoint.reset(new MultipeerEndpoint(*this));
					return mpendpoint->OnJoinGroup(req, res);
				}
				else if (ENABLE_UPDATE_SIGNALLING_SERVER)
				{
					mpendpoint.reset(new MultipeerEndpoint(*this));
					return mpendpoint->OnJoinGroup(req, res);
				}
				else
				{
					return false;
				}
			}

			virtual bool On(const ::wshttp::request_t &req, std::unique_ptr< ::wshttp::Response> &res)
			{
				if (ENABLE_PEER2PEER)
				{
					return endpoints.back().OnSessionDescription(req, res);
				}
				else if (ENABLE_MULTIPEER)
				{
					return mpendpoint->OnSessionDescription(req, res);
				}
				else if (ENABLE_UPDATE_SIGNALLING_SERVER)
				{
					return mpendpoint->OnSessionDescription(req, res);
				}
				else
				{
					return false;
				}
			}

		private:
			bool OnAddSource(const std::string& config)
			{
				SourceParam src_param_;
				return SourceTraits<RtspSource>::extract(config, src_param_) && OnAddSource(src_param_);
			}

		private:
			virtual void RestApiInitialize() {}

			template <typename Source>
			friend struct SourceTraits;

		//--------------------------------------------------------------------------------------
		private:
			static void cb_rtspsrc_pad_added(GstElement* src,GstPad* src_pad, gpointer depay);
			void OnTerminate();

		private:
			bool OnAddSink(const ReplaySink::Param& param);

		private:
			GstPad* replay_sink_pad_;
			GstElement *replay_sink_;

		private:
			std::vector<wsf::gstreamer::WebRTCEndPoint> endpoints;
			std::unique_ptr<MultipeerEndpoint> mpendpoint;
		};

		bool Pipeline<RtspSource>::OnAddSource(const SourceParam& param)
		{
			GstElement *src = gst_element_factory_make("rtspsrc", "src");
			GstElement *depay = gst_element_factory_make("rtph264depay", "depay");
			GstElement *parse = gst_element_factory_make("h264parse", "parse");

			if (src == NULL || depay == NULL || parse == NULL) return false;

			gst_bin_add_many(GST_BIN(pipeline()), src, depay, parse, NULL);

			g_object_set(G_OBJECT(src), "location", param.url.c_str(), NULL);
			g_signal_connect(src, "pad-added", (GCallback)cb_rtspsrc_pad_added, depay);

			return (gst_element_link(depay, parse) == TRUE) ? true : false;

		}

		void Pipeline<RtspSource>::cb_rtspsrc_pad_added(GstElement* src, GstPad* src_pad, gpointer depay)
		{
			GstCaps* caps = gst_pad_query_caps(src_pad, NULL);
			GstStructure *stru = gst_caps_get_structure(caps, 0);
			const GValue* media_type = gst_structure_get_value(stru, "media");

			if (g_str_equal(g_value_get_string(media_type), "video") )
			{
				GstPad* sink_pad = gst_element_get_static_pad(GST_ELEMENT_CAST(depay), "sink");
				GstPadLinkReturn ret = gst_pad_link(src_pad, sink_pad);
				g_assert(ret == GST_PAD_LINK_OK);
				gst_object_unref(sink_pad);
			}
		}
		
		bool Pipeline<RtspSource>::OnAddSink(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res)
		{
			const std::string & config = req.content();
			const std::string & sink_type = json::parse(config.c_str())["SinkType"];

			std::string error;
			if (sink_type == SinkTraits<RtspSink>::Name)
			{
				mpendpoint.reset(new MultipeerEndpoint(*this));
				mpendpoint->OnAddRtspSink(req, res);
				res->set_status_code(::wsf::http::status_code::OK);
				res->Reply();
				return true;
				//RtspSink::Param param;
				//if( !SinkTraits<RtspSink>::extract(config, param) ) error = "Sink parameter extract fail";
				//if( !OnAddSink(param) ) error = "Sink add substep fail";
			}
			else if (sink_type == SinkTraits<HlsSink>::Name)
			{
				//HlsSink::Param param;
				//return SinkTraits<HlsSink>::extract(config, param, res) && OnAddSink(param, res);
			}
			else if (sink_type == SinkTraits<WebrtcSink>::Name)
			{
				//WebrtcSink::Param param;
				//return SinkTraits<WebrtcSink>::extract(config, param, res) && OnAddSink(param, res);
			}
			else if (sink_type == SinkTraits<ReplaySink>::Name)
			{
				ReplaySink::Param param;
				if ( !SinkTraits<ReplaySink>::extract(config, param) ) error = "Sink parameter extract fail";
				if ( !OnAddSink(param) ) error = "Sink add substep fail";
			}
			else
			{
				error = "Unrecognized sink type";
			}

			if (!error.empty())
			{
				res->set_status_code( ::wsf::http::status_code::Bad_Request );
				res->content() = "{\"reason\":\""+ error +"\"}";
				res->Reply();
				return false;
			}
			if (GST_STATE(pipeline()) != GST_STATE_PLAYING) gst_element_set_state(pipeline(), GST_STATE_PLAYING);
			OnStartMainLoop();

			res->set_status_code(::wsf::http::status_code::OK);
			res->Reply();

			
			return true;
			
		}

		bool Pipeline<RtspSource>::OnAddSink(const ReplaySink::Param& param)
		{
			//replay_sink_ = gst_element_factory_make("splitmuxsink", "sink");
			//
			//gst_bin_add(GST_BIN(pipeline()), replay_sink_);
			//
			//g_object_set(G_OBJECT(replay_sink_), "location", param.fragment_location.c_str(), "max-size-time", 10000000, NULL);
			//
			//replay_sink_pad_ = gst_element_get_request_pad(replay_sink_, "video");
			//GstElement * parse = gst_bin_get_by_name(GST_BIN_CAST(pipeline()), "parse");
			//GstPad* src_pad = gst_element_get_static_pad(parse, "src");
			//GstPadLinkReturn res = gst_pad_link(src_pad, replay_sink_pad_);
			//if (res != GST_PAD_LINK_OK) return false;
			//
			//gst_object_unref(src_pad);

			GstElement * dec = gst_element_factory_make("avdec_h264", "dec");
			GstElement * sink = gst_element_factory_make("d3dvideosink", "sink");

			gst_bin_add_many(GST_BIN(pipeline()), dec, sink, NULL);
			
			GstElement * parse = gst_bin_get_by_name(GST_BIN_CAST(pipeline()), "parse");
			gst_element_link_many(parse, dec, sink, NULL);
			gst_element_set_state(pipeline(), GST_STATE_PLAYING);
			return true;
		}

		void Pipeline<RtspSource>::OnTerminate()
		{
			if (replay_sink_ && replay_sink_pad_)
			{
				gst_element_release_request_pad(replay_sink_, replay_sink_pad_);
			}
		}
	}
}

#endif