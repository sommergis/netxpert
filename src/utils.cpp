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
    #ifdef _WIN32, _WIN64
    if (_chdir(newPath) == 0)
    #endif // _WIN
    #ifdef __linux__
    if (chdir(newPath) == 0)
    #endif // __linux__
        return true;
    else
        return false;
}

bool UTILS::FileExists(const std::string& path)
{
	#ifdef _WIN32, _WIN64
		#ifndef _UNICODE
			LPCSTR path3 = path.c_str();
			return PathFileExists(path3);
		#endif
		#ifdef _UNICODE
			std::wstring path2 = convertStringToWString(path);
			//Funktioniert nur mit UNICODE direktive
			LPCWSTR path3 = path2.c_str();
			return PathFileExists(path3);
		#endif
	#endif

    #ifdef __linux__
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
    #endif //__linux__
}

bool UTILS::PathExists(const std::string& path)
{
	#ifdef _WIN32, _WIN64
	return FileExists(path);
	#endif
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
std::wstring UTILS::convertStringToWString(const std::string& str)
{
	const std::ctype<wchar_t>& CType = std::use_facet<std::ctype<wchar_t> >(std::locale());
	std::vector<wchar_t> wideStringBuffer(str.length());
	CType.widen(str.data(), str.data() + str.length(), &wideStringBuffer[0]);

	return std::wstring(&wideStringBuffer[0], wideStringBuffer.size());
}

std::string UTILS::convertWStringToString(const std::wstring& str)
{
	const std::ctype<wchar_t>& CType = std::use_facet<std::ctype<wchar_t> >(std::locale());
	std::vector<char> stringBuffer(str.length());
	char * pc = new char[str.length() + 1];
	//CType.narrow(str.begin(), str.end(),"?", pc);
	CType.narrow(str.c_str(), str.c_str() + str.length() + 1, '?', pc);

	//return std::string(&stringBuffer[0], stringBuffer.size());
	return std::string(pc, str.length() + 1);
}
/*
std::wstring UTILS::convertStringToWString(const std::string &s)
{
#ifdef _WIN32, _WIN64
	#ifdef _UNICODE
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(s);
	#else
		std::wstring wsTmp(s.begin(), s.end());
		return wsTmp;
	#endif
#else
    std::wstring wsTmp(s.begin(), s.end());
	return wsTmp;
#endif
}

std::string UTILS::convertWStringToString (const std::wstring &ws)
{
#ifdef _WIN32, _WIN64
	#ifdef _UNICODE
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(ws);
	#else
		std::string sTmp(ws.begin(), ws.end());
		return sTmp;
	#endif
#else
	std::string sTmp(ws.begin(), ws.end());
	return sTmp;
#endif
}

std::wstring UTILS::convertStringToWString(const std::string& str)
{
    typedef std::codecvt_utf8_utf16<wchar_t> convert_typeX;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}
std::string UTILS::convertWStringToString(const std::wstring& wstr)
{
	typedef std::codecvt_utf16<wchar_t> convert_typeX;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}*/

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
