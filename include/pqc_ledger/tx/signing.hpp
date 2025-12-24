#pragma once

#include "../types.hpp"
#include "../error.hpp"
#include <string>
#include <vector>

namespace pqc_ledger::tx {

/**
 * Sign a transaction with post-quantum key.
 * 
 * This function:
 * 1. Encodes the transaction without signatures
 * 2. Creates domain-separated signing message
 * 3. Signs the message with the PQ private key
 * 4. Attaches the signature to the transaction
 * 
 * @param tx Transaction to sign (will be modified)
 * @param privkey PQ private key
 * @param algorithm PQ algorithm name (e.g., "Dilithium3")
 * @return Result indicating success or error
 */
Result<void> sign_transaction(Transaction& tx,
                              const std::vector<uint8_t>& privkey,
                              const std::string& algorithm = "Dilithium3");

/**
 * Sign a transaction in hybrid mode (classical + PQ).
 * 
 * @param tx Transaction to sign (will be modified)
 * @param pq_privkey PQ private key
 * @param ed25519_privkey Ed25519 private key
 * @param pq_algorithm PQ algorithm name (e.g., "Dilithium3")
 * @return Result indicating success or error
 */
Result<void> sign_transaction_hybrid(Transaction& tx,
                                     const std::vector<uint8_t>& pq_privkey,
                                     const std::vector<uint8_t>& ed25519_privkey,
                                     const std::string& pq_algorithm = "Dilithium3");

/**
 * Verify a transaction signature.
 * 
 * This function:
 * 1. Encodes the transaction without signatures
 * 2. Creates domain-separated signing message
 * 3. Verifies the signature(s) against the message
 * 
 * @param tx Transaction to verify
 * @param chain_id Expected chain ID (for domain separation)
 * @return Result<bool> - true if valid, false if invalid, or error
 */
Result<bool> verify_transaction(const Transaction& tx, uint32_t chain_id);

} // namespace pqc_ledger::tx

