#include "licensing.h"
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>

namespace SARPLinggo {

LicenseManager* g_license = nullptr;

LicenseManager::LicenseManager(const std::string& secret_key) : m_secret_key(secret_key) {
    load_stored_license();
}

std::string LicenseManager::get_local_hwid() {
    HKEY hKey;
    char buffer[256] = {0};
    DWORD dwSize = sizeof(buffer);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, (LPBYTE)buffer, &dwSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            std::string guid(buffer);
            guid.erase(std::remove(guid.begin(), guid.end(), '-'), guid.end());
            return guid;
        }
        RegCloseKey(hKey);
    }
    return "UNKNOWN_HWID";
}

std::string LicenseManager::hmac_sha256_simple(const std::string& key, const std::string& data) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    std::string result = "";

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
        return result;
    }

    DWORD cbHash = 0, cbData = 0;
    if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0) == 0) {
        std::vector<UCHAR> hash(cbHash);
        if (BCryptCreateHash(hAlg, &hHash, NULL, 0, (PBYTE)key.data(), (ULONG)key.size(), 0) == 0) {
            BCryptHashData(hHash, (PBYTE)data.data(), (ULONG)data.size(), 0);
            BCryptFinishHash(hHash, hash.data(), (ULONG)hash.size(), 0);
            BCryptDestroyHash(hHash);

            std::stringstream ss;
            for (size_t i = 0; i < hash.size(); ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
            }
            result = ss.str();
        }
    }

    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

bool LicenseManager::verify_token(const std::string& token, std::string& out_hwid, int& out_days, int& out_p2) {
    // Format: SARP-XXXX-HWID-P1-P2
    if (token.rfind("SARP-", 0) != 0) return false;

    std::stringstream ss(token);
    std::string segment;
    std::vector<std::string> parts;
    while (std::getline(ss, segment, '-')) {
        parts.push_back(segment);
    }

    if (parts.size() < 5) return false;

    std::string prefix = parts[0];
    std::string days_str = parts[1];
    std::string hwid_part = parts[2];
    std::string sig_p1 = parts[3];
    std::string sig_p2 = parts[4];

    out_days = atoi(days_str.c_str());
    out_hwid = hwid_part;
    out_p2 = atoi(sig_p2.c_str());

    std::string local_hwid = get_local_hwid();
    if (hwid_part != "GLOB" && hwid_part != local_hwid) {
        return false;
    }

    std::string data_to_sign = prefix + "-" + days_str + "-" + hwid_part;
    std::string expected_hmac = hmac_sha256_simple(m_secret_key, data_to_sign);

    if (expected_hmac.empty() || sig_p1.empty()) return false;
    return (expected_hmac.rfind(sig_p1, 0) == 0);
}

bool LicenseManager::activate_token(const std::string& token, std::string& out_message) {
    std::string hwid;
    int days = 0, p2 = 0;
    if (!verify_token(token, hwid, days, p2)) {
        out_message = "Format token tidak valid atau tanda tangan digital token tidak cocok!";
        return false;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::time_t exp_time = now_time + (days * 86400LL);

    std::tm tm_now = *std::localtime(&now_time);
    std::tm tm_exp = *std::localtime(&exp_time);

    char act_buf[64], exp_buf[64];
    std::strftime(act_buf, sizeof(act_buf), "%Y-%m-%d %H:%M:%S", &tm_now);
    std::strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%d %H:%M:%S", &tm_exp);

    m_info.token = token;
    m_info.hwid = hwid;
    m_info.days = days;
    m_info.activated_at = act_buf;
    m_info.expires_at = exp_buf;
    m_info.is_valid = true;
    m_active = true;

    // Write to SARPLinggo_license.json
    std::ofstream out(m_license_file);
    if (out.is_open()) {
        out << "{\n";
        out << "  \"token\": \"" << token << "\",\n";
        out << "  \"hwid\": \"" << hwid << "\",\n";
        out << "  \"days\": " << days << ",\n";
        out << "  \"activated_at\": " << now_time << ",\n";
        out << "  \"expires_at\": " << exp_time << "\n";
        out << "}\n";
        out.close();
    }

    out_message = "Lisensi berhasil diaktifkan! Sisa durasi: " + std::to_string(days) + " Hari.";
    return true;
}

void LicenseManager::load_stored_license() {
    std::ifstream in(m_license_file);
    if (!in.is_open()) {
        m_active = false;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Parse simple JSON fields
    auto find_str = [&](const std::string& key) -> std::string {
        size_t p = content.find("\"" + key + "\":");
        if (p == std::string::npos) return "";
        size_t q1 = content.find("\"", p + key.length() + 3);
        if (q1 == std::string::npos) return "";
        size_t q2 = content.find("\"", q1 + 1);
        if (q2 == std::string::npos) return "";
        return content.substr(q1 + 1, q2 - q1 - 1);
    };

    auto find_int64 = [&](const std::string& key) -> int64_t {
        size_t p = content.find("\"" + key + "\":");
        if (p == std::string::npos) return 0;
        size_t start = p + key.length() + 3;
        while (start < content.length() && (content[start] == ' ' || content[start] == '\t')) start++;
        return atoll(&content[start]);
    };

    std::string token = find_str("token");
    std::string hwid = find_str("hwid");
    int64_t expires_at = find_int64("expires_at");

    if (token.empty()) {
        m_active = false;
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    if (expires_at > 0 && now_time > expires_at) {
        m_active = false;
        return;
    }

    std::string local_hwid = get_local_hwid();
    if (hwid != "GLOB" && hwid != local_hwid) {
        m_active = false;
        return;
    }

    std::string verified_hwid;
    int days = 0, p2 = 0;
    if (verify_token(token, verified_hwid, days, p2)) {
        m_info.token = token;
        m_info.hwid = hwid;
        m_info.days = days;
        std::time_t exp_t = (std::time_t)expires_at;
        std::tm tm_exp = *std::localtime(&exp_t);
        char exp_buf[64];
        std::strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%d %H:%M:%S", &tm_exp);
        m_info.expires_at = exp_buf;
        m_info.is_valid = true;
        m_active = true;
    } else {
        m_active = false;
    }
}

} // namespace SARPLinggo
