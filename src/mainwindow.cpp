#include "mainwindow.h"
#include <windows.h>
#include <commctrl.h>
#include <sstream>
#include <algorithm>
#include <strsafe.h>

// Initialize the static instance
MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow() : m_hwnd(NULL), m_hInstance(NULL)
{
    s_instance = this;
}

MainWindow::~MainWindow()
{
    s_instance = nullptr;
}

bool MainWindow::initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    
    // Create the main dialog window
    m_hwnd = CreateDialog(hInstance,
                         MAKEINTRESOURCE(IDD_MAIN_DIALOG),
                         NULL,
                         DialogProc);
    
    if (!m_hwnd) {
        DWORD error = GetLastError();
        WCHAR buffer[256];
        StringCchPrintf(buffer, _countof(buffer), L"Failed to create dialog: %u", error);
        MessageBox(NULL, buffer, L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Initialize the common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);
    
    // Store UI control handles
    m_softwareList = GetDlgItem(m_hwnd, IDC_SOFTWARE_LIST);
    m_scanButton = GetDlgItem(m_hwnd, IDC_SCAN_BUTTON);
    m_blockButton = GetDlgItem(m_hwnd, IDC_BLOCK_BUTTON);
    m_unblockButton = GetDlgItem(m_hwnd, IDC_UNBLOCK_BUTTON);
    m_selectAllButton = GetDlgItem(m_hwnd, IDC_SELECT_ALL_BUTTON);
    m_unselectAllButton = GetDlgItem(m_hwnd, IDC_UNSELECT_ALL_BUTTON);
    m_inboundCheck = GetDlgItem(m_hwnd, IDC_INBOUND_CHECK);
    m_outboundCheck = GetDlgItem(m_hwnd, IDC_OUTBOUND_CHECK);
    m_progressBar = GetDlgItem(m_hwnd, IDC_PROGRESS_BAR);
    m_statusLabel = GetDlgItem(m_hwnd, IDC_STATUS_LABEL);
    
    // Setup the list view
    ListView_SetExtendedListViewStyle(m_softwareList, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
    
    // Add columns to the list view
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 300;
    lvc.pszText = const_cast<LPWSTR>(L"Name");
    lvc.iSubItem = 0;
    ListView_InsertColumn(m_softwareList, 0, &lvc);
    
    lvc.cx = 200;
    lvc.pszText = const_cast<LPWSTR>(L"Version");
    lvc.iSubItem = 1;
    ListView_InsertColumn(m_softwareList, 1, &lvc);
    
    lvc.cx = 300;
    lvc.pszText = const_cast<LPWSTR>(L"Path");
    lvc.iSubItem = 2;
    ListView_InsertColumn(m_softwareList, 2, &lvc);
    
    // Set initial state of checkboxes
    SendMessage(m_inboundCheck, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_outboundCheck, BM_SETCHECK, BST_CHECKED, 0);
    
    // Hide progress bar initially
    ShowWindow(m_progressBar, SW_HIDE);
    
    // Disable buttons initially
    EnableWindow(m_blockButton, FALSE);
    EnableWindow(m_unblockButton, FALSE);
    EnableWindow(m_selectAllButton, FALSE);
    EnableWindow(m_unselectAllButton, FALSE);
    
    // Set initial status text
    setStatusText(L"Ready");
    
    return true;
}

void MainWindow::show() {
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
}

// Static dialog procedure
INT_PTR CALLBACK MainWindow::DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!s_instance) return FALSE;
    
    switch (message) {
        case WM_INITDIALOG:
            s_instance->initDialog();
            return TRUE;
        
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return TRUE;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return TRUE;
                
                case IDC_SCAN_BUTTON:
                    s_instance->scanForAdobeSoftware();
                    return TRUE;
                
                case IDC_BLOCK_BUTTON:
                    s_instance->blockSelectedSoftware();
                    return TRUE;
                
                case IDC_UNBLOCK_BUTTON:
                    s_instance->unblockSelectedSoftware();
                    return TRUE;
                
                case IDC_SELECT_ALL_BUTTON:
                    s_instance->selectAllSoftware();
                    return TRUE;
                
                case IDC_UNSELECT_ALL_BUTTON:
                    s_instance->unselectAllSoftware();
                    return TRUE;
            }
            break;
        
        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
                // Handle list item selection changes if needed
                NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                if (pnmv->uChanged & LVIF_STATE) {
                    // Update buttons based on selection if needed
                }
            }
            break;
    }
    
    return FALSE;
}

void MainWindow::initDialog() {
    // Additional initialization after dialog creation
    // Center the dialog on the screen
    RECT rcClient, rcDesktop;
    GetWindowRect(m_hwnd, &rcClient);
    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    
    int x = ((rcDesktop.right - rcDesktop.left) - (rcClient.right - rcClient.left)) / 2 + rcDesktop.left;
    int y = ((rcDesktop.bottom - rcDesktop.top) - (rcClient.bottom - rcClient.top)) / 2 + rcDesktop.top;
    
    SetWindowPos(m_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void MainWindow::scanForAdobeSoftware() {
    // Disable scan button during scan
    EnableWindow(m_scanButton, FALSE);
    setStatusText(L"Scanning for Adobe software...");
    showProgressBar(true);
    setProgressBarRange(0, 0); // Indeterminate progress
    
    // Clear the current list
    ListView_DeleteAllItems(m_softwareList);
    m_detectedSoftware.clear();
    
    // Perform the scan
    m_detectedSoftware = m_scanner.scanForAdobeSoftware();
    
    showProgressBar(false);
    EnableWindow(m_scanButton, TRUE);
    
    if (m_detectedSoftware.empty()) {
        setStatusText(L"No Adobe software found");
        showMessageBox(L"Scan Complete", L"No Adobe software was found on this system.");
        return;
    }
    
    // Populate the list view
    refreshSoftwareList();
    
    // Format status message
    std::wostringstream oss;
    oss << L"Found " << m_detectedSoftware.size() << L" Adobe software products";
    setStatusText(oss.str());
    
    // Enable buttons
    EnableWindow(m_blockButton, TRUE);
    EnableWindow(m_unblockButton, TRUE);
    EnableWindow(m_selectAllButton, TRUE);
    EnableWindow(m_unselectAllButton, TRUE);
}

void MainWindow::refreshSoftwareList() {
    // Clear the list
    ListView_DeleteAllItems(m_softwareList);
    
    // Add items to the list
    for (size_t i = 0; i < m_detectedSoftware.size(); ++i) {
        const auto& software = m_detectedSoftware[i];
        
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(software.name.c_str());
        lvi.lParam = (LPARAM)i; // Store the index for later reference
        
        int itemIndex = ListView_InsertItem(m_softwareList, &lvi);
        
        // Set version
        ListView_SetItemText(m_softwareList, itemIndex, 1, 
                           const_cast<LPWSTR>(software.version.empty() ? L"Unknown" : software.version.c_str()));
        
        // Set path
        ListView_SetItemText(m_softwareList, itemIndex, 2, const_cast<LPWSTR>(software.path.c_str()));
        
        // Set background color for blocked items
        if (software.isBlocked) {
            // Unfortunately, Windows ListView doesn't support per-item background colors by default.
            // You would need to implement custom drawing, which is beyond the scope of this example.
            // Instead, we'll add a marker to the item text
            std::wstring name = software.name + L" [BLOCKED]";
            ListView_SetItemText(m_softwareList, itemIndex, 0, const_cast<LPWSTR>(name.c_str()));
        }
    }
}

void MainWindow::blockSelectedSoftware() {
    // Count selected items
    int selectedCount = 0;
    int itemCount = ListView_GetItemCount(m_softwareList);
    std::vector<int> selectedIndices;
    
    for (int i = 0; i < itemCount; ++i) {
        if (ListView_GetCheckState(m_softwareList, i)) {
            selectedCount++;
            selectedIndices.push_back(i);
        }
    }
    
    if (selectedCount == 0) {
        showMessageBox(L"No Selection", L"Please select at least one Adobe application to block.", MB_ICONWARNING);
        return;
    }
    
    bool inbound = SendMessage(m_inboundCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool outbound = SendMessage(m_outboundCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    if (!inbound && !outbound) {
        showMessageBox(L"No Rule Type", L"Please select at least one rule type (inbound or outbound).", MB_ICONWARNING);
        return;
    }
    
    int successCount = 0;
    
    showProgressBar(true);
    setProgressBarRange(0, selectedCount);
    setStatusText(L"Creating firewall rules...");
    
    for (size_t i = 0; i < selectedIndices.size(); ++i) {
        int itemIndex = selectedIndices[i];
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = itemIndex;
        ListView_GetItem(m_softwareList, &lvi);
        
        size_t softwareIndex = (size_t)lvi.lParam;
        if (softwareIndex < m_detectedSoftware.size()) {
            const auto& software = m_detectedSoftware[softwareIndex];
            
            // Extract baseName from path
            std::wstring path = software.path;
            size_t lastBackslash = path.find_last_of(L'\\');
            std::wstring baseName;
            if (lastBackslash != std::wstring::npos) {
                baseName = path.substr(lastBackslash + 1);
            } else {
                baseName = path;
            }
            
            // Sử dụng tên mới "VUTHAO Block Adobe..." cho rule firewall
            std::wstring customRuleName = L"VUTHAO Block Adobe - " + baseName;
            
            if (m_firewallManager.blockApplication(software.path, customRuleName, inbound, outbound)) {
                successCount++;
                
                // Update the software info
                m_detectedSoftware[softwareIndex].isBlocked = true;
                
                // Update the list item text
                std::wstring name = software.name + L" [BLOCKED]";
                ListView_SetItemText(m_softwareList, itemIndex, 0, const_cast<LPWSTR>(name.c_str()));
            }
        }
        
        setProgressBarPos((int)i + 1);
    }
    
    showProgressBar(false);
    
    // Format status message
    std::wostringstream oss;
    oss << L"Successfully blocked " << successCount << L" of " << selectedCount << L" applications";
    setStatusText(oss.str());
    
    if (successCount != selectedCount) {
        std::wostringstream message;
        message << L"Successfully blocked " << successCount << L" of " << selectedCount 
               << L" applications. Some applications could not be blocked.";
        showMessageBox(L"Partial Success", message.str(), MB_ICONWARNING);
    } else {
        std::wostringstream message;
        message << L"Successfully blocked all " << successCount << L" selected applications.";
        showMessageBox(L"Success", message.str());
    }
}

void MainWindow::unblockSelectedSoftware() {
    // Count selected items
    int selectedCount = 0;
    int itemCount = ListView_GetItemCount(m_softwareList);
    std::vector<int> selectedIndices;
    
    for (int i = 0; i < itemCount; ++i) {
        if (ListView_GetCheckState(m_softwareList, i)) {
            selectedCount++;
            selectedIndices.push_back(i);
        }
    }
    
    if (selectedCount == 0) {
        showMessageBox(L"No Selection", L"Please select at least one Adobe application to unblock.", MB_ICONWARNING);
        return;
    }
    
    bool inbound = SendMessage(m_inboundCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool outbound = SendMessage(m_outboundCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    if (!inbound && !outbound) {
        showMessageBox(L"No Rule Type", L"Please select at least one rule type (inbound or outbound).", MB_ICONWARNING);
        return;
    }
    
    int successCount = 0;
    
    showProgressBar(true);
    setProgressBarRange(0, selectedCount);
    setStatusText(L"Removing firewall rules...");
    
    for (size_t i = 0; i < selectedIndices.size(); ++i) {
        int itemIndex = selectedIndices[i];
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = itemIndex;
        ListView_GetItem(m_softwareList, &lvi);
        
        size_t softwareIndex = (size_t)lvi.lParam;
        if (softwareIndex < m_detectedSoftware.size()) {
            const auto& software = m_detectedSoftware[softwareIndex];
            
            // Extract baseName from path
            std::wstring path = software.path;
            size_t lastBackslash = path.find_last_of(L'\\');
            std::wstring baseName;
            if (lastBackslash != std::wstring::npos) {
                baseName = path.substr(lastBackslash + 1);
            } else {
                baseName = path;
            }
            
            // Sử dụng tên custom rule giống như khi tạo
            std::wstring customRuleName = L"VUTHAO Block Adobe - " + baseName;
            
            if (m_firewallManager.unblockApplication(software.path, customRuleName, inbound, outbound)) {
                successCount++;
                
                // Update the software info
                m_detectedSoftware[softwareIndex].isBlocked = false;
                
                // Update the list item text - remove [BLOCKED] marker if present
                std::wstring name = software.name;
                size_t pos = name.find(L" [BLOCKED]");
                if (pos != std::wstring::npos) {
                    name = name.substr(0, pos);
                }
                ListView_SetItemText(m_softwareList, itemIndex, 0, const_cast<LPWSTR>(name.c_str()));
            }
        }
        
        setProgressBarPos((int)i + 1);
    }
    
    showProgressBar(false);
    
    // Format status message
    std::wostringstream oss;
    oss << L"Successfully unblocked " << successCount << L" of " << selectedCount << L" applications";
    setStatusText(oss.str());
    
    if (successCount != selectedCount) {
        std::wostringstream message;
        message << L"Successfully unblocked " << successCount << L" of " << selectedCount 
               << L" applications. Some applications could not be unblocked.";
        showMessageBox(L"Partial Success", message.str(), MB_ICONWARNING);
    } else {
        std::wostringstream message;
        message << L"Successfully unblocked all " << successCount << L" selected applications.";
        showMessageBox(L"Success", message.str());
    }
}

void MainWindow::selectAllSoftware() {
    int itemCount = ListView_GetItemCount(m_softwareList);
    for (int i = 0; i < itemCount; ++i) {
        ListView_SetCheckState(m_softwareList, i, TRUE);
    }
}

void MainWindow::unselectAllSoftware() {
    int itemCount = ListView_GetItemCount(m_softwareList);
    for (int i = 0; i < itemCount; ++i) {
        ListView_SetCheckState(m_softwareList, i, FALSE);
    }
}

void MainWindow::setStatusText(const std::wstring& text) {
    SetWindowText(m_statusLabel, text.c_str());
}

void MainWindow::showProgressBar(bool show) {
    ShowWindow(m_progressBar, show ? SW_SHOW : SW_HIDE);
}

void MainWindow::setProgressBarRange(int min, int max) {
    SendMessage(m_progressBar, PBM_SETRANGE, 0, MAKELPARAM(min, max));
}

void MainWindow::setProgressBarPos(int pos) {
    SendMessage(m_progressBar, PBM_SETPOS, (WPARAM)pos, 0);
}

void MainWindow::enableControls(bool enable) {
    EnableWindow(m_scanButton, enable);
    EnableWindow(m_blockButton, enable);
    EnableWindow(m_unblockButton, enable);
    EnableWindow(m_selectAllButton, enable);
    EnableWindow(m_unselectAllButton, enable);
}

void MainWindow::showMessageBox(const std::wstring& title, const std::wstring& message, UINT type) {
    MessageBox(m_hwnd, message.c_str(), title.c_str(), type);
}
