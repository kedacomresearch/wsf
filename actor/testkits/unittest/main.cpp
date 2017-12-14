

#include <wsf/actor.hpp>
#include "timer/Service.hpp"
#include "echo/Service.hpp"
#include "call/Service.hpp"
#include "notify/Service.hpp"



int main(int argc, char *argv[])
{
	std::string prefix="/service-center";
	uint16_t    port = 80;
	if (argc > 1)
	{
		port = (uint16_t)std::stoi(argv[1]);
	}

	std::cout << "WSF Actor Unittest Service-Center " << std::endl
		<< "http://0.0.0.0:" << std::to_string(port) << prefix << std::endl
		<< std::endl;

	std::string config = "{\"service-center\" : "\
		"{ \"prefix\":\"" + prefix + "\",\"port\":" + std::to_string(port) + "}"\
		"}"
		;

	wsf::actor::CreateServicePrototype<timer::Service>("timer");
	wsf::actor::CreateServicePrototype<echo::Service>("echo");
	wsf::actor::CreateServicePrototype<call::Service>("call");
	wsf::actor::CreateServicePrototype<notify::Service>("notify");

	wsf::actor::Initialize(config);

	std::cout << "Enter 'q' or 'Q' to exit.\n"
		<< std::endl;
	// Synchronize with the dummy message sent in reply to make sure we're done.
	getchar();

	::wsf::actor::Terminate();
	return 0;
}