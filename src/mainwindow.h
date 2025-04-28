#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include "software_scanner.h"
#include "firewall_manager.h"

// Define control IDs - must match resources.rc
#define IDC_SOFTWARE_LIST             1001
#define IDC_SCAN_BUTTON               1002
#define IDC_BLOCK_BUTTON              1003
#define IDC_UNBLOCK_BUTTON            1004
#define IDC_SELECT_ALL_BUTTON         1005
#define IDC_UNSELECT_ALL_BUTTON       1006
#define IDC_INBOUND_CHECK             1007
#define IDC_OUTBOUND_CHECK            1008
#define IDC_TITLE_LABEL               1009
#define IDC_PROGRESS_BAR              1010
#define IDC_STATUS_LABEL              1011
#define IDI_APP_ICON                  2000
#define IDD_MAIN_DIALOG               3000

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool initialize(HINSTANCE hInstance);
    void show();

private:
    // Window procedure
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    // UI manipulation methods
    void initDialog();
    void scanForAdobeSoftware();
    void refreshSoftwareList();
    void blockSelectedSoftware();
    void unblockSelectedSoftware();
    void selectAllSoftware();
    void unselectAllSoftware();
    void setStatusText(const std::wstring& text);
    void showProgressBar(bool show);
    void setProgressBarRange(int min, int max);
    void setProgressBarPos(int pos);
    void enableControls(bool enable);
    void showMessageBox(const std::wstring& title, const std::wstring& message, UINT type = MB_OK | MB_ICONINFORMATION);
    
    // UI controls
    HWND m_hwnd;
    HWND m_softwareList;
    HWND m_scanButton;
    HWND m_blockButton;
    HWND m_unblockButton;
    HWND m_selectAllButton;
    HWND m_unselectAllButton;
    HWND m_inboundCheck;
    HWND m_outboundCheck;
    HWND m_progressBar;
    HWND m_statusLabel;
    
    // Data
    HINSTANCE m_hInstance;
    SoftwareScanner m_scanner;
    FirewallManager m_firewallManager;
    std::vector<SoftwareInfo> m_detectedSoftware;
    
    // Static instance for the window procedure
    static MainWindow* s_instance;
};

#endif // MAINWINDOW_H
