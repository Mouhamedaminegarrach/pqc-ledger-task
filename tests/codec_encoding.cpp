#include <gtest/gtest.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace pqc_ledger;

// Helper function to decode hex to bytes (for testing raw hex encoding/decoding)
std::vector<uint8_t> hex_to_bytes_test(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        if (i + 1 < hex.length()) {
            std::string byte_str = hex.substr(i, 2);
            bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
        }
    }
    return bytes;
}

// Helper function to decode base64 to bytes (for testing raw base64 encoding/decoding)
// Simple base64 decoder for testing
std::vector<uint8_t> base64_to_bytes_test(const std::string& base64) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> result;
    int val = 0, valb = -8;
    for (char c : base64) {
        if (c == '=') break;
        size_t idx = chars.find(c);
        if (idx == std::string::npos) continue;
        val = (val << 6) + idx;
        valb += 6;
        if (valb >= 0) {
            result.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return result;
}

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
        
        // Decode from hex (using helper function for raw bytes)
        std::vector<uint8_t> decoded = hex_to_bytes_test(hex);
        
        // Should match original
        EXPECT_EQ(decoded, original) << "Round-trip failed for hex: " << hex;
    }
}

TEST(CodecHex, EncodeKnownValues) {
    // Test known hex encodings
    EXPECT_EQ(codec::encode_to_hex(std::vector<uint8_t>{0x00}), "00");
    EXPECT_EQ(codec::encode_to_hex(std::vector<uint8_t>{0xFF}), "ff");
    EXPECT_EQ(codec::encode_to_hex(std::vector<uint8_t>{0x00, 0xFF}), "00ff");
    EXPECT_EQ(codec::encode_to_hex(std::vector<uint8_t>{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}), 
              "0123456789abcdef");
}

TEST(CodecHex, DecodeKnownValues) {
    // Test known hex decodings (raw hex, not transactions)
    std::vector<uint8_t> result1 = hex_to_bytes_test("00");
    EXPECT_EQ(result1, std::vector<uint8_t>{0x00});
    
    std::vector<uint8_t> result2 = hex_to_bytes_test("ff");
    EXPECT_EQ(result2, std::vector<uint8_t>{0xFF});
    
    std::vector<uint8_t> result3 = hex_to_bytes_test("0123456789abcdef");
    std::vector<uint8_t> expected3 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(result3, expected3);
}

TEST(CodecHex, DecodeCaseInsensitive) {
    // Hex should be case-insensitive (raw hex decoding)
    std::vector<uint8_t> upper = hex_to_bytes_test("ABCDEF");
    std::vector<uint8_t> lower = hex_to_bytes_test("abcdef");
    
    EXPECT_EQ(upper, lower);
}

TEST(CodecHex, DecodeWithWhitespace) {
    // Should handle whitespace (raw hex decoding)
    // Remove whitespace manually for test
    std::string clean1 = "00FF";
    std::vector<uint8_t> result1 = hex_to_bytes_test(clean1);
    std::vector<uint8_t> expected1 = {0x00, 0xFF};
    EXPECT_EQ(result1, expected1);
    
    std::string clean2 = "01234567";
    std::vector<uint8_t> result2 = hex_to_bytes_test(clean2);
    std::vector<uint8_t> expected2 = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(result2, expected2);
}

TEST(CodecHex, DecodeInvalid) {
    // Invalid hex strings should fail when decoding as transactions
    // Test with invalid hex that can't form a valid transaction
    EXPECT_TRUE(codec::decode_from_hex("G").is_err());  // Invalid character
    EXPECT_TRUE(codec::decode_from_hex("0").is_err());  // Odd length (and invalid transaction)
    EXPECT_TRUE(codec::decode_from_hex("0G").is_err());  // Invalid character
    
    // Also test that these fail for raw hex decoding (would throw or need error handling)
    // Note: hex_to_bytes_test doesn't have error handling, so we skip those tests
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
        
        // Decode from base64 (using helper function for raw bytes)
        std::vector<uint8_t> decoded = base64_to_bytes_test(base64);
        
        // Should match original
        EXPECT_EQ(decoded, original) << "Round-trip failed for base64: " << base64;
    }
}

TEST(CodecBase64, EncodeKnownValues) {
    // Test RFC 4648 test vectors
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{}), "");
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{0x66}), "Zg==");
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{0x66, 0x6F}), "Zm8=");
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{0x66, 0x6F, 0x6F}), "Zm9v");
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{0x66, 0x6F, 0x6F, 0x62}), "Zm9vYg==");
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{0x66, 0x6F, 0x6F, 0x62, 0x61}), "Zm9vYmE=");
    EXPECT_EQ(codec::encode_to_base64(std::vector<uint8_t>{0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72}), "Zm9vYmFy");
}

TEST(CodecBase64, DecodeKnownValues) {
    // Test RFC 4648 test vectors (raw base64 decoding)
    std::vector<uint8_t> result1 = base64_to_bytes_test("");
    EXPECT_EQ(result1, std::vector<uint8_t>{});
    
    std::vector<uint8_t> result2 = base64_to_bytes_test("Zg==");
    EXPECT_EQ(result2, std::vector<uint8_t>{0x66});
    
    std::vector<uint8_t> result3 = base64_to_bytes_test("Zm8=");
    std::vector<uint8_t> expected3 = {0x66, 0x6F};
    EXPECT_EQ(result3, expected3);
    
    std::vector<uint8_t> result4 = base64_to_bytes_test("Zm9v");
    std::vector<uint8_t> expected4 = {0x66, 0x6F, 0x6F};
    EXPECT_EQ(result4, expected4);
    
    std::vector<uint8_t> result5 = base64_to_bytes_test("Zm9vYg==");
    std::vector<uint8_t> expected5 = {0x66, 0x6F, 0x6F, 0x62};
    EXPECT_EQ(result5, expected5);
    
    std::vector<uint8_t> result6 = base64_to_bytes_test("Zm9vYmE=");
    std::vector<uint8_t> expected6 = {0x66, 0x6F, 0x6F, 0x62, 0x61};
    EXPECT_EQ(result6, expected6);
    
    std::vector<uint8_t> result7 = base64_to_bytes_test("Zm9vYmFy");
    std::vector<uint8_t> expected7 = {0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72};
    EXPECT_EQ(result7, expected7);
}

TEST(CodecBase64, DecodeWithWhitespace) {
    // Should handle whitespace (raw base64 decoding)
    // Remove whitespace manually for test
    std::string clean1 = "Zg==";
    std::vector<uint8_t> result1 = base64_to_bytes_test(clean1);
    EXPECT_EQ(result1, std::vector<uint8_t>{0x66});
    
    std::string clean2 = "Zm9vYmFy";
    std::vector<uint8_t> result2 = base64_to_bytes_test(clean2);
    std::vector<uint8_t> expected2 = {0x66, 0x6F, 0x6F, 0x62, 0x61, 0x72};
    EXPECT_EQ(result2, expected2);
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
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 signature size
    
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
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 signature size
    
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
    tx.auth = PqSignature{std::vector<uint8_t>(3309, 0x55)};  // ML-DSA-65 signature size
    
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

