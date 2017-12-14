#ifndef WSF_ACTOR_FRAMEWORK_HPP
#define WSF_ACTOR_FRAMEWORK_HPP

#include <Theron/Theron.h>
#include <string>
#include <assert.h>
#ifndef _WIN32
#include <mm_malloc.h>
#endif
#include <assert.h>


namespace wsf
{
    namespace actor
    {
		//declare the function
		bool Initialize(const std::string&);
		void Terminate();
        class Framework : public ::Theron::Framework
        {
		public:
            Framework(const Theron::Framework::Parameters &params = Theron::Framework::Parameters());
            ~Framework();

        public:
            void *operator new(size_t i)
            {
                return _mm_malloc(i, 64);
            }

            void operator delete(void *p)
            {
                _mm_free(p);
            }
			static inline Framework& instance() {
				assert(instance_);
				return *instance_;
			}
		private:
			static Framework* instance_;
			friend bool ::wsf::actor::Initialize(const std::string&);
			friend void ::wsf::actor::Terminate();
        };

    }
}

#endif
