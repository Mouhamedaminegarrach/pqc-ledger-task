#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <algorithm>

using namespace pqc_ledger;

Transaction create_test_tx() {
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
    return tx;
}

TEST(Mutation, FlipAmountByte) {
    auto tx = create_test_tx();
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto mut_data = encoded.value();
    // Find and flip a byte in the amount field (after version, chain_id, nonce, pubkey, to)
    // Amount starts after: 1 (version) + 4 (chain_id) + 8 (nonce) + 2+1952 (pubkey) + 32 (to) = 1999
    size_t amount_offset = 1 + 4 + 8 + 2 + 1952 + 32;
    if (amount_offset + 8 <= mut_data.size()) {
        mut_data[amount_offset] ^= 0xFF;  // Flip all bits
        
        auto decoded = codec::decode(mut_data);
        // Should either fail to decode, or if it decodes, verification should fail
        if (decoded.is_ok()) {
            // If it decodes, the amount is wrong, so structure might be invalid
            EXPECT_FALSE(tx::is_valid_structure(decoded.value())) << "Mutated amount should be detected";
        } else {
            // Decoding failed, which is also acceptable
            EXPECT_TRUE(true);
        }
    }
}

TEST(Mutation, FlipFeeByte) {
    auto tx = create_test_tx();
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto mut_data = encoded.value();
    // Fee starts after amount (8 bytes)
    size_t fee_offset = 1 + 4 + 8 + 2 + 1952 + 32 + 8;
    if (fee_offset + 8 <= mut_data.size()) {
        mut_data[fee_offset] ^= 0xFF;
        
        auto decoded = codec::decode(mut_data);
        if (decoded.is_ok()) {
            EXPECT_FALSE(tx::is_valid_structure(decoded.value())) << "Mutated fee should be detected";
        }
    }
}

TEST(Mutation, FlipNonceByte) {
    auto tx = create_test_tx();
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto mut_data = encoded.value();
    // Nonce starts after version (1) + chain_id (4) = 5
    size_t nonce_offset = 5;
    if (nonce_offset + 8 <= mut_data.size()) {
        mut_data[nonce_offset] ^= 0xFF;
        
        auto decoded = codec::decode(mut_data);
        if (decoded.is_ok()) {
            EXPECT_FALSE(tx::is_valid_structure(decoded.value())) << "Mutated nonce should be detected";
        }
    }
}

TEST(Mutation, FlipSignatureByte) {
    auto tx = create_test_tx();
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto mut_data = encoded.value();
    // Signature is at the end
    if (mut_data.size() > 10) {
        mut_data[mut_data.size() - 10] ^= 0xFF;  // Flip a byte in signature
        
        auto decoded = codec::decode(mut_data);
        // Should decode (signature is just data), but verification should fail
        if (decoded.is_ok()) {
            auto verify_result = tx::verify_transaction(decoded.value(), 1);
            if (verify_result.is_ok()) {
                EXPECT_FALSE(verify_result.value()) << "Mutated signature should fail verification";
            }
        }
    }
}

TEST(Mutation, FlipLengthPrefix) {
    auto tx = create_test_tx();
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto mut_data = encoded.value();
    // Length prefix for pubkey is after version (1) + chain_id (4) + nonce (8) = 13
    size_t len_prefix_offset = 13;
    if (len_prefix_offset + 2 <= mut_data.size()) {
        mut_data[len_prefix_offset] ^= 0xFF;  // Flip length prefix
        
        auto decoded = codec::decode(mut_data);
        EXPECT_TRUE(decoded.is_err()) << "Invalid length prefix should cause decode failure";
        if (decoded.is_err()) {
            EXPECT_EQ(decoded.error().code, ErrorCode::InvalidLengthPrefix) << 
                "Should return InvalidLengthPrefix error";
        }
    }
}

TEST(Mutation, TrailingBytes) {
    auto tx = create_test_tx();
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
    auto tx = create_test_tx();
    // Change pubkey to wrong size (should be 1952 for Dilithium3)
    tx.from_pubkey = std::vector<uint8_t>(1000, 0x42);  // Wrong size
    
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
    auto tx = create_test_tx();
    // Change signature to wrong size (should be 3293 for Dilithium3)
    tx.auth = PqSignature{std::vector<uint8_t>(1000, 0x55)};  // Wrong size
    
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
        std::vector<uint8_t>(3293, 0x22)  // Correct PQ sig size
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
    auto tx = create_test_tx();
    auto encoded = codec::encode(tx);
    ASSERT_TRUE(encoded.is_ok());
    
    auto mut_data = encoded.value();
    // Find the length prefix for pubkey (after version, chain_id, nonce = 13 bytes)
    size_t len_prefix_offset = 13;
    if (len_prefix_offset + 2 <= mut_data.size()) {
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
}

