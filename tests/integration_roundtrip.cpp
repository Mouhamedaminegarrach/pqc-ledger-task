#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>

using namespace pqc_ledger;

TEST(IntegrationRoundtrip, EncodeDecodeEncode) {
    // Create a test transaction
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = std::vector<uint8_t>(1952, 0x42);  // Dilithium3 pubkey size
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 sig size
    
    // Encode
    auto encoded1 = codec::encode(tx);
    ASSERT_TRUE(encoded1.is_ok()) << "Encoding failed: " << encoded1.error().message;
    
    // Decode
    auto decoded = codec::decode(encoded1.value());
    ASSERT_TRUE(decoded.is_ok()) << "Decoding failed: " << decoded.error().message;
    
    // Encode again
    auto encoded2 = codec::encode(decoded.value());
    ASSERT_TRUE(encoded2.is_ok()) << "Re-encoding failed: " << encoded2.error().message;
    
    // Should be identical
    EXPECT_EQ(encoded1.value(), encoded2.value()) << "Round-trip encoding failed: bytes differ";
}

TEST(IntegrationRoundtrip, SignVerify) {
    // SHA256 is now always available via picosha2
    
    // Generate keypair
    auto keypair_result = crypto::generate_keypair("Dilithium3");
    ASSERT_TRUE(keypair_result.is_ok()) << "Key generation failed: " << 
        (keypair_result.is_err() ? keypair_result.error().message : "");
    
    const auto& [pubkey, privkey] = keypair_result.value();
    
    // Create unsigned transaction
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = pubkey;
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{{}};  // Empty signature for unsigned tx
    
    // Sign transaction
    auto sign_result = tx::sign_transaction(tx, privkey, "Dilithium3");
    ASSERT_TRUE(sign_result.is_ok()) << "Signing failed: " << 
        (sign_result.is_err() ? sign_result.error().message : "");
    
    // Verify transaction with correct chain_id
    auto verify_result = tx::verify_transaction(tx, 1);
    ASSERT_TRUE(verify_result.is_ok()) << "Verification failed: " << 
        (verify_result.is_err() ? verify_result.error().message : "");
    EXPECT_TRUE(verify_result.value()) << "Valid signature should verify";
    
    // Mutate signature and verify it fails
    auto& pq_sig = std::get<PqSignature>(tx.auth);
    if (!pq_sig.sig.empty()) {
        pq_sig.sig[0] ^= 0xFF;  // Flip a bit in signature
        
        auto verify_fail = tx::verify_transaction(tx, 1);
        ASSERT_TRUE(verify_fail.is_ok()) << "Verification should return result even for invalid signature";
        EXPECT_FALSE(verify_fail.value()) << "Mutated signature should fail verification";
    }
}

