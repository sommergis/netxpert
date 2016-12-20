#ifndef UTILS_H
#define UTILS_H

#include <omp.h>
#include <string>
#include <stdio.h> /* defines FILENAME_MAX */
#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/vector.hpp"

#if (defined _WIN32 || _WIN64)
	#include <locale>
	#include <codecvt>
    #include <direct.h>
    //Fix error with min/max on windows
    #define NOMINMAX
    #include <windows.h>
    #undef NOMINMAX
    #include "Shlwapi.h"
    #define getCurrentDir _getcwd
#endif // _WIN

#ifdef __linux__
    #include <unistd.h>
    #include <sys/stat.h>
    #define getCurrentDir getcwd
#endif

namespace netxpert {
    namespace utils {

    template<typename TimeT = std::chrono::microseconds,
        typename ClockT=std::chrono::high_resolution_clock,
        typename DurationT=double>
    class Stopwatch
    {
    private:
        std::chrono::time_point<ClockT> _start, _end;
    public:
        Stopwatch() { start(); }
        void start() { _start = _end = ClockT::now(); }
        DurationT stop() { _end = ClockT::now(); return elapsed();}
        DurationT elapsed() {
            auto delta = std::chrono::duration_cast<TimeT>(_end-_start);
            return delta.count();
        }
    };

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
            template<typename T>
            static T DeserializeJSONtoObject(std::string _jsonString)
            {
                std::stringstream ss (_jsonString);
                cereal::JSONInputArchive archive ( ss );
                T outData;
                archive( outData );
                return outData;
            }
            template<typename T>
            static std::string SerializeObjectToJSON(T _inData, std::string _rootNodeName="input")
            {
                std::stringstream ss;
                cereal::JSONOutputArchive archive ( ss );
                archive( cereal::make_nvp(_rootNodeName,_inData) );
                std::string ret = ss.str();
                return ret;
            }
            static std::wstring convertStringToWString(const std::string &s);
            static std::string convertWStringToString (const std::wstring &ws);
            static std::string Replace(std::string& str, const std::string& from, const std::string& to);
            static std::string ReplaceAll(std::string& str, const std::string& from, const std::string& to);
            static bool IsEqual (const std::string& a, const std::string& b) { return a == b; }
    };
} //namespace utils
}//namespace netxpert
#endif // UTILS_H
