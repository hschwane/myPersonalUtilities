/*
 * mpUtils
 * FileSink.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the FileSink class
 *
 * Copyright (c) 2017 Hendrik Schwanekamp
 *
 */

#ifndef MPUTILS_FILESINK_H
#define MPUTILS_FILESINK_H

// includes
//--------------------
#include <fstream>
#include <experimental/filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include "Log.h"
//--------------------

// namespace
//--------------------
namespace mpu {
//--------------------

//-------------------------------------------------------------------
/**
 * class FileSink
 *
 * usage:
 * create an instance of file sink and pass it to the log class to log messages to a file.
 * Use maxFileSize and numLogsToKeep to enable logrotation. If maxFileSize is 0 Logs will not be rotated.
 * set printPlaintexts = false to ignore log messages that print plain text
 *
 */
class FileSink
{
public:
    FileSink(std::string sFilename, std:: size_t maxFileSize = 0, int numLogsToKeep = 1, bool printPlaintexts=true); // filesize in bytes
    void operator()(const LogMessage &msg);

private:
    std::ofstream file;
    void rotateLog();

    std::size_t maxFileSize; // max file size before log is rotated
    int iNumLogsToKeep; // number of old logs to keep
    std::string sLogfileName; // save the filename to manage old logfiles
    bool m_printPlaintexts; // whether or not plaintext messages should be printed to the file
};


}
#endif //MPUTILS_FILESINK_H
