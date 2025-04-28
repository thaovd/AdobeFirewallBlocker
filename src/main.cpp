#include "mainwindow.h"
#include <windows.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

bool isRunningAsAdmin() {
    bool result = false;
    HANDLE token = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            result = elevation.TokenIsElevated != 0;
        }
    }
    
    if (token) {
        CloseHandle(token);
    }
    
    return result;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    // Initialize common controls
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&iccex);
    
    // Check if running with admin privileges
    if (!isRunningAsAdmin()) {
        MessageBox(NULL,
                  L"This application requires administrator privileges to manage Windows Firewall rules.\n\n"
                  L"Please right-click on the application and select \"Run as administrator\".",
                  L"Administrator Privileges Required",
                  MB_OK | MB_ICONWARNING);
        return 1;
    }
    
    // Initialize main window
    MainWindow mainWindow;
    if (!mainWindow.initialize(hInstance)) {
        return 1;
    }
    
    // Show the main window
    mainWindow.show();
    
    // Standard message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(NULL, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return (int)msg.wParam;
}
