#include "calc.hpp"


#include <functional>
void main()
{
	
	
	std::string config = "";// "{\"services\":[{\"prototype\":\"calc\",\"port\":8080,\"prefix\":\"/calc\"}]}";
	
	wsf::actor::CreateServicePrototype<Calc>("calc");

	wsf::actor::Initialize(config);

	getchar();
	::wsf::actor::Terminate();

}