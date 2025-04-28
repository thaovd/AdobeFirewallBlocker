#ifndef FIREWALL_MANAGER_H
#define FIREWALL_MANAGER_H

#include <string>
#include <windows.h>
#include <netfw.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

class FirewallManager {
public:
    FirewallManager();
    ~FirewallManager();

    bool initialize();
    bool blockApplication(const std::wstring& path, const std::wstring& name, bool inbound, bool outbound);
    bool unblockApplication(const std::wstring& path, const std::wstring& name, bool inbound, bool outbound);
    bool isApplicationBlocked(const std::wstring& path);

private:
    bool createFirewallRule(const std::wstring& path, const std::wstring& name, bool inbound);
    bool removeFirewallRule(const std::wstring& name, bool inbound);
    std::wstring generateRuleName(const std::wstring& appName, bool inbound);

    ComPtr<INetFwPolicy2> m_firewallPolicy;
    bool m_initialized;
};

#endif // FIREWALL_MANAGER_H
