#pragma once
#include <string>
#include <ctime>

namespace SARPLinggo {

struct LicenseInfo {
    std::string token;
    std::string hwid;
    int days = 0;
    std::string activated_at;
    std::string expires_at;
    bool is_valid = false;
};

class LicenseManager {
private:
    std::string m_secret_key = "SARP_LINGGO_SECRET_HMAC_KEY_2026";
    std::string m_license_file = "SARPLinggo_license.json";
    LicenseInfo m_info;
    bool m_active = false;

public:
    LicenseManager(const std::string& secret_key = "SARP_LINGGO_SECRET_HMAC_KEY_2026");
    ~LicenseManager() = default;

    static std::string get_local_hwid();
    static std::string hmac_sha256_simple(const std::string& key, const std::string& data);

    bool verify_token(const std::string& token, std::string& out_hwid, int& out_days, int& out_p2);
    bool activate_token(const std::string& token, std::string& out_message);
    
    bool is_active() const { return m_active; }
    LicenseInfo get_license_info() const { return m_info; }
    void load_stored_license();
};

extern LicenseManager* g_license;

} // namespace SARPLinggo
