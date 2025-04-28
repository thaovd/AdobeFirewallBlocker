#include "firewall_manager.h"
#include <comdef.h>
#include <string>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

FirewallManager::FirewallManager() : m_initialized(false) {
    initialize();
}

FirewallManager::~FirewallManager() {
    // COM cleanup is handled by smart pointers
}

bool FirewallManager::initialize() {
    if (m_initialized) return true;

    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return false;
    }

    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        reinterpret_cast<void**>(m_firewallPolicy.GetAddressOf()));

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    m_initialized = true;
    return true;
}

bool FirewallManager::blockApplication(const std::wstring& path, const std::wstring& name, bool inbound, bool outbound) {
    if (!m_initialized) {
        if (!initialize()) return false;
    }

    bool success = true;

    if (inbound) {
        success &= createFirewallRule(path, name, true);
    }

    if (outbound) {
        success &= createFirewallRule(path, name, false);
    }

    return success;
}

bool FirewallManager::unblockApplication(const std::wstring& path, const std::wstring& name, bool inbound, bool outbound) {
    if (!m_initialized) {
        if (!initialize()) return false;
    }

    bool success = true;

    if (inbound) {
        success &= removeFirewallRule(name, true);
    }

    if (outbound) {
        success &= removeFirewallRule(name, false);
    }

    return success;
}

bool FirewallManager::createFirewallRule(const std::wstring& path, const std::wstring& name, bool inbound) {
    if (!m_initialized) return false;

    try {
        ComPtr<INetFwRules> pFwRules;
        HRESULT hr = m_firewallPolicy->get_Rules(&pFwRules);
        if (FAILED(hr)) {
            return false;
        }

        ComPtr<INetFwRule> pFwRule;
        hr = CoCreateInstance(
            __uuidof(NetFwRule),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwRule),
            reinterpret_cast<void**>(pFwRule.GetAddressOf()));

        if (FAILED(hr)) {
            return false;
        }

        // Generate rule name with "VUTHAO Block Adobe"
        std::wstring ruleName = L"" + name + L" - " + (inbound ? L"Inbound" : L"Outbound");

        // Replace forward slashes with backslashes
        std::wstring updatedPath = path;
        for (size_t i = 0; i < updatedPath.length(); i++) {
            if (updatedPath[i] == L'/') updatedPath[i] = L'\\';
        }

        // Set rule properties
        pFwRule->put_Name(_bstr_t(ruleName.c_str()));
        pFwRule->put_ApplicationName(_bstr_t(updatedPath.c_str()));
        pFwRule->put_Action(NET_FW_ACTION_BLOCK);
        pFwRule->put_Enabled(VARIANT_TRUE);
        pFwRule->put_Direction(inbound ? NET_FW_RULE_DIR_IN : NET_FW_RULE_DIR_OUT);

        // Add the rule
        hr = pFwRules->Add(pFwRule.Get());
        if (FAILED(hr)) {
            return false;
        }

        return true;
    }
    catch (_com_error& e) {
        return false;
    }
}

bool FirewallManager::removeFirewallRule(const std::wstring& name, bool inbound) {
    if (!m_initialized) return false;

    try {
        ComPtr<INetFwRules> pFwRules;
        HRESULT hr = m_firewallPolicy->get_Rules(&pFwRules);
        if (FAILED(hr)) {
            return false;
        }

        // Generate rule name
        std::wstring ruleName = L"VUTHAO Block - " + name + L" - " + (inbound ? L"Inbound" : L"Outbound");

        // Remove the rule
        hr = pFwRules->Remove(_bstr_t(ruleName.c_str()));
        if (FAILED(hr)) {
            return false;
        }

        return true;
    }
    catch (_com_error& e) {
        return false;
    }
}

bool FirewallManager::isApplicationBlocked(const std::wstring& path) {
    if (!m_initialized) {
        if (!initialize()) return false;
    }

    try {
        ComPtr<INetFwRules> pFwRules;
        HRESULT hr = m_firewallPolicy->get_Rules(&pFwRules);
        if (FAILED(hr)) {
            return false;
        }

        // Enumerate through all rules to find matching ones
        IUnknown* pEnumerator;
        hr = pFwRules.Get()->get__NewEnum(&pEnumerator);
        if (FAILED(hr)) return false;

        IEnumVARIANT* pVariant;
        hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pVariant);
        if (FAILED(hr)) {
            pEnumerator->Release();
            return false;
        }

        pEnumerator->Release();

        VARIANT var;
        VariantInit(&var);

        ULONG fetched = 0;
        bool blocked = false;

        // Replace forward slashes with backslashes
        std::wstring updatedPath = path;
        for (size_t i = 0; i < updatedPath.length(); i++) {
            if (updatedPath[i] == L'/') updatedPath[i] = L'\\';
        }

        while (pVariant->Next(1, &var, &fetched) == S_OK && fetched > 0) {
            if (var.vt == VT_DISPATCH) {
                INetFwRule* pRule = nullptr;
                hr = var.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pRule);
                if (SUCCEEDED(hr) && pRule) {
                    BSTR bstrApp;
                    pRule->get_ApplicationName(&bstrApp);
                    if (bstrApp) {
                        std::wstring appPath = bstrApp;
                        SysFreeString(bstrApp);

                        // Case insensitive comparison
                        if (_wcsicmp(appPath.c_str(), updatedPath.c_str()) == 0) {
                            NET_FW_ACTION action;
                            pRule->get_Action(&action);

                            if (action == NET_FW_ACTION_BLOCK) {
                                blocked = true;
                                pRule->Release();
                                break;
                            }
                        }
                    }
                    pRule->Release();
                }
            }
            VariantClear(&var);
        }

        pVariant->Release();
        return blocked;
    }
    catch (_com_error&) {
        return false;
    }
}
