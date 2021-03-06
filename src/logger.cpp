/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#include "logger.hpp"
#include <iostream>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::utils;

//Init static member variables must be out of class scope!
string LOGGER::FullLogFileName = "";
bool LOGGER::IsInitialized = false;
string LOGGER::sPath = "";
string LOGGER::sFileName = "";
string LOGGER::sTime = "";
LOG_LEVEL LOGGER::applicationLogLevel = LOG_LEVEL::LogAll;
Config LOGGER::NETXPERT_CNFG;

void LOGGER::Initialize(const Config& cnfg)
{
    NETXPERT_CNFG = cnfg;
    string fileName = readConfig();
    sPath = UTILS::GetDirectoryName(fileName);

    sFileName = UTILS::GetFileNameWithoutExtension(fileName);

    //this variable used to create log filename format "
    //for example filename : ErrorLogYYYYMMDD
    time_facet *facet = new time_facet("%Y%m%d_%H%M%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();

	sTime = ss.str();
#ifdef _WIN32
	//trim of first two chars
	sTime = sTime.substr(2, 16);
#endif
    //Create File - do not append
    if (sPath.empty()) // no directory specified in config
    {
        FullLogFileName = sTime + "_" + sFileName + ".log";
    }
    else
    {
        FullLogFileName = sPath + "/"
            + sTime + "_" + sFileName + ".log";
    }

    if (NETXPERT_CNFG.LogLevel != netxpert::cnfg::LOG_LEVEL::LogQuiet)
        cout << "Logfile is: " << FullLogFileName << endl;

    ofstream outfile(FullLogFileName.c_str());
    IsInitialized = true;
}

string LOGGER::readConfig()
{
    applicationLogLevel = NETXPERT_CNFG.LogLevel;
    if (applicationLogLevel != netxpert::cnfg::LOG_LEVEL::LogQuiet)
    {
        cout << "App Log Level: "<< applicationLogLevel <<endl;
        cout << "Logfile: "<< NETXPERT_CNFG.LogFileFullPath <<endl;
    }
    return NETXPERT_CNFG.LogFileFullPath;
}

void LOGGER::writeLog(string _logMsg, LOG_LEVEL msgLogLevel)
{
    // appends every log msg to file
    ofstream outfile;
    outfile.open(FullLogFileName.c_str(), ios::out | ios::app);
    if (msgLogLevel >= applicationLogLevel)
    {
        cout << _logMsg << endl;
        outfile << _logMsg << endl;
    }
}
void LOGGER::LogDebug(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S.%f");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << microsec_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " DEBUG " + _logMsg;
    writeLog(totalMsg, LOG_LEVEL::LogDebug);
}

void LOGGER::LogInfo(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S.%f");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << microsec_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " INFO " + _logMsg;
    writeLog(totalMsg, LOG_LEVEL::LogInfo);

}
void LOGGER::LogWarning(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S.%f");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << microsec_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " WARN " + _logMsg;
    writeLog(totalMsg, LOG_LEVEL::LogWarning);
}

void LOGGER::LogError(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S.%f");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << microsec_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " ERROR " + _logMsg;
    writeLog(totalMsg, LOG_LEVEL::LogError);
}

void LOGGER::LogFatal(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S.%f");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << microsec_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " FATAL " + _logMsg;
    writeLog(totalMsg, LOG_LEVEL::LogFatal);
}
