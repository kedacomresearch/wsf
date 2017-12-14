#include <wsf/actor.hpp>
#include <wsf/actor/inner/ServiceCenter.hpp>
#include "Framework.hpp"
#include "timer/ActorTimer.hpp"
#include <wsf/transport/transport.hpp>
namespace wsf
{
    namespace actor
    {
		bool Initialize( const std::string& config )
		{

			{
				using namespace log4cplus;
				Logger root = Logger::getInstance("");
				root.setLogLevel(INFO_LOG_LEVEL);

				SharedAppenderPtr consoleAppender(new ConsoleAppender());
				std::auto_ptr<Layout> simpleLayout(new SimpleLayout());
				consoleAppender->setLayout(simpleLayout);
				root.addAppender(consoleAppender);
			}
			::wsf::transport::Initialize();

			assert(!Framework::instance_ );
			assert(!ServiceCenter::instance_);
			Framework::instance_ = new Framework();
			Timer::instance_ = new Timer();
			if (!Timer::instance().Initialize())
			{
				goto _failed;
			}

			ServiceCenter::instance_ = new ServiceCenter("actor.service-center");
			if (!ServiceCenter::instance().Initialize(config))
			{
				goto _failed;
			}
			
			
			return true;
		_failed:
			Terminate();
			return false;

		}

		
		void Terminate()
		{
			if (ServiceCenter::instance_)
			{
				ServiceCenter::instance().Terminate();
				delete ServiceCenter::instance_;
				ServiceCenter::instance_ = NULL;
			}

			if (Timer::instance_)
			{
				Timer::instance().Terminate();
				delete Timer::instance_;
				Timer::instance_ = NULL;
			}

			if (Framework::instance_)
			{
				delete Framework::instance_;
				Framework::instance_ = NULL;
			}
			::wsf::transport::Terminate();
		}
	}
}