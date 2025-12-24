#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>

using namespace pqc_ledger;

TEST(Replay, DifferentChainId) {
    // Check if OpenSSL is available
    auto test_hash = crypto::sha256({0x01, 0x02, 0x03});
    if (test_hash.is_err() && 
        test_hash.error().message.find("OpenSSL not available") != std::string::npos) {
        GTEST_SKIP() << "Requires OpenSSL for SHA256";
    }
    
    // Generate keypair
    auto keypair_result = crypto::generate_keypair("Dilithium3");
    ASSERT_TRUE(keypair_result.is_ok()) << "Key generation failed: " << 
        (keypair_result.is_err() ? keypair_result.error().message : "");
    
    const auto& [pubkey, privkey] = keypair_result.value();
    
    // Create and sign transaction for chain_id = 1
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
    
    // Sign transaction for chain_id = 1
    auto sign_result = tx::sign_transaction(tx, privkey, "Dilithium3");
    ASSERT_TRUE(sign_result.is_ok()) << "Signing failed: " << 
        (sign_result.is_err() ? sign_result.error().message : "");
    
    // Try to verify with chain_id = 2 (should fail due to domain separation)
    auto verify_wrong = tx::verify_transaction(tx, 2);
    ASSERT_TRUE(verify_wrong.is_ok()) << "Verification should return result";
    EXPECT_FALSE(verify_wrong.value()) << 
        "Verification with wrong chain_id should fail due to domain separation";
    
    // Verify with correct chain_id = 1 (should succeed)
    auto verify_correct = tx::verify_transaction(tx, 1);
    ASSERT_TRUE(verify_correct.is_ok()) << "Verification failed: " << 
        (verify_correct.is_err() ? verify_correct.error().message : "");
    EXPECT_TRUE(verify_correct.value()) << 
        "Verification with correct chain_id should succeed";
}

TEST(Replay, DomainSeparation) {
    // Test that the signing message includes chain_id
    // This is tested indirectly through the create_signing_message function
    
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = std::vector<uint8_t>(1952, 0x42);
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{std::vector<uint8_t>(3293, 0x55)};
    
    // Encode for signing
    auto encoded1 = codec::encode_for_signing(tx);
    ASSERT_TRUE(encoded1.is_ok());
    
    // Create signing message for chain_id = 1
    auto msg1 = crypto::create_signing_message(1, encoded1.value());
    ASSERT_TRUE(msg1.is_ok());
    
    // Create signing message for chain_id = 2 (same tx data)
    auto msg2 = crypto::create_signing_message(2, encoded1.value());
    ASSERT_TRUE(msg2.is_ok());
    
    // Messages should be different due to domain separation
    EXPECT_NE(msg1.value(), msg2.value()) << 
        "Signing messages for different chain_ids should differ";
}

