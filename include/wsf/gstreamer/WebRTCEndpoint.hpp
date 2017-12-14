#ifndef WSF_GSTREAMER_WEBRTC_ENDPOINT
#define WSF_GSTREAMER_WEBRTC_ENDPOINT

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
#include <wsf/gstreamer/Pipeline.hpp>

#define ENABLE_UPDATE_SIGNALLING_SERVER TRUE

namespace wsf
{
	namespace gstreamer
	{
		class MultipeerEndpoint;
		class WebRTCEndPoint
		{
		public:
			explicit WebRTCEndPoint( IPipeline &pipeline, MultipeerEndpoint *multipeer_endpoint=NULL )
				: pipeline_(pipeline), multipeer_endpoint_(multipeer_endpoint) {}

			bool OnJoinGroup(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res);
			bool OnLeaveGroup(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) { return false; }

			bool OnSessionDescription(const ::wshttp::request_t &req, std::unique_ptr< ::wshttp::Response> &res);
			bool OnPeerState(const ::wshttp::request_t &req, std::unique_ptr< ::wshttp::Response> &res) { return true; }

		private:
			bool OnOffer(const ::wshttp::request_t &req, std::unique_ptr< ::wshttp::Response> &res);
			bool OnRemoteCandidate(const ::wshttp::request_t &req, std::unique_ptr< ::wshttp::Response> &res);

		private:
			void TryAnswer();
			bool CanAnswer();
			void Answer();

		private:
			static void OnLocalSources(GList *sources, gpointer endpoint);
			static void OnSource(GObject *local_source, GstBin* sink_bin, gpointer endpoint);
			static void OnRemoteSource(OwrMediaSession *media_session, OwrMediaSource *source, gpointer endpoint);
			static void OnLocalCandidate(GObject *media_session, OwrCandidate *candidate, gpointer null);
			static void OnLocalCandidatesGatheringDone(GObject *media_session, gpointer endpoint);
			static void OnDtlsCertificate(GObject *media_session, GParamSpec *pspec, gpointer endpoint);

		private:
			static void OnAnswerPayload(GObject *media_session, const std::string& media_type, sdp::payload_t &payload, std::vector<sdp::payload_t>& answer_payloads, const std::vector<sdp::payload_t>& offer_payloads);
			static void OnAnswerICE(GObject *media_session, sdp::ice_t &ice);
			static void OnAnswerDtls(GObject *media_session, sdp::dtls_t &dtls);
			static bool OnOfferPayload(OwrMediaSession *session, const sdp::payload_t &payload, const std::string& s_media_type, OwrMediaType& media_type, OwrCodecType& codec_type, const std::vector<sdp::payload_t>& payloads);
			static void OnOfferICE(OwrMediaSession *session, const sdp::ice_t &ice, bool rtcpmux);
			static void OnSetLocalSource(OwrMediaSession *session, OwrMediaType media_type, GList *local_sources);

		private:
			IPipeline &pipeline_;
			MultipeerEndpoint *multipeer_endpoint_ = NULL;

			std::string group_id;
			std::string id;

		private:
			GList *local_sources = NULL;
			GList *renderers = NULL;
			OwrTransportAgent *transport_agent =  NULL;
			sdp::session_description_t session_description;
		};
	}
}

#endif