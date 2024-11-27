#ifndef FS_UTILS_H
#define FS_UTILS_H

#include <iostream>
#include <fstream>
#include <filesystem>

std::string StrFromFile(std::string tPath) {
    std::filesystem::path path(tPath);
    if(!std::filesystem::exists(path)) {
        LOG_WARN("File at \"" + tPath + "\" doesn't seem to exist.");
        return "File at \"" + tPath + "\" doesn't seem to exist.";
    }
    // Open the stream to 'lock' the file.
    std::ifstream f(path, std::ios::in | std::ios::binary);
    // Obtain the size of the file.
    const auto sz = std::filesystem::file_size(path);
    // Create a buffer.
    std::string result(sz, '\0');
    // Read the whole file into the buffer.
    f.read(result.data(), sz);
    return result;
}

void StrToFile(std::string tPath, std::string tIn) {
    std::ofstream file;
    file.open(tPath);
    file << tIn << std::endl;
    file.close();
}

int StrEndsWith(const char* tStr, const char* tsuffix) {
    if (!tStr || !tsuffix)
        return 0;
    size_t lenstr = strlen(tStr);
    size_t lensuffix = strlen(tsuffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(tStr + lenstr - lensuffix, tsuffix, lensuffix) == 0;
}

// !!! NOT UTF8 !!!
// Use StrToLower instead.
std::string StrASCIIToLower(std::string tStr) {
    std::transform(tStr.begin(), tStr.end(), tStr.begin(),
        [](unsigned char c) {
            return std::tolower(c);
        });
    return tStr;
}
// !!! NOT UTF8 !!!
// Use StrToUpper instead.
std::string StrASCIIToUpper(std::string tStr) {
    std::transform(tStr.begin(), tStr.end(), tStr.begin(),
        [](unsigned char c) {
            return std::toupper(c);
        });
    return tStr;
}

glm::vec3 float3ToVec3(float* tF) {
    return glm::vec3(tF[0], tF[1], tF[2]);
}

static const std::string currentDateTime() {
    struct tm newtime;
    __time64_t long_time;
    char timebuf[26];
    errno_t err;

    // Get time as 64-bit integer.
    _time64(&long_time);
    // Convert to local time.
    err = _localtime64_s(&newtime, &long_time);
    if (err) {
        LOG_WARN("Invalid argument to _localtime64_s.");
        return "invalid";
    }
    strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %X", &newtime);
    return timebuf;
}

#include "UtfConv.hpp"
std::string StrToLower(std::string tStr) {
    return reinterpret_cast<const char*>(Utf8StrMakeLwrUtf8Str(reinterpret_cast<const unsigned char*>(tStr.c_str())));
}

std::string StrToUpper(std::string tStr) {
    return reinterpret_cast<const char*>(Utf8StrMakeUprUtf8Str(reinterpret_cast<const unsigned char*>(tStr.c_str())));
}

#include "../common.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.hpp"

/// Loads texture from given file.
unsigned int TextureFromFile(const std::string& tPath, bool tGamma = false) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(tPath.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = 0;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            tGamma ? format = GL_RGB : format = GL_SRGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
        LOG_ERRR("Texture failed to load at path: " + tPath);
    stbi_image_free(data);
    return textureID;
}

/// Dialog for file (open/save).
struct FileDialog {
    // Deafult return value (if file isn't gotten).
    std::string default_file = "";
    // Path to start in.
    const char* path = NULL;
    // Extension filter.
    const char* filter = "All\0*.*\0";
    // Index of filter's item.
    int filter_id = 0;
    // Opens file dialog and gets file name. Otherwise returns set default.
    std::string open() const;
    // Opens file dialog and gets file name. Otherwise returns set default.
    std::string save() const;
};

#ifdef WIN32
#include <windows.h>
static void openURL(const char* tUrl) {
    ShellExecuteA(NULL, "open", tUrl, NULL, NULL, SW_SHOWNORMAL);
}
std::string FileDialog::open() const {
    OPENFILENAME ofn;       //Common dialog box structure.
    char szFile[MAX_PATH]{};     //Buffer for file name.
    HWND hwnd = nullptr;    //Owner window.
    HANDLE hf = nullptr;    //File handle.
    //Initialize OPENFILENAME.
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    //Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = filter_id;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = path;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    //Display the Open dialog box. 
    if (GetOpenFileName(&ofn) == TRUE)
        return szFile;
    return default_file;
}
std::string FileDialog::save() const {
    OPENFILENAME ofn;       //Common dialog box structure.
    char szFile[MAX_PATH]{};     //Buffer for file name.
    HWND hwnd = nullptr;    //Owner window.
    HANDLE hf = nullptr;    //File handle.
    //Initialize OPENFILENAME.
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    //Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = filter_id;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = path;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    //Display the Open dialog box. 
    if (GetSaveFileName(&ofn) == TRUE)
        return szFile;
    return default_file;
}
#endif // !WIN32

#endif // !FS_UTILS_H