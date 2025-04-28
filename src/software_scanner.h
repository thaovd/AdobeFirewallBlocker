#ifndef SOFTWARE_SCANNER_H
#define SOFTWARE_SCANNER_H

#include <string>
#include <vector>
#include <windows.h>

struct SoftwareInfo {
    std::wstring name;
    std::wstring path;
    std::wstring version;
    bool isBlocked;
};

class SoftwareScanner {
public:
    SoftwareScanner();
    ~SoftwareScanner();

    std::vector<SoftwareInfo> scanForAdobeSoftware();

private:
    std::vector<SoftwareInfo> scanRegistry();
    std::vector<SoftwareInfo> searchInCommonLocations();
    bool isAdobeProduct(const std::wstring& name);
    std::wstring getExecutablePath(const std::wstring& installPath);
    bool checkIfBlocked(const std::wstring& path);
};

#endif // SOFTWARE_SCANNER_H
