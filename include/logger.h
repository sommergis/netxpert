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

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include "config.h"
#include "utils.h"

namespace netxpert {
    namespace utils {

    /**
    * \Static Class for logging
    **/
    class LOGGER
    {
        protected:
            LOGGER() {}
        public:
            ~LOGGER() {}
            static void Initialize(const netxpert::cnfg::Config& cnfg);
            static void LogDebug(std::string logMsg);
            static void LogInfo(std::string logMsg);
            static void LogWarning(std::string logMsg);
            static void LogError(std::string logMsg);
            static void LogFatal(std::string logMsg);
            static bool IsInitialized;
            static std::string FullLogFileName;

        private:
            static netxpert::cnfg::Config NETXPERT_CNFG;
            static void writeLog(std::string logMsg, netxpert::cnfg::LOG_LEVEL);
            static std::string sTime;
            static std::string sPath;
            static std::string sFileName;
            static netxpert::cnfg::LOG_LEVEL applicationLogLevel;
            static std::string readConfig();
    };
} //namespace utils
}//namespace netxpert

#endif // LOGGER_H
