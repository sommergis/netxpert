#include "utils.h"

using namespace netxpert;

std::string UTILS::GetCurrentDir()
{
    char cCurrentPath[FILENAME_MAX];
    // calls define "getCurrentDir" in utils.h
    if (!getCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
    {
        //return to_string(errno);
        return "";
    }
    else {
        return static_cast<std::string>(cCurrentPath);
    }
}

bool UTILS::SetCurrentDir(const std::string& path)
{
    const char* newPath = path.c_str();
    if (chdir(newPath) == 0)
        return true;
    else
        return false;
}

bool UTILS::FileExists(const std::string& path)
{
    #ifdef WINDOWS
    return PathFileExists(path.c_str());
    #else
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
    #endif
}

bool UTILS::PathExists(const std::string& path)
{
    #ifdef WINDOWS
    return PathFileExists(path.c_str());
    #else
    struct stat st;
    bool exists = false;
    if(stat(path.c_str(),&st) == 0)
        if(st.st_mode & S_IFDIR != 0)
            exists = true;
    return exists;
    #endif
}

std::string UTILS::GetDirectoryName(std::string& _filePath) {
    size_t found;
    found=_filePath.find_last_of("/\\");
    return _filePath.substr(0,found);
}

std::string UTILS::GetFileNameWithoutExtension(std::string& _filePath) {
    size_t found;
    found=_filePath.find_last_of("/\\");
    std::string filename = _filePath.substr(found+1);
    found=filename.find_last_of(".");
    return filename.substr(0,found);
}
std::vector<std::string>& UTILS::Split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> UTILS::Split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    UTILS::Split(s, delim, elems);
    return elems;
}
