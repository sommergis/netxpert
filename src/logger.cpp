#include "logger.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace NetXpert;

//Init static member variables must be out of class scope!
string LOGGER::FullLogFileName = "";
bool LOGGER::IsInitialized = false;
string LOGGER::sPath = "";
string LOGGER::sFileName = "";
string LOGGER::sTime = "";
LOG_LEVEL LOGGER::applicationLogLevel = LOG_LEVEL::All;
Config LOGGER::NetXpertConfig;

void LOGGER::Initialize()
{
    string fileName = readConfig();
    //cout << "Log File Path: " << fileName << endl;
    sPath = getDirectoryName(fileName);
    //cout << sPath << endl;

    sFileName = getFileNameWithoutExtension(fileName);
    //cout << sFileName << endl;

    //this variable used to create log filename format "
    //for example filename : ErrorLogYYYYMMDD
    time_facet *facet = new time_facet("%Y%m%d_%H%M%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();

    sTime = ss.str();
    //cout << sTime << endl;

    //Create File - do not append
    FullLogFileName = sPath + boost::filesystem::path::preferred_separator
     + sTime + "_" + sFileName + ".log";
    cout << "Logfile is: " << FullLogFileName << endl;

    ofstream outfile(FullLogFileName.c_str());
    IsInitialized = true;
}
string LOGGER::getDirectoryName(string _filePath) {
    path p(_filePath);
    path dir = p.parent_path();
    return dir.string();
}
string LOGGER::getFileNameWithoutExtension(string _filePath) {
    path p(_filePath);
    path fileNameWithoutExt = p.filename().stem();
    return fileNameWithoutExt.string();
}
string LOGGER::readConfig()
{
    applicationLogLevel = NetXpertConfig.LogLevel;
    cout << "App Log Level: "<< applicationLogLevel <<endl;
    cout << "Logfile: "<< NetXpertConfig.LogFileFullPath <<endl;
    return NetXpertConfig.LogFileFullPath;
    //return "/home/hahne/dev/netxpert/NetXpert/bin/Debug/NetXpert.log";
}
void LOGGER::writeLog(string _logMsg, LOG_LEVEL msgLogLevel)
{
    //cout << "Log Level: " << LOG_LEVEL << endl;
    if (msgLogLevel >= applicationLogLevel)
    {
        // appends every log msg to file
        ofstream outfile;
        outfile.open(FullLogFileName.c_str(), ios::out | ios::app);
        outfile << _logMsg << endl;
    }
}
void LOGGER::LogDebug(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " DEBUG " + _logMsg;
    cout << totalMsg << endl;
    writeLog(totalMsg, Debug);
}

void LOGGER::LogInfo(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " INFO " + _logMsg;
    cout << totalMsg << endl;
    writeLog(totalMsg, Info);
}
void LOGGER::LogWarning(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " WARN " + _logMsg;
    cout << totalMsg << endl;
    writeLog(totalMsg, Warning);
}

void LOGGER::LogError(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " ERROR " + _logMsg;
    cout << totalMsg << endl;
    writeLog(totalMsg, Error);
}

void LOGGER::LogFatal(string _logMsg)
{
    //format with time
    time_facet *facet = new time_facet("%Y-%m-%d %H:%M:%S");
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << second_clock::local_time();
    string timeStr = ss.str();
    string totalMsg = timeStr + " FATAL " + _logMsg;
    cout << totalMsg << endl;
    writeLog(totalMsg, Fatal);
}
