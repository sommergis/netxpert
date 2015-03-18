#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include "config.h"

using namespace std;

namespace NetXpert {

    /**
    * \Static Class for logging
    **/
    class LOGGER
    {
        protected:
            LOGGER() {}
        public:
            ~LOGGER() {}
            static void Initialize(Config& cnfg);
            static void LogDebug(string logMsg);
            static void LogInfo(string logMsg);
            static void LogWarning(string logMsg);
            static void LogError(string logMsg);
            static void LogFatal(string logMsg);
            static bool IsInitialized;
            static string FullLogFileName;

        private:
            static Config NETXPERT_CNFG;
            static void writeLog(string logMsg, LOG_LEVEL);
            static string sTime;
            static string sPath;
            static string sFileName;
            static LOG_LEVEL applicationLogLevel;
            static string readConfig();
            static string getDirectoryName(string fileName);
            static string getFileNameWithoutExtension(string fileName);
    };
}

#endif // LOGGER_H
