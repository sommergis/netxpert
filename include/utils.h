#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <stdio.h> /* defines FILENAME_MAX */
#include <iostream>
#include <vector>
#include <sstream>

#ifdef WINDOWS
    #include <direct.h>
    #include <windows.h>
    #include "Shlwapi.h"
    #define getCurrentDir _getcwd
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define getCurrentDir getcwd
 #endif

namespace netxpert {

    /**
    * \Static Static Class that provides utility functions
    **/
    class UTILS
    {
        protected:
            UTILS(){}
        public:
            static bool PathExists(const std::string& path);
            static bool FileExists(const std::string& path);
            static std::string GetCurrentDir();
            static bool SetCurrentDir(const std::string& path);
            static std::string GetDirectoryName(std::string& _filePath);
            static std::string GetFileNameWithoutExtension(std::string& _filePath);
            static std::vector<std::string>& Split(const std::string &s, char delim, std::vector<std::string> &elems);
            static std::vector<std::string> Split(const std::string &s, char delim);
    };
}
#endif // UTILS_H
