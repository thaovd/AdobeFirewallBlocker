# Adobe Firewall Blocker

**Adobe Firewall Blocker** is a Windows application that uses the Win32 API to detect and manage Adobe applications through the Windows Firewall. The application allows users to block or unblock Adobe applications by creating or removing firewall rules.

## Features

- **Scan for Adobe Applications**: Automatically detects installed Adobe applications on the system.
- **Block Applications**: Creates firewall rules to block selected Adobe applications.
- **Unblock Applications**: Removes firewall rules to unblock selected Adobe applications.
- **Firewall Rule Management**: Supports both Inbound and Outbound rules.
- **Simple User Interface**: Built with Win32 API components such as ListView, ProgressBar, and Buttons.

## System Requirements

- **Operating System**: Windows 7 or later.
- **Administrator Privileges**: The application requires **Administrator** rights to manage firewall rules.
- **Development Tools**: Visual Studio 2022 or CMake for building the source code.

## How to Use

### 1. Download and Run the Application
- Download the executable file `AdobeFirewallBlocker.exe` from the `build_vs2022/Release` directory.
- Right-click the file and select **Run as administrator**.

### 2. Main Features
- **Scan for Adobe Software**: Click this button to scan for installed Adobe applications.
- **Block Selected**: Select applications from the list and click this button to block them.
- **Unblock Selected**: Select applications from the list and click this button to unblock them.
- **Select All / Unselect All**: Select or deselect all applications in the list.

### 3. Firewall Rules
- **Inbound Rules**: Blocks incoming connections to the application.
- **Outbound Rules**: Blocks outgoing connections from the application.

## Build Instructions

### 1. Install Required Tools
- [CMake](https://cmake.org/download/)
- [Visual Studio 2022](https://visualstudio.microsoft.com/)

### 2. Build the Source Code
Open a Command Prompt and run the following commands:

```bash
cd src
mkdir build_vs2022
cd build_vs2022
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The executable file will be generated at: `c:\Users\Admin\Desktop\demo\build_vs2022\Release\AdobeFirewallBlocker.exe`.

## Project Structure

```
demo/
├── src/
│   ├── main.cpp                # Entry point of the application
│   ├── mainwindow.cpp          # Main window logic
│   ├── mainwindow.h            # Main window declarations
│   ├── software_scanner.cpp    # Scans for Adobe applications
│   ├── software_scanner.h      # SoftwareScanner declarations
│   ├── firewall_manager.cpp    # Manages firewall rules
│   ├── firewall_manager.h      # FirewallManager declarations
│   ├── resources.rc            # Resource definitions (dialogs, icons)
│   └── resources/              # Directory for icons
│       └── app.ico             # Application icon
├── CMakeLists.txt              # CMake configuration
└── README.md                   # Documentation
```

## Notes

- **Application Icon**: Place the icon file at `src/resources/app.ico`. This path is referenced in the `resources.rc` file.
- **Administrator Privileges**: The application requires administrator privileges to create and delete firewall rules.

## Contact

- **Website**: [vuthao.id.vn](https://vuthao.id.vn)
- **Email**: support@vuthao.id.vn
