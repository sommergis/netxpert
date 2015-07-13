#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include "config.h"
#include "utils.h"

using namespace std;

namespace netxpert {

    /**
    * \Static Class for logging
    **/
    class LOGGER
    {
        protected:
            LOGGER() {}
        public:
            ~LOGGER() {}
            static void Initialize(const Config& cnfg);
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
    };
}

#endif // LOGGER_H
