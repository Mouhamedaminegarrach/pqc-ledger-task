#include "pqc_ledger/codec/encode.hpp"
#include "pqc_ledger/types.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace pqc_ledger::codec {

namespace {
    void write_u8(std::vector<uint8_t>& out, uint8_t value) {
        out.push_back(value);
    }
    
    void write_u16_be(std::vector<uint8_t>& out, uint16_t value) {
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    }
    
    void write_u32_be(std::vector<uint8_t>& out, uint32_t value) {
        out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    }
    
    void write_u64_be(std::vector<uint8_t>& out, uint64_t value) {
        out.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    }
    
    void write_bytes_with_len(std::vector<uint8_t>& out, const std::vector<uint8_t>& bytes) {
        if (bytes.size() > UINT16_MAX) {
            throw std::runtime_error("Bytes length exceeds u16 max");
        }
        write_u16_be(out, static_cast<uint16_t>(bytes.size()));
        out.insert(out.end(), bytes.begin(), bytes.end());
    }
}

Result<std::vector<uint8_t>> encode(const Transaction& tx) {
    std::vector<uint8_t> out;
    
    // Version
    write_u8(out, tx.version);
    
    // Chain ID
    write_u32_be(out, tx.chain_id);
    
    // Nonce
    write_u64_be(out, tx.nonce);
    
    // From pubkey (variable length with prefix)
    write_bytes_with_len(out, tx.from_pubkey);
    
    // To address (fixed 32 bytes, no length prefix)
    out.insert(out.end(), tx.to.begin(), tx.to.end());
    
    // Amount
    write_u64_be(out, tx.amount);
    
    // Fee
    write_u64_be(out, tx.fee);
    
    // Auth tag
    write_u8(out, static_cast<uint8_t>(tx.auth_mode));
    
    // Auth payload
    if (tx.auth_mode == AuthMode::PqOnly) {
        const auto& pq_sig = std::get<PqSignature>(tx.auth);
        write_bytes_with_len(out, pq_sig.sig);
    } else if (tx.auth_mode == AuthMode::Hybrid) {
        const auto& hybrid_sig = std::get<HybridSignature>(tx.auth);
        write_bytes_with_len(out, hybrid_sig.classical_sig);
        write_bytes_with_len(out, hybrid_sig.pq_sig);
    }
    
    return Result<std::vector<uint8_t>>::Ok(std::move(out));
}

Result<std::vector<uint8_t>> encode_for_signing(const Transaction& tx) {
    std::vector<uint8_t> out;
    
    // Version
    write_u8(out, tx.version);
    
    // Chain ID
    write_u32_be(out, tx.chain_id);
    
    // Nonce
    write_u64_be(out, tx.nonce);
    
    // From pubkey (variable length with prefix)
    write_bytes_with_len(out, tx.from_pubkey);
    
    // To address (fixed 32 bytes, no length prefix)
    out.insert(out.end(), tx.to.begin(), tx.to.end());
    
    // Amount
    write_u64_be(out, tx.amount);
    
    // Fee
    write_u64_be(out, tx.fee);
    
    // Note: No auth field - signatures are excluded
    
    return Result<std::vector<uint8_t>>::Ok(std::move(out));
}

std::string encode_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::string encode_to_base64(const std::vector<uint8_t>& bytes) {
    const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    result.reserve(((bytes.size() + 2) / 3) * 4);
    
    size_t i = 0;
    while (i < bytes.size()) {
        // Get 3 bytes (24 bits)
        size_t bytes_read = 0;
        uint32_t b0 = 0, b1 = 0, b2 = 0;
        
        if (i < bytes.size()) {
            b0 = bytes[i++];
            bytes_read++;
        }
        if (i < bytes.size()) {
            b1 = bytes[i++];
            bytes_read++;
        }
        if (i < bytes.size()) {
            b2 = bytes[i++];
            bytes_read++;
        }
        
        // Combine into 24-bit value
        uint32_t combined = (b0 << 16) | (b1 << 8) | b2;
        
        // Extract 4 groups of 6 bits each
        result += base64_chars[(combined >> 18) & 0x3F];
        result += base64_chars[(combined >> 12) & 0x3F];
        
        // Third character: present if we have at least 2 bytes
        if (bytes_read >= 2) {
            result += base64_chars[(combined >> 6) & 0x3F];
        } else {
            result += '=';
        }
        
        // Fourth character: present only if we have 3 bytes
        if (bytes_read >= 3) {
            result += base64_chars[combined & 0x3F];
        } else {
            result += '=';
        }
    }
    
    return result;
}

Result<std::string> encode_to_hex(const Transaction& tx) {
    auto encoded_result = encode(tx);
    if (encoded_result.is_err()) {
        return Result<std::string>::Err(encoded_result.error());
    }
    return Result<std::string>::Ok(encode_to_hex(encoded_result.value()));
}

Result<std::string> encode_to_base64(const Transaction& tx) {
    auto encoded_result = encode(tx);
    if (encoded_result.is_err()) {
        return Result<std::string>::Err(encoded_result.error());
    }
    return Result<std::string>::Ok(encode_to_base64(encoded_result.value()));
}

} // namespace pqc_ledger::codec

