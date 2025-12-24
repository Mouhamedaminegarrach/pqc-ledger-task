#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "error.hpp"

namespace pqc_ledger {

// Address is 32 bytes (first 32 bytes of SHA256(pubkey))
using Address = std::array<uint8_t, 32>;

// Public key size depends on the PQ algorithm
// Dilithium2: 1312 bytes, Dilithium3: 1952 bytes, Dilithium5: 2592 bytes
// We'll use Dilithium3 as default (1952 bytes)
constexpr size_t PQ_PUBKEY_SIZE = 1952;  // Dilithium3
constexpr size_t PQ_SIG_SIZE = 3293;     // Dilithium3 signature size

// Ed25519 sizes (for hybrid mode)
constexpr size_t ED25519_PUBKEY_SIZE = 32;
constexpr size_t ED25519_SIG_SIZE = 64;

using PublicKey = std::vector<uint8_t>;
using Signature = std::vector<uint8_t>;

enum class AuthMode : uint8_t {
    PqOnly = 0,
    Hybrid = 1
};

struct PqSignature {
    Signature sig;
};

struct HybridSignature {
    Signature classical_sig;  // Ed25519
    Signature pq_sig;         // Dilithium
};

// Transaction structure
struct Transaction {
    uint8_t version;           // Must be 1
    uint32_t chain_id;
    uint64_t nonce;
    PublicKey from_pubkey;     // Fixed length based on PQ algorithm
    Address to;                // Fixed 32 bytes
    uint64_t amount;
    uint64_t fee;
    AuthMode auth_mode;
    
    // Auth payload (one of these based on auth_mode)
    std::variant<PqSignature, HybridSignature> auth;
    
    // Helper to get address from pubkey
    static Address derive_address(const PublicKey& pubkey);
    
    // Validation helpers
    bool is_valid_structure() const;
};

} // namespace pqc_ledger

