#ifndef WSF_GSTREAMER_PIPELINE_TEMPL_HPP
#define WSF_GSTREAMER_PIPELINE_TEMPL_HPP

#include "Pipeline.hpp"

namespace wsf
{
	namespace gstreamer
	{
		template <typename SourceTrait>
		struct SourceTraits;

		template <typename Source>
		class Pipeline : public wsf::gstreamer::IPipeline
		{

		protected:
			typedef typename Source::Param SourceParam;
			Pipeline() : IPipeline() {}
			virtual ~Pipeline() {}

		private:
			virtual bool OnAddSource(const SourceParam& param) { return false; }
			virtual bool OnAddSink(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) { return false; }
			virtual bool On(const ::wthttp::request_t &req, std::unique_ptr< ::wthttp::Response> &res) { return false; }
			

		private:
			bool OnAddSource(const std::string& config);

		private:
			virtual void RestApiInitialize() {}

			template <typename SourceTrait>
			friend struct SourceTraits;
		};

		template <typename Source>
		bool Pipeline<Source>::OnAddSource(const std::string& config)
		{
			SourceParam src_param_;
			return SourceTraits<Source>::extract(config, src_param_) && OnAddSource(src_param_);
		}

	}
}

#include "RtspSourcePipeline.hpp"


#endif