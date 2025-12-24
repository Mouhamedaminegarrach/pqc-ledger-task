#pragma once

#include "../error.hpp"
#include "../types.hpp"
#include <string>
#include <vector>

namespace pqc_ledger::crypto {

/**
 * Generate an Ed25519 key pair (for hybrid mode).
 * 
 * @return Result containing public key and private key, or error
 */
Result<std::pair<PublicKey, std::vector<uint8_t>>> generate_ed25519_keypair();

/**
 * Load Ed25519 public key from file.
 * 
 * @param path Path to public key file
 * @return Result containing public key or error
 */
Result<PublicKey> load_ed25519_public_key(const std::string& path);

/**
 * Load Ed25519 private key from file.
 * 
 * @param path Path to private key file
 * @return Result containing private key bytes or error
 */
Result<std::vector<uint8_t>> load_ed25519_private_key(const std::string& path);

/**
 * Save Ed25519 public key to file.
 * 
 * @param pubkey Public key to save
 * @param path Output file path
 * @return Result indicating success or error
 */
Result<void> save_ed25519_public_key(const PublicKey& pubkey, const std::string& path);

/**
 * Save Ed25519 private key to file.
 * 
 * @param privkey Private key to save
 * @param path Output file path
 * @return Result indicating success or error
 */
Result<void> save_ed25519_private_key(const std::vector<uint8_t>& privkey, const std::string& path);

/**
 * Sign a message with Ed25519 private key.
 * 
 * @param message Message to sign (32 bytes, typically a hash)
 * @param privkey Private key (32 bytes)
 * @return Result containing signature (64 bytes) or error
 */
Result<Signature> ed25519_sign(const std::vector<uint8_t>& message,
                                const std::vector<uint8_t>& privkey);

/**
 * Verify an Ed25519 signature.
 * 
 * @param message Message that was signed (32 bytes, typically a hash)
 * @param signature Signature to verify (64 bytes)
 * @param pubkey Public key (32 bytes)
 * @return Result<bool> - true if valid, false if invalid, or error
 */
Result<bool> ed25519_verify(const std::vector<uint8_t>& message,
                            const Signature& signature,
                            const PublicKey& pubkey);

} // namespace pqc_ledger::crypto

