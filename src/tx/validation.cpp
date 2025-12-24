#include "pqc_ledger/tx/validation.hpp"
#include "pqc_ledger/tx/signing.hpp"
#include "pqc_ledger/crypto/pq.hpp"
#include "pqc_ledger/crypto/address.hpp"

namespace pqc_ledger::tx {

Result<void> validate_cheap_checks(const Transaction& tx, uint32_t expected_chain_id) {
    // Version must be 1
    if (tx.version != 1) {
        return Result<void>::Err(Error(ErrorCode::InvalidVersion,
            "Version must be 1, got " + std::to_string(tx.version)));
    }
    
    // Chain ID must match
    if (tx.chain_id != expected_chain_id) {
        return Result<void>::Err(Error(ErrorCode::InvalidChainId,
            "Chain ID mismatch: expected " + std::to_string(expected_chain_id) +
            ", got " + std::to_string(tx.chain_id)));
    }
    
    // Nonce should be non-zero (basic check)
    if (tx.nonce == 0) {
        return Result<void>::Err(Error(ErrorCode::InvalidTransaction, "Nonce cannot be zero"));
    }
    
    // Amount and fee should be non-zero
    if (tx.amount == 0) {
        return Result<void>::Err(Error(ErrorCode::InvalidAmount, "Amount cannot be zero"));
    }
    if (tx.fee == 0) {
        return Result<void>::Err(Error(ErrorCode::InvalidFee, "Fee cannot be zero"));
    }
    
    // Public key size must match expected PQ algorithm size
    // Default to Dilithium3
    auto pubkey_size_result = crypto::get_pubkey_size("Dilithium3");
    if (pubkey_size_result.is_err()) {
        return Result<void>::Err(pubkey_size_result.error());
    }
    if (tx.from_pubkey.size() != pubkey_size_result.value()) {
        return Result<void>::Err(Error(ErrorCode::InvalidPublicKey,
            "Public key size mismatch: expected " + std::to_string(pubkey_size_result.value()) +
            ", got " + std::to_string(tx.from_pubkey.size())));
    }
    
    // Validate auth mode and signature sizes
    if (tx.auth_mode == AuthMode::PqOnly) {
        const auto& pq_sig = std::get<PqSignature>(tx.auth);
        auto sig_size_result = crypto::get_signature_size("Dilithium3");
        if (sig_size_result.is_err()) {
            return Result<void>::Err(sig_size_result.error());
        }
        if (pq_sig.sig.size() != sig_size_result.value()) {
            return Result<void>::Err(Error(ErrorCode::InvalidSignature,
                "PQ signature size mismatch: expected " + std::to_string(sig_size_result.value()) +
                ", got " + std::to_string(pq_sig.sig.size())));
        }
    } else if (tx.auth_mode == AuthMode::Hybrid) {
        const auto& hybrid_sig = std::get<HybridSignature>(tx.auth);
        
        // Check Ed25519 signature size
        if (hybrid_sig.classical_sig.size() != 64) {
            return Result<void>::Err(Error(ErrorCode::InvalidSignature,
                "Ed25519 signature size mismatch: expected 64, got " +
                std::to_string(hybrid_sig.classical_sig.size())));
        }
        
        // Check PQ signature size
        auto sig_size_result = crypto::get_signature_size("Dilithium3");
        if (sig_size_result.is_err()) {
            return Result<void>::Err(sig_size_result.error());
        }
        if (hybrid_sig.pq_sig.size() != sig_size_result.value()) {
            return Result<void>::Err(Error(ErrorCode::InvalidSignature,
                "PQ signature size mismatch: expected " + std::to_string(sig_size_result.value()) +
                ", got " + std::to_string(hybrid_sig.pq_sig.size())));
        }
    } else {
        return Result<void>::Err(Error(ErrorCode::InvalidAuthTag, "Unknown auth mode"));
    }
    
    return Result<void>::Ok();
}

Result<bool> validate_transaction(const Transaction& tx, uint32_t chain_id) {
    // DoS-aware ordering:
    // 1. Cheap structural checks first
    auto cheap_result = validate_cheap_checks(tx, chain_id);
    if (cheap_result.is_err()) {
        return Result<bool>::Ok(false);  // Invalid, but return false (not error)
    }
    
    // 2. Expensive signature verification last
    auto verify_result = verify_transaction(tx, chain_id);
    if (verify_result.is_err()) {
        return Result<bool>::Err(verify_result.error());
    }
    return Result<bool>::Ok(verify_result.value());
}

bool is_valid_structure(const Transaction& tx) {
    // Basic structure validation
    if (tx.version != 1) {
        return false;
    }
    
    if (tx.from_pubkey.empty()) {
        return false;
    }
    
    if (tx.auth_mode == AuthMode::PqOnly) {
        const auto& pq_sig = std::get<PqSignature>(tx.auth);
        return !pq_sig.sig.empty();
    } else if (tx.auth_mode == AuthMode::Hybrid) {
        const auto& hybrid_sig = std::get<HybridSignature>(tx.auth);
        return !hybrid_sig.classical_sig.empty() && !hybrid_sig.pq_sig.empty();
    }
    
    return false;
}

} // namespace pqc_ledger::tx

