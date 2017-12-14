#include "Framework.hpp"

namespace wsf
{
    namespace actor
    {
		Framework* Framework::instance_ = NULL;

        Framework::Framework(const Theron::Framework::Parameters &params /* = Theron::Framework::Parameters()*/)
             : Theron::Framework(params)
        {
        }

        Framework::~Framework()
        {
        }

    }
}
