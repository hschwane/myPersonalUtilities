/*
 * mpUtils
 * SyslogStreambuf.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the SyslogStreambuf class, which is used by the Log class to output to the Linux syslog.
 *
 * Copyright 2016 Hendrik Schwanekamp
 *
 */

#ifdef __linux__ // only compile for linux

// includes
//--------------------
#include "SyslogStreambuf.h"
// include Log here, to resolve the forward declaration of "class Log;" in the header
#include "Log.h"
//--------------------

// namespace
//--------------------
namespace mpu {
//--------------------

// functions of the SyslogStreambuf class
//-------------------------------------------------------------------

SyslogStreambuf::SyslogStreambuf(const std::string &sIdent, int facility, Log *pLog)
{
    if (!pLog)
        throw std::invalid_argument("SyslogStreambuffer needs a Log to take the current loglvl from!");
    pAssociatedLog = pLog;
    this->sIdent = sIdent;
    openlog(this->sIdent.c_str(), LOG_PID | LOG_NDELAY, facility);
}

SyslogStreambuf::~SyslogStreambuf()
{
    closelog();
}

int SyslogStreambuf::sync()
{
    int iPriority;
    switch (pAssociatedLog->getLastLvl())
    {
    case LogLvl::FATAL_ERROR:
        iPriority = LOG_CRIT;
        break;
    case LogLvl::ERROR:
        iPriority = LOG_ERR;
        break;
    case LogLvl::WARNING:
        iPriority = LOG_WARNING;
        break;
    case LogLvl::INFO :
        iPriority = LOG_INFO;
        break;
    case LogLvl::DEBUG :
    case LogLvl::DEBUG2:
        iPriority = LOG_DEBUG;
        break;
    default:
        // if there is no valid log level just delete the message
        sBuffer.empty();
        return 0;   // don't indicate a error, or else it would render the stream useless
        // (this is a log, you don't want to check it all the time to reset
        // error flags, it just needs to work)
    }

    if (!sBuffer.empty())
    {
        ::syslog(iPriority, "%s",sBuffer.c_str());
        sBuffer = "";
    }
    return 0;
}

}

#endif // __linux__