// crc32.hpp - Tambahkan ke deps/ atau include/
#ifndef CRC32_HPP
#define CRC32_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

class CRC32 {
private:
    static uint32_t table[256];
    static bool initialized;
    
    static void init_table() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
            }
            table[i] = crc;
        }
        initialized = true;
    }

    uint32_t crc = 0xFFFFFFFF;

public:
    CRC32() {
        if (!initialized) init_table();
    }
    
    // Reset untuk kalkulasi baru
    void reset() { crc = 0xFFFFFFFF; }
    
    // Update dengan chunk data (streaming)
    void update(const void* data, size_t length) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < length; ++i) {
            crc = table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
        }
    }
    
    // Update dengan string
    void update(const std::string& str) {
        update(str.data(), str.size());
    }
    
    // Finalize dan return CRC32
    uint32_t final() const {
        return ~crc;
    }
    
    // Helper: Calculate dari file existing (untuk verifikasi post-upload)
    static uint32_t fileChecksum(const std::string& filepath) {
        CRC32 crc;
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return 0;
        
        char buffer[8192];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            crc.update(buffer, file.gcount());
        }
        return crc.final();
    }
    
    // Convert ke hex string
    static std::string toHex(uint32_t crc) {
        char hex[9];
        snprintf(hex, sizeof(hex), "%08X", crc);
        return std::string(hex);
    }
    
    // Parse dari hex string
    static uint32_t fromHex(const std::string& hex) {
        return static_cast<uint32_t>(std::strtoul(hex.c_str(), nullptr, 16));
    }
};

// Static definitions
uint32_t CRC32::table[256];
bool CRC32::initialized = false;

#endif // CRC32_HPP