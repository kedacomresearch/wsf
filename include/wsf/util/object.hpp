#ifndef WSF_UTIL_OBJECT_HPP
#define WSF_UTIL_OBJECT_HPP

#include <stdint.h>

namespace wsf
{
    namespace util
    {
        class IObject
        {
        protected:
            IObject(){};
            //virtual void *object_ptr() const = 0;

        public:
            virtual ~IObject()
            {
            }
// 
//             template <typename T>
//             inline T &value()
//             {
//                 return *static_cast<T *>(object_ptr());
//             }
        };

//         template <typename T>
//         class Object : public IObject
//         {
//         public:
//             Object()
//             {
//             }
// 
//             Object(T &obj)
//                 : obj_(obj)
//             {
//             }
// 
//             T &value()
//             {
//                 return obj_;
//             }
// 
//         protected:
//             void *object_ptr() const
//             {
//                 return const_cast<T *>(&obj_);
//             }
// 
//         private:
//             T obj_;
//         };
    }
}

#endif//!
