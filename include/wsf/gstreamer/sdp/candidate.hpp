#ifndef WSF_GSTREAMER_SDP_CANDIDATE
#define WSF_GSTREAMER_SDP_CANDIDATE
#include "SessionDescription.hpp"
#include <owr_candidate.h>
namespace wsf
{
	namespace gstreamer
	{
		namespace sdp
		{
			OwrCandidate * MakeCandidate(const candidate_t& candidate);
			void DestroyCandidate(OwrCandidate * candidate);
		}
	}
}
#endif