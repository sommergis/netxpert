#include "logger.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace netxpert;

//Init static member variables must be out of class scope!
string LOGGER::FullLogFileName = "";
bool LOGGER::IsInitialized = false;
string LOGGER::sPath = "";
string LOGGER::sFileName = "";
string LOGGER::sTime = "";
LOG_LEVEL LOGGER::applicationLogLevel = LOG_LEVEL::All;
Config LOGGER::NETXPERT_CNFG;

void LOGGER::Initialize(const Config& cnfg)
{
    NETXPERT_CNFG = cnfg;
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
        FullLogFileName = sPath + to_string(boost::filesystem::path::preferred_separator)
            + sTime + "_" + sFileName + ".log";
    }

    cout << "Logfile is: " << FullLogFileName << endl;

    ofstream outfile(FullLogFileName.c_str());
    IsInitialized = true;
}
string LOGGER::getDirectoryName(string _filePath) {
    path p(_filePath);
    //cout << "path p: " << p.string() << endl;
    path dir = p.parent_path();
    //cout << "path dir: " << dir.string() << endl;
    return dir.string();
}
string LOGGER::getFileNameWithoutExtension(string _filePath) {
    path p(_filePath);
    path fileNameWithoutExt = p.filename().stem();
    return fileNameWithoutExt.string();
}
string LOGGER::readConfig()
{
    applicationLogLevel = NETXPERT_CNFG.LogLevel;
    cout << "App Log Level: "<< applicationLogLevel <<endl;
    cout << "Logfile: "<< NETXPERT_CNFG.LogFileFullPath <<endl;
    return NETXPERT_CNFG.LogFileFullPath;
    //return "/home/hahne/dev/netxpert/NetXpert/bin/Debug/NetXpert.log";
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
    writeLog(totalMsg, Debug);
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
    writeLog(totalMsg, Info);

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
    writeLog(totalMsg, Warning);
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
    writeLog(totalMsg, Error);
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
    writeLog(totalMsg, Fatal);
}
