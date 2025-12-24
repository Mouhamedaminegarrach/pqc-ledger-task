#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <string>

using namespace pqc_ledger;

// Test hex encoding/decoding
TEST(CodecHex, EncodeDecodeRoundTrip) {
    // Test various byte sequences
    std::vector<std::vector<uint8_t>> test_cases = {
        {},  // Empty
        {0x00},
        {0xFF},
        {0x00, 0xFF},
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    };
    
    for (const auto& original : test_cases) {
        // Encode to hex
        std::string hex = codec::encode_to_hex(original);
        
        // Decode from hex
        auto decoded_result = codec::decode_from_hex(hex);
        ASSERT_TRUE(decoded_result.is_ok()) << "Failed to decode hex: " << hex;
        
        // Should match original
        EXPECT_EQ(decoded_result.value(), original) << "Round-trip failed for hex: " << hex;
    }
}

TEST(CodecHex, EncodeKnownValues) {
    // Test known hex encodings
    EXPECT_EQ(codec::encode_to_hex({0x00}), "00");
    EXPECT_EQ(codec::encode_to_hex({0xFF}), "ff");
    EXPECT_EQ(codec::encode_to_hex({0x00, 0xFF}), "00ff");
    EXPECT_EQ(codec::encode_to_hex({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}), 
              "0123456789abcdef");
}

TEST(CodecHex, DecodeKnownValues) {
    // Test known hex decodings
    auto result1 = codec::decode_from_hex("00");
    ASSERT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.value(), std::vector<uint8_t>{0x00});
    
    auto result2 = codec::decode_from_hex("ff");
    ASSERT_TRUE(result2.is_ok());
    EXPECT_EQ(result2.value(), std::vector<uint8_t>{0xFF});
    
    auto result3 = codec::decode_from_hex("0123456789abcdef");
    ASSERT_TRUE(result3.is_ok());
    EXPECT_EQ(result3.value(), std::vector<uint8_t>({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}));
}

TEST(CodecHex, DecodeCaseInsensitive) {
    // Hex should be case-insensitive
    auto upper = codec::decode_from_hex("ABCDEF");
    auto lower = codec::decode_from_hex("abcdef");
    
    ASSERT_TRUE(upper.is_ok());
    ASSERT_TRUE(lower.is_ok());
    EXPECT_EQ(upper.value(), lower.value());
}

TEST(CodecHex, DecodeWithWhitespace) {
    // Should handle whitespace
    auto result1 = codec::decode_from_hex("00 FF");
    ASSERT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.value(), std::vector<uint8_t>{0x00, 0xFF});
    
    auto result2 = codec::decode_from_hex("01\n23\t45\r67");
    ASSERT_TRUE(result2.is_ok());
    EXPECT_EQ(result2.value(), std::vector<uint8_t>({0x01, 0x23, 0x45, 0x67}));
}

TEST(CodecHex, DecodeInvalid) {
    // Invalid hex strings should fail
    EXPECT_TRUE(codec::decode_from_hex("G").is_err());  // Invalid character
    EXPECT_TRUE(codec::decode_from_hex("0").is_err());  // Odd length
    EXPECT_TRUE(codec::decode_from_hex("0G").is_err());  // Invalid character
}

// Test base64 encoding/decoding
TEST(CodecBase64, EncodeDecodeRoundTrip) {
    // Test various byte sequences
    std::vector<std::vector<uint8_t>> test_cases = {
        {},  // Empty
        {0x00},
        {0xFF},
        {0x00, 0xFF},
        {0x01, 0x02, 0x03},  // Exactly 3 bytes (no padding)
        {0x01, 0x02},  // 2 bytes (1 padding)
        {0x01},  // 1 byte (2 padding)
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    };
    
    for (const auto& original : test_cases) {
        // Encode to base64
        std::string base64 = codec::encode_to_base64(original);
        
        // Decode from base64
        auto decoded_result = codec::decode_from_base64(base64);
        ASSERT_TRUE(decoded_result.is_ok()) << "Failed to decode base64: " << base64;
        
        // Should match original
        EXPECT_EQ(decoded_result.value(), original) << "Round-trip failed for base64: " << base64;
    }
}

TEST(CodecBase64, EncodeKnownValues) {
    // Test RFC 4648 test vectors
    EXPECT_EQ(codec::encode_to_base64({}), "");
    EXPECT_EQ(codec::encode_to_base64({0x66}), "Zg==");
    EXPECT_EQ(codec::encode_to_base64({0x66, 0x6F}), "Zm8=");
    EXPECT_EQ(codec::encode_to_base64({0x66, 0x6F, 0x6F}), "Zm9v");
    EXPECT_EQ(codec::encode_to_base64({0x66, 0x6F, 0x6F, 0x62}), "Zm9vYg==");
    EXPECT_EQ(codec::encode_to_base64({0x66, 0x6F, 0x6F, 0x62, 0x61}), "Zm9vYmE=");
    EXPECT_EQ(codec::encode_to_base64({0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72}), "Zm9vYmFy");
}

TEST(CodecBase64, DecodeKnownValues) {
    // Test RFC 4648 test vectors
    auto result1 = codec::decode_from_base64("");
    ASSERT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.value(), std::vector<uint8_t>{});
    
    auto result2 = codec::decode_from_base64("Zg==");
    ASSERT_TRUE(result2.is_ok());
    EXPECT_EQ(result2.value(), std::vector<uint8_t>{0x66});
    
    auto result3 = codec::decode_from_base64("Zm8=");
    ASSERT_TRUE(result3.is_ok());
    EXPECT_EQ(result3.value(), std::vector<uint8_t>({0x66, 0x6F}));
    
    auto result4 = codec::decode_from_base64("Zm9v");
    ASSERT_TRUE(result4.is_ok());
    EXPECT_EQ(result4.value(), std::vector<uint8_t>({0x66, 0x6F, 0x6F}));
    
    auto result5 = codec::decode_from_base64("Zm9vYg==");
    ASSERT_TRUE(result5.is_ok());
    EXPECT_EQ(result5.value(), std::vector<uint8_t>({0x66, 0x6F, 0x6F, 0x62}));
    
    auto result6 = codec::decode_from_base64("Zm9vYmE=");
    ASSERT_TRUE(result6.is_ok());
    EXPECT_EQ(result6.value(), std::vector<uint8_t>({0x66, 0x6F, 0x6F, 0x62, 0x61}));
    
    auto result7 = codec::decode_from_base64("Zm9vYmFy");
    ASSERT_TRUE(result7.is_ok());
    EXPECT_EQ(result7.value(), std::vector<uint8_t>({0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72}));
}

TEST(CodecBase64, DecodeWithWhitespace) {
    // Should handle whitespace
    auto result1 = codec::decode_from_base64("Zg ==");
    ASSERT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.value(), std::vector<uint8_t>{0x66});
    
    auto result2 = codec::decode_from_base64("Zm\n9v\tYm\rFy");
    ASSERT_TRUE(result2.is_ok());
    EXPECT_EQ(result2.value(), std::vector<uint8_t>({0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72}));
}

TEST(CodecBase64, DecodeInvalid) {
    // Invalid base64 strings should fail
    EXPECT_TRUE(codec::decode_from_base64("Zg=").is_err());  // Invalid padding (only 1 char before =)
    EXPECT_TRUE(codec::decode_from_base64("Z").is_err());  // Incomplete group
    EXPECT_TRUE(codec::decode_from_base64("Zg").is_err());  // Incomplete group
    EXPECT_TRUE(codec::decode_from_base64("Zg!=").is_err());  // Invalid character
}

// Test transaction encoding/decoding with hex/base64
TEST(CodecTransaction, EncodeDecodeHexRoundTrip) {
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
    
    // Encode to hex
    auto hex_result = codec::encode_to_hex(tx);
    ASSERT_TRUE(hex_result.is_ok());
    
    // Decode from hex
    auto decoded_result = codec::decode_from_hex(hex_result.value());
    ASSERT_TRUE(decoded_result.is_ok());
    
    // Encode again
    auto hex_result2 = codec::encode_to_hex(decoded_result.value());
    ASSERT_TRUE(hex_result2.is_ok());
    
    // Should be identical
    EXPECT_EQ(hex_result.value(), hex_result2.value());
}

TEST(CodecTransaction, EncodeDecodeBase64RoundTrip) {
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
    
    // Encode to base64
    auto base64_result = codec::encode_to_base64(tx);
    ASSERT_TRUE(base64_result.is_ok());
    
    // Decode from base64
    auto decoded_result = codec::decode_from_base64(base64_result.value());
    ASSERT_TRUE(decoded_result.is_ok());
    
    // Encode again
    auto base64_result2 = codec::encode_to_base64(decoded_result.value());
    ASSERT_TRUE(base64_result2.is_ok());
    
    // Should be identical
    EXPECT_EQ(base64_result.value(), base64_result2.value());
}

TEST(CodecTransaction, HexBase64Equivalence) {
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
    
    // Encode to binary
    auto binary_result = codec::encode(tx);
    ASSERT_TRUE(binary_result.is_ok());
    
    // Encode binary to hex
    std::string hex = codec::encode_to_hex(binary_result.value());
    
    // Encode binary to base64
    std::string base64 = codec::encode_to_base64(binary_result.value());
    
    // Decode hex and base64
    auto decoded_hex = codec::decode_from_hex(hex);
    auto decoded_base64 = codec::decode_from_base64(base64);
    
    ASSERT_TRUE(decoded_hex.is_ok());
    ASSERT_TRUE(decoded_base64.is_ok());
    
    // Both should decode to the same transaction
    auto encoded_hex = codec::encode(decoded_hex.value());
    auto encoded_base64 = codec::encode(decoded_base64.value());
    
    ASSERT_TRUE(encoded_hex.is_ok());
    ASSERT_TRUE(encoded_base64.is_ok());
    
    EXPECT_EQ(encoded_hex.value(), encoded_base64.value());
    EXPECT_EQ(encoded_hex.value(), binary_result.value());
}

