#include "pqc_ledger/crypto/address.hpp"
#include "pqc_ledger/crypto/hash.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace pqc_ledger::crypto {

Result<Address> derive_address(const PublicKey& pubkey) {
    // Address = first_32_bytes(SHA256(from_pubkey_bytes))
    auto hash_result = sha256(pubkey);
    if (hash_result.is_err()) {
        return Result<Address>::Err(hash_result.error());
    }
    
    const auto& hash = hash_result.value();
    if (hash.size() < 32) {
        return Result<Address>::Err(Error(ErrorCode::HashError, "Hash too short"));
    }
    
    Address addr;
    std::copy(hash.begin(), hash.begin() + 32, addr.begin());
    
    return Result<Address>::Ok(addr);
}

std::string address_to_hex(const Address& addr) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : addr) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

Result<Address> address_from_hex(const std::string& hex) {
    if (hex.size() != 64) {
        return Result<Address>::Err(Error(ErrorCode::InvalidHexEncoding,
            "Hex string must be 64 characters (32 bytes)"));
    }
    
    Address addr;
    for (size_t i = 0; i < 32; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        try {
            addr[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        } catch (...) {
            return Result<Address>::Err(Error(ErrorCode::InvalidHexEncoding,
                "Invalid hex character at position " + std::to_string(i * 2)));
        }
    }
    
    return Result<Address>::Ok(addr);
}

} // namespace pqc_ledger::crypto

