#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <algorithm>

using namespace pqc_ledger;

// Helper to create and sign a valid transaction
std::pair<Transaction, std::vector<uint8_t>> create_valid_signed_tx() {
    // Generate keypair
    auto keypair_result = crypto::generate_keypair("Dilithium3");
    if (!keypair_result.is_ok()) {
        return {{}, {}};
    }
    
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
    if (!sign_result.is_ok()) {
        return {{}, {}};
    }
    
    // Encode signed transaction
    auto encoded = codec::encode(tx);
    if (!encoded.is_ok()) {
        return {{}, {}};
    }
    
    return {tx, encoded.value()};
}

// ============================================================================
// Test 1: Round-trip encoding
// ============================================================================

TEST(ValidationTests, RoundTripEncodeDecodeEncode) {
    // Create a test transaction
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = std::vector<uint8_t>(1952, 0x42);  // ML-DSA-65/Dilithium3 pubkey size
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 sig size
    
    // Encode → decode → encode
    auto encoded1 = codec::encode(tx);
    ASSERT_TRUE(encoded1.is_ok()) << "First encoding failed: " << encoded1.error().message;
    
    auto decoded = codec::decode(encoded1.value());
    ASSERT_TRUE(decoded.is_ok()) << "Decoding failed: " << decoded.error().message;
    
    auto encoded2 = codec::encode(decoded.value());
    ASSERT_TRUE(encoded2.is_ok()) << "Second encoding failed: " << encoded2.error().message;
    
    // Must yield identical bytes
    EXPECT_EQ(encoded1.value(), encoded2.value()) 
        << "Round-trip encoding failed: bytes differ";
}

// ============================================================================
// Test 2: Mutation tests - flip one byte in amount/fee/nonce/signature/length prefix
// ============================================================================

TEST(ValidationTests, MutationFlipAmountByte) {
    auto [tx, encoded] = create_valid_signed_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Calculate amount offset: version(1) + chain_id(4) + nonce(8) + pubkey_len(2) + pubkey(1952) + to(32) = 1999
    size_t amount_offset = 1 + 4 + 8 + 2 + 1952 + 32;
    ASSERT_LT(amount_offset + 7, encoded.size()) << "Amount offset out of bounds";
    
    // Flip one byte in amount
    auto mut_data = encoded;
    mut_data[amount_offset] ^= 0xFF;
    
    // Must fail decode or verification
    auto decoded = codec::decode(mut_data);
    if (decoded.is_ok()) {
        // If decode succeeds, verification MUST fail
        auto verify_result = tx::verify_transaction(decoded.value(), 1);
        ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
        EXPECT_FALSE(verify_result.value()) 
            << "Mutated amount should cause signature verification to fail";
    } else {
        // Decoding failed (strict validation caught it) - also acceptable
        EXPECT_TRUE(decoded.is_err());
    }
}

TEST(ValidationTests, MutationFlipFeeByte) {
    auto [tx, encoded] = create_valid_signed_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Fee starts after amount (8 bytes)
    size_t fee_offset = 1 + 4 + 8 + 2 + 1952 + 32 + 8;
    ASSERT_LT(fee_offset + 7, encoded.size()) << "Fee offset out of bounds";
    
    // Flip one byte in fee
    auto mut_data = encoded;
    mut_data[fee_offset] ^= 0xFF;
    
    // Must fail decode or verification
    auto decoded = codec::decode(mut_data);
    if (decoded.is_ok()) {
        auto verify_result = tx::verify_transaction(decoded.value(), 1);
        ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
        EXPECT_FALSE(verify_result.value()) 
            << "Mutated fee should cause signature verification to fail";
    } else {
        EXPECT_TRUE(decoded.is_err());
    }
}

TEST(ValidationTests, MutationFlipNonceByte) {
    auto [tx, encoded] = create_valid_signed_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Nonce starts after version(1) + chain_id(4) = 5
    size_t nonce_offset = 5;
    ASSERT_LT(nonce_offset + 7, encoded.size()) << "Nonce offset out of bounds";
    
    // Flip one byte in nonce
    auto mut_data = encoded;
    mut_data[nonce_offset] ^= 0xFF;
    
    // Must fail decode or verification
    auto decoded = codec::decode(mut_data);
    if (decoded.is_ok()) {
        auto verify_result = tx::verify_transaction(decoded.value(), 1);
        ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
        EXPECT_FALSE(verify_result.value()) 
            << "Mutated nonce should cause signature verification to fail";
    } else {
        EXPECT_TRUE(decoded.is_err());
    }
}

TEST(ValidationTests, MutationFlipSignatureByte) {
    auto [tx, encoded] = create_valid_signed_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Flip one byte in signature (at the end)
    ASSERT_GT(encoded.size(), 10) << "Encoded transaction too short";
    auto mut_data = encoded;
    mut_data[mut_data.size() - 10] ^= 0xFF;
    
    // Should decode (signature is just data), but verification MUST fail
    auto decoded = codec::decode(mut_data);
    ASSERT_TRUE(decoded.is_ok()) << "Decoding should succeed (signature is just data)";
    
    auto verify_result = tx::verify_transaction(decoded.value(), 1);
    ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
    EXPECT_FALSE(verify_result.value()) 
        << "Mutated signature should fail verification";
}

TEST(ValidationTests, MutationFlipLengthPrefix) {
    // Create a test transaction (doesn't need to be signed for this test)
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
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 sig size
    
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    // Length prefix for pubkey is after version(1) + chain_id(4) + nonce(8) = 13
    size_t len_prefix_offset = 13;
    ASSERT_LT(len_prefix_offset + 1, encoded.value().size()) 
        << "Length prefix offset out of bounds";
    
    // Flip length prefix
    auto mut_data = encoded.value();
    mut_data[len_prefix_offset] ^= 0xFF;
    
    // Must fail decode
    auto decoded = codec::decode(mut_data);
    EXPECT_TRUE(decoded.is_err()) 
        << "Invalid length prefix should cause decode failure";
    if (decoded.is_err()) {
        EXPECT_TRUE(decoded.error().code == ErrorCode::MismatchedLength ||
                   decoded.error().code == ErrorCode::InvalidLengthPrefix) 
            << "Should return length-related error";
    }
}

// ============================================================================
// Test 3: Trailing bytes - append garbage → decode must fail
// ============================================================================

TEST(ValidationTests, TrailingBytesMustFail) {
    // Create a test transaction
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
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 sig size
    
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    // Append garbage bytes
    auto mut_data = encoded.value();
    mut_data.push_back(0x42);  // Add trailing byte
    mut_data.push_back(0xAA);
    mut_data.push_back(0xFF);
    
    // Decode must fail
    auto decoded = codec::decode(mut_data);
    EXPECT_TRUE(decoded.is_err()) 
        << "Trailing bytes should cause decode failure";
    if (decoded.is_err()) {
        EXPECT_EQ(decoded.error().code, ErrorCode::TrailingBytes) 
            << "Should return TrailingBytes error";
    }
}

// ============================================================================
// Test 4: Chain replay - verify same tx under different chain_id → must fail
// ============================================================================

TEST(ValidationTests, ChainReplayMustFail) {
    // Generate keypair
    auto keypair_result = crypto::generate_keypair("Dilithium3");
    ASSERT_TRUE(keypair_result.is_ok()) 
        << "Key generation failed: " << 
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
    ASSERT_TRUE(sign_result.is_ok()) 
        << "Signing failed: " << 
        (sign_result.is_err() ? sign_result.error().message : "");
    
    // Verify with correct chain_id = 1 (should succeed)
    auto verify_correct = tx::verify_transaction(tx, 1);
    ASSERT_TRUE(verify_correct.is_ok()) 
        << "Verification failed: " << 
        (verify_correct.is_err() ? verify_correct.error().message : "");
    EXPECT_TRUE(verify_correct.value()) 
        << "Verification with correct chain_id should succeed";
    
    // Verify with different chain_id = 2 (must fail due to domain separation)
    auto verify_wrong = tx::verify_transaction(tx, 2);
    ASSERT_TRUE(verify_wrong.is_ok()) 
        << "Verification should return result";
    EXPECT_FALSE(verify_wrong.value()) 
        << "Verification with different chain_id must fail (replay prevention)";
    
    // Verify with different chain_id = 999 (must fail)
    auto verify_wrong2 = tx::verify_transaction(tx, 999);
    ASSERT_TRUE(verify_wrong2.is_ok()) 
        << "Verification should return result";
    EXPECT_FALSE(verify_wrong2.value()) 
        << "Verification with different chain_id must fail (replay prevention)";
}

