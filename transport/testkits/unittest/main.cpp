#include "http/http.hpp"
#include "websocket/websocket.hpp"
#include <wsf/util/logger.hpp>
#include <wsf/transport/transport.hpp>
#include <iostream>

int main()
{
#ifdef UNIT_DEBUG
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
//_CrtSetBreakAlloc(19715);
#endif
    using namespace log4cplus;
    Logger root = Logger::getInstance("");
    root.setLogLevel(INFO_LOG_LEVEL);

    SharedAppenderPtr consoleAppender(new ConsoleAppender());
    std::auto_ptr<Layout> simpleLayout(new SimpleLayout());
    consoleAppender->setLayout(simpleLayout);
    root.addAppender(consoleAppender);


    

    //////////////////////////////////////////////////////////////////////////
    ::wsf::transport::Initialize();

    ::wsf::transport::http_test::Initialize();
    ::wsf::transport::websocket_test::Initialize();
	std::cout << "Enter 'q' or 'Q' to exit.\n"
		<< std::endl;
    // Synchronize with the dummy message sent in reply to make sure we're done.
    while (1)
    {
        //std::cout << "Enter 'q' or 'Q' to exit." << std::endl;

        char c = getchar();
        if (c == 'q' || c == 'Q')
            break;
    };


    //////////////////////////////////////////////////////////////////////////
    ::wsf::transport::http_test::Terminate();
    ::wsf::transport::websocket_test::Terminate();

    ::wsf::transport::Terminate();



    return 0;
}
