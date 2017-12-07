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

    ///\brief A simple stopwatch
    template<typename TimeT = std::chrono::microseconds,
        typename ClockT=std::chrono::high_resolution_clock,
        typename DurationT=double>
    class Stopwatch
    {
    private:
        std::chrono::time_point<ClockT> _start, _end;
    public:
        ///\brief Constructor
        /// Calls start() method on initialization.
        Stopwatch() { start(); }
        ///\brief starts timer of the stopwatch
        void start() { _start = _end = ClockT::now(); }
        ///\brief stops the timer of the stopwatch
        /// Calls elapsed() method
        ///\return elapsed time
        DurationT stop() { _end = ClockT::now(); return elapsed();}
        ///\brief Gets the elapsed amount of time since the start() method was called
        DurationT elapsed() {
            auto delta = std::chrono::duration_cast<TimeT>(_end-_start);
            return delta.count();
        }
    };

    ///\brief Static Class that provides utility functions
    class UTILS
    {
        protected:
            ///\brief Empty protected Destructor because of static class
            UTILS(){}
        public:
            ///\brief Returns true if the given (directory) path exists
            static bool PathExists(const std::string& path);
            ///\brief Returns true if the given file exists
            static bool FileExists(const std::string& path);
            ///\brief Gets the current working directory
            static std::string GetCurrentDir();
            ///\brief Sets the current working directory
            static bool SetCurrentDir(const std::string& path);
            ///\brief Gets the directory name from the given path
            static std::string GetDirectoryName(std::string& _filePath);
            ///\brief Gets the file name without its extension from the given path
            static std::string GetFileNameWithoutExtension(std::string& _filePath);
            ///\brief Splits the string with the given delimter into a vector
            static std::vector<std::string>& Split(const std::string &s, char delim, std::vector<std::string> &elems);
            ///\brief Splits the given string with the given delimter into a vector
            static std::vector<std::string> Split(const std::string &s, char delim);
            ///\brief Deserializes a JSON string to a object of type T
            template<typename T>
            static T DeserializeJSONtoObject(std::string _jsonString)
            {
              try {
                std::stringstream ss (_jsonString);
                cereal::JSONInputArchive archive ( ss );
                T outData;
                archive( outData );
                return outData;
              }
              catch (cereal::RapidJSONException& ex) {
                std::cout << "Error reading config!" << std::endl;
                std::terminate();
              }
            }
            ///\brief Serializes an object of type T to JSON string
            template<typename T>
            static std::string SerializeObjectToJSON(T _inData, std::string _rootNodeName="input")
            {
                std::stringstream ss;
                cereal::JSONOutputArchive archive ( ss );
                archive( cereal::make_nvp(_rootNodeName,_inData) );
                std::string ret = ss.str();
                return ret;
            }
            ///\brief Converts a std::string to a std::wstring
            static std::wstring convertStringToWString(const std::string &s);
            ///\brief Converts a std::wstring to a std::string
            static std::string convertWStringToString (const std::wstring &ws);
            ///\brief Replace occurence (from) in a string with a new value (to)
            static std::string Replace(std::string& str, const std::string& from, const std::string& to);
            ///\brief Replace every occurence (from) in a string with a new value (to)
            static std::string ReplaceAll(std::string& str, const std::string& from, const std::string& to);
            ///\brief Returns true if the given strings are equal
            static bool IsEqual (const std::string& a, const std::string& b) { return a == b; }
    };
} //namespace utils
}//namespace netxpert
#endif // UTILS_H
