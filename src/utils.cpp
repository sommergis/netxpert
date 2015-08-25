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
    #ifdef _WIN32
    std::wstring path2;
    StringToWString(path2, path);
    LPCWSTR path3 = path2.c_str();
    return PathFileExists(path3);
    #endif // _WIN32
    #ifdef _WIN64
    std::wstring path2;
    StringToWString(path2, path);
    LPCWSTR path3 = path2.c_str();
    return PathFileExists(path3);
    #endif // _WIN64
    #ifdef __linux__
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
    #endif //__linux__
}

bool UTILS::PathExists(const std::string& path)
{
    #ifdef _WIN32
    std::wstring path2;
    StringToWString(path2, path);
    LPCWSTR path3 = path2.c_str();
    return PathFileExists(path3);
    #endif // _WIN32
    #ifdef _WIN64
    std::wstring path2;
    StringToWString(path2, path);
    LPCWSTR path3 = path2.c_str();
    return PathFileExists(path3);
    #endif // _WIN64
    #ifdef __linux__
    struct stat st;
    bool exists = false;
    if(stat(path.c_str(),&st) == 0)
        if(st.st_mode & S_IFDIR != 0)
            exists = true;
    return exists;
    #endif //__linux__
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


/**
 *  Converts string to wstring
 */
int UTILS::StringToWString(std::wstring &ws, const std::string &s)
{
    std::wstring wsTmp(s.begin(), s.end());

    ws = wsTmp;

    return 0;
}
/**
 *  Converts wstring to string
 */
int UTILS::WStringToString (std::string &s, const std::wstring &ws)
{
    std::string sTmp(ws.begin(), ws.end());

    s = sTmp;

    return 0;
}

std::vector<std::string> UTILS::Split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    UTILS::Split(s, delim, elems);
    return elems;
}
std::string UTILS::Replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return "";
    return str.replace(start_pos, from.length(), to);
}
std::string UTILS::ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return "";
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
    return str;
}
