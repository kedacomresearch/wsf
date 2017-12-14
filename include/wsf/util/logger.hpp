#ifndef WSF_UTIL_LOGGER_HPP
#define WSF_UTIL_LOGGER_HPP
#include <log4cplus/appender.h>
#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/loggingmacros.h>


#ifndef _DISABLE_LOGGER_SHUTCUT_

//global logger register
#define LOGGER(name)                                   \
    static ::log4cplus::Logger Logger_()               \
    {                                                  \
        return ::log4cplus::Logger::getInstance(name); \
    }


#define FATAL(event)                   \
    LOG4CPLUS_FATAL(Logger_(), event); \
    assert(0);

#define ERR(event) \
    LOG4CPLUS_ERROR(Logger_(), event)

#define WARN(event) \
    LOG4CPLUS_WARN(Logger_(), event)

#define INFO(event) \
    LOG4CPLUS_INFO(Logger_(), event)

#define DEBUG(event) \
    LOG4CPLUS_DEBUG(Logger_(), event)

#define TRACE(event) \
    LOG4CPLUS_TRACE(Logger_(), event)

#endif


#endif
