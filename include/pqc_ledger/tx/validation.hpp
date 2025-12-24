#pragma once

#include "../types.hpp"
#include "../error.hpp"

namespace pqc_ledger::tx {

/**
 * Perform cheap validation checks on a transaction.
 * These checks should be done before expensive signature verification.
 * 
 * Checks:
 * - Version is 1
 * - Chain ID matches expected
 * - Nonce is valid (non-zero, reasonable range)
 * - Amount and fee are valid (non-zero, reasonable)
 * - Public key size matches expected PQ algorithm size
 * - Address format is valid
 * - Auth mode and signature sizes match
 * 
 * @param tx Transaction to validate
 * @param expected_chain_id Expected chain ID
 * @return Result indicating success or error
 */
Result<void> validate_cheap_checks(const Transaction& tx, uint32_t expected_chain_id);

/**
 * Full transaction validation pipeline.
 * 
 * Ordering (DoS-aware):
 * 1. Parse and decode (already done if we have Transaction object)
 * 2. Cheap structural checks (validate_cheap_checks)
 * 3. Expensive signature verification (verify_transaction)
 * 
 * @param tx Transaction to validate
 * @param chain_id Expected chain ID
 * @return Result<bool> - true if valid, false if invalid, or error
 */
Result<bool> validate_transaction(const Transaction& tx, uint32_t chain_id);

/**
 * Check if transaction structure is valid.
 * 
 * @param tx Transaction to check
 * @return true if structure is valid, false otherwise
 */
bool is_valid_structure(const Transaction& tx);

} // namespace pqc_ledger::tx

