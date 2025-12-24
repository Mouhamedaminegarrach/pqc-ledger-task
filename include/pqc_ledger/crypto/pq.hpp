#pragma once

#include "../error.hpp"
#include "../types.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace pqc_ledger::crypto {

/**
 * Generate a post-quantum key pair.
 * Uses Dilithium3 by default.
 * 
 * @param algorithm Algorithm name (e.g., "Dilithium3")
 * @return Result containing public key and private key paths, or error
 */
Result<std::pair<PublicKey, std::vector<uint8_t>>> generate_keypair(const std::string& algorithm = "Dilithium3");

/**
 * Load public key from file.
 * 
 * @param path Path to public key file
 * @return Result containing public key or error
 */
Result<PublicKey> load_public_key(const std::string& path);

/**
 * Load private key from file.
 * 
 * @param path Path to private key file
 * @return Result containing private key bytes or error
 */
Result<std::vector<uint8_t>> load_private_key(const std::string& path);

/**
 * Save public key to file.
 * 
 * @param pubkey Public key to save
 * @param path Output file path
 * @return Result indicating success or error
 */
Result<void> save_public_key(const PublicKey& pubkey, const std::string& path);

/**
 * Save private key to file.
 * 
 * @param privkey Private key to save
 * @param path Output file path
 * @return Result indicating success or error
 */
Result<void> save_private_key(const std::vector<uint8_t>& privkey, const std::string& path);

/**
 * Sign a message with a post-quantum private key.
 * 
 * @param message Message to sign (32 bytes, typically a hash)
 * @param privkey Private key
 * @param algorithm Algorithm name (e.g., "Dilithium3")
 * @return Result containing signature or error
 */
Result<Signature> sign(const std::vector<uint8_t>& message,
                       const std::vector<uint8_t>& privkey,
                       const std::string& algorithm = "Dilithium3");

/**
 * Verify a message signature with a post-quantum public key.
 * 
 * @param message Message that was signed (32 bytes, typically a hash)
 * @param signature Signature to verify
 * @param pubkey Public key
 * @param algorithm Algorithm name (e.g., "Dilithium3")
 * @return Result<bool> - true if valid, false if invalid, or error
 */
Result<bool> verify(const std::vector<uint8_t>& message,
                    const Signature& signature,
                    const PublicKey& pubkey,
                    const std::string& algorithm = "Dilithium3");

/**
 * Get expected public key size for algorithm.
 * 
 * @param algorithm Algorithm name
 * @return Result containing size in bytes or error
 */
Result<size_t> get_pubkey_size(const std::string& algorithm);

/**
 * Get expected signature size for algorithm.
 * 
 * @param algorithm Algorithm name
 * @return Result containing size in bytes or error
 */
Result<size_t> get_signature_size(const std::string& algorithm);

} // namespace pqc_ledger::crypto

