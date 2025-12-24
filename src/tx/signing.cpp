#include "pqc_ledger/tx/signing.hpp"
#include "pqc_ledger/codec/encode.hpp"
#include "pqc_ledger/crypto/hash.hpp"
#include "pqc_ledger/crypto/pq.hpp"
#include "pqc_ledger/crypto/classical.hpp"

namespace pqc_ledger::tx {

Result<void> sign_transaction(Transaction& tx,
                              const std::vector<uint8_t>& privkey,
                              const std::string& algorithm) {
    // 1. Encode transaction without signatures
    auto encoded_result = codec::encode_for_signing(tx);
    if (encoded_result.is_err()) {
        return Result<void>::Err(encoded_result.error());
    }
    
    // 2. Create domain-separated signing message
    auto msg_result = crypto::create_signing_message(tx.chain_id, encoded_result.value());
    if (msg_result.is_err()) {
        return Result<void>::Err(msg_result.error());
    }
    
    // 3. Sign the message with PQ private key
    auto sig_result = crypto::sign(msg_result.value(), privkey, algorithm);
    if (sig_result.is_err()) {
        return Result<void>::Err(sig_result.error());
    }
    
    // 4. Attach signature to transaction
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{std::move(sig_result.value())};
    
    return Result<void>::Ok();
}

Result<void> sign_transaction_hybrid(Transaction& tx,
                                     const std::vector<uint8_t>& pq_privkey,
                                     const std::vector<uint8_t>& ed25519_privkey,
                                     const std::string& pq_algorithm) {
    // 1. Encode transaction without signatures
    auto encoded_result = codec::encode_for_signing(tx);
    if (encoded_result.is_err()) {
        return Result<void>::Err(encoded_result.error());
    }
    
    // 2. Create domain-separated signing message
    auto msg_result = crypto::create_signing_message(tx.chain_id, encoded_result.value());
    if (msg_result.is_err()) {
        return Result<void>::Err(msg_result.error());
    }
    
    // 3. Sign with both keys
    auto pq_sig_result = crypto::sign(msg_result.value(), pq_privkey, pq_algorithm);
    if (pq_sig_result.is_err()) {
        return Result<void>::Err(pq_sig_result.error());
    }
    
    auto ed25519_sig_result = crypto::ed25519_sign(msg_result.value(), ed25519_privkey);
    if (ed25519_sig_result.is_err()) {
        return Result<void>::Err(ed25519_sig_result.error());
    }
    
    // 4. Attach both signatures to transaction
    tx.auth_mode = AuthMode::Hybrid;
    tx.auth = HybridSignature{std::move(ed25519_sig_result.value()), std::move(pq_sig_result.value())};
    
    return Result<void>::Ok();
}

Result<bool> verify_transaction(const Transaction& tx, uint32_t chain_id) {
    // 1. Encode transaction without signatures
    auto encoded_result = codec::encode_for_signing(tx);
    if (encoded_result.is_err()) {
        return Result<bool>::Err(encoded_result.error());
    }
    
    // 2. Create domain-separated signing message
    auto msg_result = crypto::create_signing_message(chain_id, encoded_result.value());
    if (msg_result.is_err()) {
        return Result<bool>::Err(msg_result.error());
    }
    
    // 3. Verify signature(s)
    if (tx.auth_mode == AuthMode::PqOnly) {
        const auto& pq_sig = std::get<PqSignature>(tx.auth);
        auto verify_result = crypto::verify(msg_result.value(), pq_sig.sig, tx.from_pubkey, "Dilithium3");
        if (verify_result.is_err()) {
            return Result<bool>::Err(verify_result.error());
        }
        return Result<bool>::Ok(verify_result.value());
        
    } else if (tx.auth_mode == AuthMode::Hybrid) {
        const auto& hybrid_sig = std::get<HybridSignature>(tx.auth);
        
        // Verify both signatures
        auto ed25519_result = crypto::ed25519_verify(msg_result.value(), hybrid_sig.classical_sig, tx.from_pubkey);
        if (ed25519_result.is_err()) {
            return Result<bool>::Err(ed25519_result.error());
        }
        if (!ed25519_result.value()) {
            return Result<bool>::Ok(false);
        }
        
        auto pq_result = crypto::verify(msg_result.value(), hybrid_sig.pq_sig, tx.from_pubkey, "Dilithium3");
        if (pq_result.is_err()) {
            return Result<bool>::Err(pq_result.error());
        }
        return Result<bool>::Ok(pq_result.value());
        
    } else {
        return Result<bool>::Err(Error(ErrorCode::InvalidAuthTag, "Unknown auth mode"));
    }
}

} // namespace pqc_ledger::tx

