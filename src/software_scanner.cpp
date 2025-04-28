#include "software_scanner.h"
#include <algorithm>
#include <shlwapi.h>
#include <strsafe.h>
#include <filesystem>
#include <sstream>
#pragma comment(lib, "shlwapi.lib")

SoftwareScanner::SoftwareScanner() {
}

SoftwareScanner::~SoftwareScanner() {
}

std::vector<SoftwareInfo> SoftwareScanner::scanForAdobeSoftware() {
    std::vector<SoftwareInfo> adobeSoftware;
    
    // Scan registry for installed software
    std::vector<SoftwareInfo> registrySoftware = scanRegistry();
    adobeSoftware.insert(adobeSoftware.end(), registrySoftware.begin(), registrySoftware.end());
    
    // Search in common installation directories
    std::vector<SoftwareInfo> directoryScannedSoftware = searchInCommonLocations();
    
    // Add non-duplicate entries
    for (const auto& software : directoryScannedSoftware) {
        bool isDuplicate = false;
        for (const auto& existingSoftware : adobeSoftware) {
            if (existingSoftware.path == software.path) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate) {
            adobeSoftware.push_back(software);
        }
    }
    
    return adobeSoftware;
}

std::vector<SoftwareInfo> SoftwareScanner::scanRegistry() {
    std::vector<SoftwareInfo> software;
    
    // Registry paths for installed software
    const WCHAR* registryPaths[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };
    
    for (const auto& regPath : registryPaths) {
        HKEY hUninstallKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hUninstallKey) == ERROR_SUCCESS) {
            DWORD index = 0;
            WCHAR keyName[256];
            DWORD keyNameSize = _countof(keyName);
            
            while (RegEnumKeyEx(hUninstallKey, index++, keyName, &keyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                HKEY hAppKey;
                std::wstring subKeyPath = regPath;
                subKeyPath += L"\\";
                subKeyPath += keyName;
                
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath.c_str(), 0, KEY_READ, &hAppKey) == ERROR_SUCCESS) {
                    WCHAR displayName[512] = { 0 };
                    WCHAR installLocation[MAX_PATH] = { 0 };
                    WCHAR displayVersion[128] = { 0 };
                    DWORD dataSize;
                    
                    dataSize = sizeof(displayName);
                    RegQueryValueEx(hAppKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &dataSize);
                    
                    dataSize = sizeof(installLocation);
                    RegQueryValueEx(hAppKey, L"InstallLocation", NULL, NULL, (LPBYTE)installLocation, &dataSize);
                    
                    dataSize = sizeof(displayVersion);
                    RegQueryValueEx(hAppKey, L"DisplayVersion", NULL, NULL, (LPBYTE)displayVersion, &dataSize);
                    
                    if (displayName[0] != '\0' && isAdobeProduct(displayName)) {
                        SoftwareInfo info;
                        info.name = displayName;
                        info.version = displayVersion;
                        info.path = getExecutablePath(installLocation);
                        
                        if (!info.path.empty()) {
                            info.isBlocked = checkIfBlocked(info.path);
                            software.push_back(info);
                        }
                    }
                    
                    RegCloseKey(hAppKey);
                }
                
                keyNameSize = _countof(keyName);
            }
            
            RegCloseKey(hUninstallKey);
        }
    }
    
    return software;
}

std::vector<SoftwareInfo> SoftwareScanner::searchInCommonLocations() {
    std::vector<SoftwareInfo> software;
    
    // Common installation directories for Adobe products
    std::vector<std::wstring> directories = {
        L"C:\\Program Files\\Adobe",
        L"C:\\Program Files (x86)\\Adobe",
        L"C:\\Program Files\\Common Files\\Adobe",
        L"C:\\Program Files (x86)\\Common Files\\Adobe"
    };
    
    for (const auto& directory : directories) {
        WIN32_FIND_DATA findData;
        std::wstring searchPath = directory + L"\\*";
        HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) continue;
        
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || 
                wcscmp(findData.cFileName, L".") == 0 || 
                wcscmp(findData.cFileName, L"..") == 0) {
                continue;
            }
            
            std::wstring productDir = directory + L"\\" + findData.cFileName;
            
            // Search for .exe files
            WIN32_FIND_DATA exeData;
            std::wstring exeSearchPath = productDir + L"\\*.exe";
            HANDLE hExeFind = FindFirstFile(exeSearchPath.c_str(), &exeData);
            
            if (hExeFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(exeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::wstring exePath = productDir + L"\\" + exeData.cFileName;
                        
                        if (isAdobeProduct(exeData.cFileName)) {
                            SoftwareInfo info;
                            info.name = L"Adobe " + std::wstring(findData.cFileName) + L" (" + exeData.cFileName + L")";
                            info.path = exePath;
                            info.version = L"Unknown";
                            info.isBlocked = checkIfBlocked(exePath);
                            software.push_back(info);
                        }
                    }
                } while (FindNextFile(hExeFind, &exeData));
                
                FindClose(hExeFind);
            }
            
            // Search one level deeper
            WIN32_FIND_DATA subDirData;
            std::wstring subDirSearchPath = productDir + L"\\*";
            HANDLE hSubDirFind = FindFirstFile(subDirSearchPath.c_str(), &subDirData);
            
            if (hSubDirFind != INVALID_HANDLE_VALUE) {
                do {
                    if ((subDirData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
                        wcscmp(subDirData.cFileName, L".") != 0 && 
                        wcscmp(subDirData.cFileName, L"..") != 0) {
                        
                        std::wstring subProductDir = productDir + L"\\" + subDirData.cFileName;
                        
                        // Search for .exe files in subdirectory
                        WIN32_FIND_DATA subExeData;
                        std::wstring subExeSearchPath = subProductDir + L"\\*.exe";
                        HANDLE hSubExeFind = FindFirstFile(subExeSearchPath.c_str(), &subExeData);
                        
                        if (hSubExeFind != INVALID_HANDLE_VALUE) {
                            do {
                                if (!(subExeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                                    std::wstring exePath = subProductDir + L"\\" + subExeData.cFileName;
                                    
                                    if (isAdobeProduct(subExeData.cFileName)) {
                                        SoftwareInfo info;
                                        info.name = L"Adobe " + std::wstring(findData.cFileName) + L" " + 
                                                 subDirData.cFileName + L" (" + subExeData.cFileName + L")";
                                        info.path = exePath;
                                        info.version = L"Unknown";
                                        info.isBlocked = checkIfBlocked(exePath);
                                        software.push_back(info);
                                    }
                                }
                            } while (FindNextFile(hSubExeFind, &subExeData));
                            
                            FindClose(hSubExeFind);
                        }
                    }
                } while (FindNextFile(hSubDirFind, &subDirData));
                
                FindClose(hSubDirFind);
            }
        } while (FindNextFile(hFind, &findData));
        
        FindClose(hFind);
    }
    
    return software;
}

bool SoftwareScanner::isAdobeProduct(const std::wstring& name) {
    std::wstring lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return lowerName.find(L"adobe") != std::wstring::npos || 
           lowerName.find(L"photoshop") != std::wstring::npos || 
           lowerName.find(L"illustrator") != std::wstring::npos || 
           lowerName.find(L"acrobat") != std::wstring::npos || 
           lowerName.find(L"after effects") != std::wstring::npos || 
           lowerName.find(L"premiere") != std::wstring::npos || 
           lowerName.find(L"indesign") != std::wstring::npos || 
           lowerName.find(L"dreamweaver") != std::wstring::npos ||
           lowerName.find(L"lightroom") != std::wstring::npos ||
           lowerName.find(L"bridge") != std::wstring::npos ||
           lowerName.find(L"flash") != std::wstring::npos ||
           lowerName.find(L"media encoder") != std::wstring::npos;
}

std::wstring SoftwareScanner::getExecutablePath(const std::wstring& installPath) {
    if (installPath.empty()) return L"";
    
    WIN32_FIND_DATA findData;
    std::wstring searchPath = installPath + L"\\*.exe";
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) return L"";
    
    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (isAdobeProduct(findData.cFileName)) {
                std::wstring exePath = installPath + L"\\" + findData.cFileName;
                FindClose(hFind);
                return exePath;
            }
        }
    } while (FindNextFile(hFind, &findData));
    
    // If no Adobe-specific executable found, return the first .exe
    FindClose(hFind);
    hFind = FindFirstFile(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring exePath = installPath + L"\\" + findData.cFileName;
                FindClose(hFind);
                return exePath;
            }
        } while (FindNextFile(hFind, &findData));
        
        FindClose(hFind);
    }
    
    return L"";
}

bool SoftwareScanner::checkIfBlocked(const std::wstring& path) {
    // Run netsh command to check if application is blocked by firewall
    WCHAR command[1024];
    StringCchPrintf(command, _countof(command), 
                   L"netsh advfirewall firewall show rule name=all | findstr /C:\"%s\"", 
                   path.c_str());
    
    // Create pipes for reading the output
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }
    
    // Make sure the read handle is not inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
    
    // Set up the start info structure
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    // Create the child process
    if (!CreateProcess(NULL, command, NULL, NULL, TRUE, 
                      CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }
    
    // Close the write end of the pipe
    CloseHandle(hWritePipe);
    
    // Read the output of the command
    char buffer[4096] = {0};
    DWORD bytesRead;
    std::string output;
    
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead != 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    
    // Close handles
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // Check if the path is in any rule
    std::wstring pathWithBackslashes = path;
    // Replace forward slashes with backslashes
    std::replace(pathWithBackslashes.begin(), pathWithBackslashes.end(), L'/', L'\\');
    
    // Convert to lowercase for case-insensitive comparison
    std::wstring lowerPath = pathWithBackslashes;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    
    // Convert output to wstring and lowercase for comparison
    std::wstring woutput(output.begin(), output.end());
    std::transform(woutput.begin(), woutput.end(), woutput.begin(), ::tolower);
    
    return woutput.find(lowerPath) != std::wstring::npos;
}
