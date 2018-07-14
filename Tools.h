#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>
#include <string>

#include <Config.h>
#include <sha256.h>

#include <vector>
#include <algorithm>
#include <iterator>

/*
    Some codes used from http://www.martinbroadhurst.com/list-the-files-in-a-directory-in-c.html
*/
#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <sys/types.h>
#include <dirent.h>

class ReadDirResult {
    public:
        std::string Name;
        bool IsDir;

        ReadDirResult() = default;
        ReadDirResult(std::string & name, bool isDir) : Name(std::move(name)), IsDir(isDir) { }
        ReadDirResult(std::string && name, bool isDir) : Name(std::move(name)), IsDir(isDir) { }

        ReadDirResult(const ReadDirResult& other)
            : Name(std::move(other.Name))
            , IsDir(other.IsDir)
        { }

        // Copy assignment operator.
        ReadDirResult& operator=(const ReadDirResult& other)
        {
            if (this != &other)
            {
                Name = std::move(other.Name);
                IsDir = other.IsDir;
            }
            return *this;
        }
};

static void read_directory(const std::string& name, std::vector<ReadDirResult>& v)
{
    DIR* dirp = opendir(name.c_str());
    struct dirent * dp;

    while ((dp = readdir(dirp)) != NULL) {
        v.push_back(ReadDirResult(std::string(dp->d_name), dp->d_type == DT_DIR));
    }
    closedir(dirp);
}
#else
#include <windows.h>

// https://stackoverflow.com/questions/12361140/win32-find-data-get-the-absolute-path
static void read_directory(const std::string& name, std::vector<ReadDirResult>& v)
{
    std::string pattern(name);
    pattern.append("\\*");
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
        do {
            v.push_back(data.cFileName);
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
}
#endif

static long int getTimestamp()
{
    time_t t = std::time(0);
    long int now = static_cast<long int> (t);
    return now;
}

struct MineInfo {
        size_t Nounce;
        std::string Hash;
};


inline static bool isInteger(char const* str)
{
    return strlen(str) != 0 && strspn(str, "0123456789") == strlen(str);
}

struct AddressPort {
        AddressPort(std::string& data)
        {
            std::stringstream ss(data);
            std::string item;
            std::vector<std::string> tokens;
            while (std::getline(ss, item, ':'))
                tokens.push_back(item);

            if (tokens.size() == 2 && isInteger(tokens[1].c_str()))
            {
                Address = tokens[0];
                Port = (size_t)std::stoi(tokens[1]);
                Success = true;
            }
        }

        bool Success;
        std::string Address;
        size_t Port;
};
