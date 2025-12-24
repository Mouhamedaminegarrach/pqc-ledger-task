#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <algorithm>

using namespace pqc_ledger;

// SHA256 is now always available via picosha2, no need to check

// Helper to create and sign a valid transaction
std::pair<Transaction, std::vector<uint8_t>> create_and_sign_tx() {
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

TEST(Mutation, FlipAmountByte) {
    
    // Create and sign a valid transaction
    auto [tx, encoded] = create_and_sign_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Find and flip a byte in the amount field
    // Amount starts after: 1 (version) + 4 (chain_id) + 8 (nonce) + 2+1952 (pubkey) + 32 (to) = 1999
    size_t amount_offset = 1 + 4 + 8 + 2 + 1952 + 32;
    ASSERT_LT(amount_offset, encoded.size()) << "Amount offset out of bounds";
    
    auto mut_data = encoded;
    mut_data[amount_offset] ^= 0xFF;  // Flip all bits
    
    // Decode the mutated transaction
    auto decoded = codec::decode(mut_data);
    
    // If decode succeeds, signature verification MUST fail
    if (decoded.is_ok()) {
        auto verify_result = tx::verify_transaction(decoded.value(), 1);
        ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
        EXPECT_FALSE(verify_result.value()) << 
            "Mutated amount should cause signature verification to fail";
    } else {
        // Decoding failed, which is also acceptable (strict decoding caught the mutation)
        EXPECT_TRUE(true);
    }
}

TEST(Mutation, FlipFeeByte) {
    
    // Create and sign a valid transaction
    auto [tx, encoded] = create_and_sign_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Fee starts after amount (8 bytes)
    size_t fee_offset = 1 + 4 + 8 + 2 + 1952 + 32 + 8;
    ASSERT_LT(fee_offset, encoded.size()) << "Fee offset out of bounds";
    
    auto mut_data = encoded;
    mut_data[fee_offset] ^= 0xFF;
    
    // Decode the mutated transaction
    auto decoded = codec::decode(mut_data);
    
    // If decode succeeds, signature verification MUST fail
    if (decoded.is_ok()) {
        auto verify_result = tx::verify_transaction(decoded.value(), 1);
        ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
        EXPECT_FALSE(verify_result.value()) << 
            "Mutated fee should cause signature verification to fail";
    } else {
        // Decoding failed, which is also acceptable
        EXPECT_TRUE(true);
    }
}

TEST(Mutation, FlipNonceByte) {
    
    // Create and sign a valid transaction
    auto [tx, encoded] = create_and_sign_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Nonce starts after version (1) + chain_id (4) = 5
    size_t nonce_offset = 5;
    ASSERT_LT(nonce_offset + 7, encoded.size()) << "Nonce offset out of bounds";
    
    auto mut_data = encoded;
    mut_data[nonce_offset] ^= 0xFF;
    
    // Decode the mutated transaction
    auto decoded = codec::decode(mut_data);
    
    // If decode succeeds, signature verification MUST fail
    if (decoded.is_ok()) {
        auto verify_result = tx::verify_transaction(decoded.value(), 1);
        ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
        EXPECT_FALSE(verify_result.value()) << 
            "Mutated nonce should cause signature verification to fail";
    } else {
        // Decoding failed, which is also acceptable
        EXPECT_TRUE(true);
    }
}

TEST(Mutation, FlipSignatureByte) {
    
    // Create and sign a valid transaction
    auto [tx, encoded] = create_and_sign_tx();
    ASSERT_FALSE(encoded.empty()) << "Failed to create and sign transaction";
    
    // Signature is at the end - flip a byte in the signature
    ASSERT_GT(encoded.size(), 10) << "Encoded transaction too short";
    auto mut_data = encoded;
    mut_data[mut_data.size() - 10] ^= 0xFF;  // Flip a byte in signature
    
    // Should decode (signature is just data), but verification MUST fail
    auto decoded = codec::decode(mut_data);
    ASSERT_TRUE(decoded.is_ok()) << "Decoding should succeed (signature is just data)";
    
    auto verify_result = tx::verify_transaction(decoded.value(), 1);
    ASSERT_TRUE(verify_result.is_ok()) << "Verification should return result";
    EXPECT_FALSE(verify_result.value()) << "Mutated signature should fail verification";
}

TEST(Mutation, FlipLengthPrefix) {
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
    
    auto mut_data = encoded.value();
    // Length prefix for pubkey is after version (1) + chain_id (4) + nonce (8) = 13
    size_t len_prefix_offset = 13;
    ASSERT_LT(len_prefix_offset + 1, mut_data.size()) << "Length prefix offset out of bounds";
    
    mut_data[len_prefix_offset] ^= 0xFF;  // Flip length prefix
    
    auto decoded = codec::decode(mut_data);
    EXPECT_TRUE(decoded.is_err()) << "Invalid length prefix should cause decode failure";
    if (decoded.is_err()) {
        // Could be MismatchedLength (if length exceeds buffer) or InvalidLengthPrefix
        EXPECT_TRUE(decoded.error().code == ErrorCode::MismatchedLength ||
                   decoded.error().code == ErrorCode::InvalidLengthPrefix) << 
            "Should return length-related error";
    }
}

TEST(Mutation, TrailingBytes) {
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
    
    auto mut_data = encoded.value();
    mut_data.push_back(0x42);  // Add trailing byte
    mut_data.push_back(0xAA);
    
    auto decoded = codec::decode(mut_data);
    EXPECT_TRUE(decoded.is_err()) << "Trailing bytes should cause decode failure";
    if (decoded.is_err()) {
        EXPECT_EQ(decoded.error().code, ErrorCode::TrailingBytes) << 
            "Should return TrailingBytes error";
    }
}

TEST(Mutation, WrongPubkeySize) {
    // Create a test transaction with wrong pubkey size
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = std::vector<uint8_t>(1000, 0x42);  // Wrong size (should be 1952 for Dilithium3)
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 sig size
    
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto decoded = codec::decode(encoded.value());
    EXPECT_TRUE(decoded.is_err()) << "Wrong pubkey size should cause decode failure";
    if (decoded.is_err()) {
        EXPECT_EQ(decoded.error().code, ErrorCode::InvalidPublicKey) << 
            "Should return InvalidPublicKey error";
    }
}

TEST(Mutation, WrongSignatureSize) {
    // Create a test transaction with wrong signature size
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
    tx.auth = PqSignature{std::vector<uint8_t>(1000, 0x55)};  // Wrong size (should be 3309 for ML-DSA-65)
    
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto decoded = codec::decode(encoded.value());
    EXPECT_TRUE(decoded.is_err()) << "Wrong signature size should cause decode failure";
    if (decoded.is_err()) {
        EXPECT_EQ(decoded.error().code, ErrorCode::InvalidSignature) << 
            "Should return InvalidSignature error";
    }
}

TEST(Mutation, WrongEd25519SignatureSize) {
    // Create hybrid transaction
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = std::vector<uint8_t>(1952, 0x42);
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::Hybrid;
    // Ed25519 signature should be 64 bytes, but we use wrong size
    tx.auth = HybridSignature{
        std::vector<uint8_t>(32, 0x11),  // Wrong size (should be 64)
        std::vector<uint8_t>(3309, 0x22)  // Correct ML-DSA-65 sig size
    };
    
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto decoded = codec::decode(encoded.value());
    EXPECT_TRUE(decoded.is_err()) << "Wrong Ed25519 signature size should cause decode failure";
    if (decoded.is_err()) {
        EXPECT_EQ(decoded.error().code, ErrorCode::InvalidSignature) << 
            "Should return InvalidSignature error";
    }
}

TEST(Mutation, LengthPrefixExceedsBuffer) {
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
    
    auto mut_data = encoded.value();
    // Find the length prefix for pubkey (after version, chain_id, nonce = 13 bytes)
    size_t len_prefix_offset = 13;
    ASSERT_LT(len_prefix_offset + 1, mut_data.size()) << "Length prefix offset out of bounds";
    
    // Set length prefix to a value larger than remaining buffer
    uint16_t huge_len = static_cast<uint16_t>(mut_data.size() + 1000);
    mut_data[len_prefix_offset] = static_cast<uint8_t>((huge_len >> 8) & 0xFF);
    mut_data[len_prefix_offset + 1] = static_cast<uint8_t>(huge_len & 0xFF);
    
    auto decoded = codec::decode(mut_data);
    EXPECT_TRUE(decoded.is_err()) << 
        "Length prefix exceeding buffer should cause decode failure";
    if (decoded.is_err()) {
        // Should return MismatchedLength or InvalidLengthPrefix
        EXPECT_TRUE(decoded.error().code == ErrorCode::MismatchedLength ||
                   decoded.error().code == ErrorCode::InvalidLengthPrefix) << 
            "Should return length-related error";
    }
}

