#ifndef WSF_GSTREAMER_SDP_SESSIONDESCRIPTION
#define WSF_GSTREAMER_SDP_SESSIONDESCRIPTION
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <type_traits>
#include <sstream>
#include <ostream>

//#define ENABLE_SSRC

namespace wsf
{
	namespace gstreamer
	{
		namespace sdp
		{
			struct origin_t
			{
				std::string username = "-";
				std::string session_id = "1";
				std::string session_version = "1";
				std::string net_type = "IN";
				std::string address_type = "IP4";
				std::string address = "127.0.0.1";
			};

			struct connection_data_t
			{
				std::string net_type;
				std::string address_type;
				std::string address;
				uint16_t ttl = 0;
				uint8_t addresses_number = 0;

			};

			struct bandwidth_t
			{
				std::string bandwidth_type;
				std::string bandwidth;
			};

			struct session_time_t
			{
				struct timing_t
				{
					uint64_t start_time = 0;
					uint64_t stop_time = 0;
				};
				struct repeat_time_t
				{
					uint64_t repeat_interval;
					uint64_t active_duration;
					std::vector<std::string> offsets;
				};

				timing_t timing;
				repeat_time_t repeat_time;              // [opt]
			};

			struct time_zone_t
			{
				struct adjustment_pair_t
				{
					uint64_t adjustment_time;
					uint64_t offset;
				};
				std::vector<adjustment_pair_t> adjustments;
			};

			struct encryption_key_t
			{
				std::string method;
				std::string key_information;                       // [opt]
			};
			
			struct payload_t
			{
				int64_t payload_type;
				std::string encoding_name;
				int64_t clockrate;
				int64_t channels;
				uint16_t encoding_parameters;
				std::vector<std::string> rtcp_fb;
				std::map<std::string, std::string> fmtp;
				int rtx_payload = -1;//retransmission payload
			};
			
			struct rtcp_t
			{
				std::string net_type;
				uint16_t port;
				std::string address_type;
				std::string address;
				bool mux = false;
				bool rsize = false;
			};
			
			struct candidate_t
			{
				std::string foundation;
				int componentid;
				std::string transport_protocol;
				std::string transport_extension;
				std::string tcp_type;
				uint32_t priority;
				std::string connection_address;
				uint16_t port;
				std::string candidate_type;
				std::string related_address;
				uint16_t related_port = 0;
				std::map<std::string, std::string> extension_attr;
			};

			struct remote_candidate_t
			{
				int componentid;
				std::string connection_address;
				uint16_t port;
			};

			struct ice_t
			{
				std::string ufrag;
				std::string password;
				bool lite = false;
				bool mismatch = false;
				std::vector<std::string> options;
				std::vector<candidate_t> candidates;
				std::vector<remote_candidate_t> remote_candidates;
			};

			struct dtls_t
			{
				std::string fingerprint_hashfunction;
				std::string fingerprint;
				std::string setup;
			};


			struct media_description_t
			{
				std::string media_type;
				uint16_t port;
				int ports_number = 0;
				std::string protocol;
				std::string mode;
				std::vector<payload_t> payloads;
				rtcp_t rtcp_info;

				std::string ssrc_group = "";
				std::vector<uint32_t> ssrcs;
				std::string cname;
				std::string mediastreamid;
				std::string mediastreamtrackid;

				ice_t ice;
				dtls_t dtls;
				std::string mid;                                   //[com:media stream identification]
				int group;

				std::string session_information;                   // [opt]
				std::vector<connection_data_t> connection_datas;   // [opt]
				bandwidth_t bandwidth;                             // [opt]
				encryption_key_t key;                              // [opt]
			};

			struct group_t
			{
				std::string semantics;
				std::string identification_tag;
				std::vector<int> media_descriptions;
			};

			struct session_description_t
			{
				int version = 0;
				origin_t origin;
				std::string session_name = "-";
				std::string session_information;                                 // [opt]
				std::string uri;                                                 // [opt]
				std::vector<std::string> email_address;                          // [opt]
				std::vector<std::string> phone_number;                           // [opt]
				connection_data_t connection_data;                               // [opt]
				bandwidth_t bandwidth;                                           // [opt]
				std::vector<session_time_t> time;                                // [>=1]
				time_zone_t time_zone;                                           // [opt]
				encryption_key_t key;                                            // [opt]
				std::vector<group_t> groups;
				std::vector<media_description_t> media_descriptions;             // [>=1]
#ifdef ENABLE_SSRC
				std::string msid_semantics;
#endif
				std::vector<std::string> unparsed;

			};

			bool Parse(const std::string& sdp_text, session_description_t& session_description, std::string& error);
			bool ParseCandidate(const std::string&sdp_candidate, candidate_t& candidate);
			bool Serialize(const session_description_t& session_description, std::string& sdp_text, std::string& error);

			template <typename T>
			T to_number(const std::string &value)
			{
				T result;
				std::stringstream ss(value);
				ss >> result;

				return result;
			}

			class Tokenizer
			{
			public:
				Tokenizer(std::string& value, char sep=' ')
					: value_(value), sep_(sep), begin_(0)
				{}

				~Tokenizer()
				{
					assert(begin_ == std::string::npos);
				}

				operator bool()
				{
					return begin_ != std::string::npos;
				}

				Tokenizer & operator >> (std::string& field_value)
				{
					peek(field_value);
					return *this;
				}

				template<typename T>
				Tokenizer & operator >> (T& field_value)
				{
					peek(buffer);
					//static_assert(std::is_arithmetic<T>::value);
					field_value = to_number<T>(buffer);

					return *this;
				}
				void switch_sep(char sep)
				{
					sep_ = sep;
				}

				void take_rest(std::string& field_value)
				{
					if (begin_ != std::string::npos)
					{
						field_value = value_.substr(begin_);
						begin_ = std::string::npos;
					}
				}

			private:
				void peek(std::string& field_value)
				{
					std::size_t pos = value_.find_first_of(sep_, begin_);

					std::size_t len = (pos == std::string::npos) ? pos : (pos - begin_);
					field_value = value_.substr(begin_, len);

					begin_ = (pos == std::string::npos) ? pos : (pos + 1);

				}

			private:
				std::string & value_;
				char sep_;

			private:
				std::size_t begin_;
				std::string buffer;
			};

			origin_t& operator << (origin_t& origin, std::string& value);
			connection_data_t& operator << (connection_data_t& connection_data, std::string& value);
			bandwidth_t& operator << (bandwidth_t& bandwidth, std::string& value);
			session_time_t::timing_t & operator << (session_time_t::timing_t& timing, std::string& value);
			session_time_t::repeat_time_t & operator << (session_time_t::repeat_time_t& repeat_time, std::string& value);
			time_zone_t & operator << (time_zone_t& time_zone, std::string& value);
			encryption_key_t & operator << (encryption_key_t& key, std::string& value);
			media_description_t & operator << (media_description_t& media_description, std::string& value);

			std::ostream & operator << (std::ostream & os, const origin_t& origin);
			std::ostream & operator << (std::ostream & os, const connection_data_t& connection_data);
			std::ostream & operator << (std::ostream & os, const bandwidth_t& bandwidth);
			std::ostream & operator << (std::ostream & os, const session_time_t::timing_t& timing);
			std::ostream & operator << (std::ostream & os, const session_time_t::repeat_time_t& repeat_time);
			std::ostream & operator << (std::ostream & os, const time_zone_t& time_zone);
			std::ostream & operator << (std::ostream & os, const encryption_key_t& key);
			std::ostream & operator << (std::ostream & os, const group_t& group);
			std::ostream & operator << (std::ostream & os, const payload_t& payload);
			std::ostream & operator << (std::ostream & os, const candidate_t& candidate);
			std::ostream & operator << (std::ostream & os, const ice_t& ice);
			std::ostream & operator << (std::ostream & os, const media_description_t& media);
		}
	}
}

#endif