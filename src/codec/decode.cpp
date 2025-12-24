#include "pqc_ledger/codec/decode.hpp"
#include "pqc_ledger/types.hpp"
#include "pqc_ledger/crypto/pq.hpp"
#include <cstring>
#include <stdexcept>
#include <cctype>

namespace pqc_ledger::codec {

namespace {
    class Reader {
    public:
        Reader(const std::vector<uint8_t>& data) : data_(data), pos_(0) {}
        
        bool has_bytes(size_t n) const {
            return pos_ + n <= data_.size();
        }
        
        uint8_t read_u8() {
            if (!has_bytes(1)) {
                throw std::runtime_error("Unexpected end of data");
            }
            return data_[pos_++];
        }
        
        uint16_t read_u16_be() {
            if (!has_bytes(2)) {
                throw std::runtime_error("Unexpected end of data");
            }
            uint16_t value = (static_cast<uint16_t>(data_[pos_]) << 8) |
                             static_cast<uint16_t>(data_[pos_ + 1]);
            pos_ += 2;
            return value;
        }
        
        uint32_t read_u32_be() {
            if (!has_bytes(4)) {
                throw std::runtime_error("Unexpected end of data");
            }
            uint32_t value = (static_cast<uint32_t>(data_[pos_]) << 24) |
                             (static_cast<uint32_t>(data_[pos_ + 1]) << 16) |
                             (static_cast<uint32_t>(data_[pos_ + 2]) << 8) |
                             static_cast<uint32_t>(data_[pos_ + 3]);
            pos_ += 4;
            return value;
        }
        
        uint64_t read_u64_be() {
            if (!has_bytes(8)) {
                throw std::runtime_error("Unexpected end of data");
            }
            uint64_t value = (static_cast<uint64_t>(data_[pos_]) << 56) |
                             (static_cast<uint64_t>(data_[pos_ + 1]) << 48) |
                             (static_cast<uint64_t>(data_[pos_ + 2]) << 40) |
                             (static_cast<uint64_t>(data_[pos_ + 3]) << 32) |
                             (static_cast<uint64_t>(data_[pos_ + 4]) << 24) |
                             (static_cast<uint64_t>(data_[pos_ + 5]) << 16) |
                             (static_cast<uint64_t>(data_[pos_ + 6]) << 8) |
                             static_cast<uint64_t>(data_[pos_ + 7]);
            pos_ += 8;
            return value;
        }
        
        std::vector<uint8_t> read_bytes(size_t n) {
            if (!has_bytes(n)) {
                throw std::runtime_error("Unexpected end of data");
            }
            std::vector<uint8_t> result(data_.begin() + pos_, data_.begin() + pos_ + n);
            pos_ += n;
            return result;
        }
        
        std::vector<uint8_t> read_bytes_with_len() {
            uint16_t len = read_u16_be();
            // Validate length doesn't exceed remaining buffer
            if (len > remaining()) {
                throw std::runtime_error("LENGTH_PREFIX_MISMATCH:" + 
                                        std::to_string(len) + " > " + 
                                        std::to_string(remaining()));
            }
            return read_bytes(len);
        }
        
        size_t remaining() const {
            return data_.size() - pos_;
        }
        
        bool at_end() const {
            return pos_ >= data_.size();
        }
        
    private:
        const std::vector<uint8_t>& data_;
        size_t pos_;
    };
    
    // Helper function to convert hex character to value
    uint8_t hex_char_to_value(char c) {
        if (c >= '0' && c <= '9') {
            return static_cast<uint8_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            return static_cast<uint8_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            return static_cast<uint8_t>(c - 'A' + 10);
        }
        throw std::runtime_error("Invalid hex character: " + std::string(1, c));
    }
    
    // Decode hex string to bytes
    std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
        // Remove whitespace
        std::string clean_hex;
        for (char c : hex) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                clean_hex += c;
            }
        }
        
        // Hex string must have even length
        if (clean_hex.length() % 2 != 0) {
            throw std::runtime_error("Hex string must have even length");
        }
        
        std::vector<uint8_t> result;
        result.reserve(clean_hex.length() / 2);
        
        for (size_t i = 0; i < clean_hex.length(); i += 2) {
            uint8_t high = hex_char_to_value(clean_hex[i]);
            uint8_t low = hex_char_to_value(clean_hex[i + 1]);
            result.push_back((high << 4) | low);
        }
        
        return result;
    }
    
    // Base64 character to value mapping
    uint8_t base64_char_to_value(char c) {
        if (c >= 'A' && c <= 'Z') {
            return static_cast<uint8_t>(c - 'A');
        } else if (c >= 'a' && c <= 'z') {
            return static_cast<uint8_t>(c - 'a' + 26);
        } else if (c >= '0' && c <= '9') {
            return static_cast<uint8_t>(c - '0' + 52);
        } else if (c == '+') {
            return 62;
        } else if (c == '/') {
            return 63;
        } else if (c == '=') {
            return 0; // Padding, but we'll handle it separately
        }
        throw std::runtime_error("Invalid base64 character: " + std::string(1, c));
    }
    
    // Decode base64 string to bytes
    std::vector<uint8_t> base64_to_bytes(const std::string& base64) {
        // Remove whitespace
        std::string clean_base64;
        for (char c : base64) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                clean_base64 += c;
            }
        }
        
        if (clean_base64.empty()) {
            return std::vector<uint8_t>();
        }
        
        // Calculate output size (base64 encodes 3 bytes as 4 characters)
        size_t padding_count = 0;
        if (clean_base64.length() > 0 && clean_base64[clean_base64.length() - 1] == '=') {
            padding_count++;
            if (clean_base64.length() > 1 && clean_base64[clean_base64.length() - 2] == '=') {
                padding_count++;
            }
        }
        
        size_t output_size = (clean_base64.length() * 3) / 4 - padding_count;
        std::vector<uint8_t> result;
        result.reserve(output_size);
        
        // Process 4 characters at a time
        for (size_t i = 0; i < clean_base64.length(); i += 4) {
            if (i + 3 >= clean_base64.length()) {
                throw std::runtime_error("Invalid base64 string: incomplete group");
            }
            
            uint8_t b0 = base64_char_to_value(clean_base64[i]);
            uint8_t b1 = base64_char_to_value(clean_base64[i + 1]);
            uint8_t b2 = (i + 2 < clean_base64.length() && clean_base64[i + 2] != '=') 
                         ? base64_char_to_value(clean_base64[i + 2]) : 0;
            uint8_t b3 = (i + 3 < clean_base64.length() && clean_base64[i + 3] != '=') 
                         ? base64_char_to_value(clean_base64[i + 3]) : 0;
            
            // Combine 4 base64 characters (6 bits each) into 3 bytes (8 bits each)
            result.push_back((b0 << 2) | (b1 >> 4));
            
            if (clean_base64[i + 2] != '=') {
                result.push_back((b1 << 4) | (b2 >> 2));
            }
            
            if (clean_base64[i + 3] != '=') {
                result.push_back((b2 << 6) | b3);
            }
        }
        
        return result;
    }
}

Result<Transaction> decode(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return Result<Transaction>::Err(Error(ErrorCode::InvalidTransaction, "Empty transaction data"));
    }
    
    try {
        Reader reader(data);
        
        Transaction tx;
        
        // Version (must be 1)
        tx.version = reader.read_u8();
        if (tx.version != 1) {
            return Result<Transaction>::Err(Error(ErrorCode::InvalidVersion, 
                "Version must be 1, got " + std::to_string(tx.version)));
        }
        
        // Chain ID
        tx.chain_id = reader.read_u32_be();
        
        // Nonce
        tx.nonce = reader.read_u64_be();
        
        // From pubkey (variable length)
        tx.from_pubkey = reader.read_bytes_with_len();
        
        // Validate pubkey size matches expected algorithm size (default: ML-DSA-65, equivalent to Dilithium3)
        // ML-DSA-65 pubkey size is 1952 bytes (same as Dilithium3)
        constexpr size_t ML_DSA_65_PUBKEY_SIZE = 1952;
        auto expected_pubkey_size = pqc_ledger::crypto::get_pubkey_size("ML-DSA-65");
        size_t expected_size = ML_DSA_65_PUBKEY_SIZE;  // Default fallback
        if (expected_pubkey_size.is_ok()) {
            expected_size = expected_pubkey_size.value();
        }
        
        if (tx.from_pubkey.size() != expected_size) {
            return Result<Transaction>::Err(Error(ErrorCode::InvalidPublicKey,
                "Public key size mismatch: expected " + 
                std::to_string(expected_size) + 
                ", got " + std::to_string(tx.from_pubkey.size())));
        }
        
        // To address (fixed 32 bytes)
        tx.to = {};
        auto to_bytes = reader.read_bytes(32);
        std::copy(to_bytes.begin(), to_bytes.end(), tx.to.begin());
        
        // Amount
        tx.amount = reader.read_u64_be();
        
        // Fee
        tx.fee = reader.read_u64_be();
        
        // Auth tag
        uint8_t auth_tag = reader.read_u8();
        if (auth_tag == 0) {
            tx.auth_mode = AuthMode::PqOnly;
            auto sig_bytes = reader.read_bytes_with_len();
            
            // Allow empty signature for unsigned transactions (size 0)
            // Otherwise, validate PQ signature size matches expected algorithm size
            if (sig_bytes.size() > 0) {
                // ML-DSA-65 signature size is 3309 bytes
                constexpr size_t ML_DSA_65_SIG_SIZE = 3309;
                auto expected_sig_size = pqc_ledger::crypto::get_signature_size("ML-DSA-65");
                size_t expected_size = ML_DSA_65_SIG_SIZE;  // Default fallback
                if (expected_sig_size.is_ok()) {
                    expected_size = expected_sig_size.value();
                }
                
                if (sig_bytes.size() != expected_size) {
                    return Result<Transaction>::Err(Error(ErrorCode::InvalidSignature,
                        "PQ signature size mismatch: expected " + 
                        std::to_string(expected_size) + 
                        ", got " + std::to_string(sig_bytes.size())));
                }
            }
            
            tx.auth = PqSignature{std::move(sig_bytes)};
        } else if (auth_tag == 1) {
            tx.auth_mode = AuthMode::Hybrid;
            auto classical_sig = reader.read_bytes_with_len();
            auto pq_sig = reader.read_bytes_with_len();
            
            // Validate Ed25519 signature size (must be 64 bytes)
            if (classical_sig.size() != 64) {
                return Result<Transaction>::Err(Error(ErrorCode::InvalidSignature,
                    "Ed25519 signature size mismatch: expected 64, got " + 
                    std::to_string(classical_sig.size())));
            }
            
            // Validate PQ signature size matches expected algorithm size (default: ML-DSA-65, equivalent to Dilithium3)
            // ML-DSA-65 signature size is 3309 bytes
            constexpr size_t ML_DSA_65_SIG_SIZE = 3309;
            auto expected_sig_size = pqc_ledger::crypto::get_signature_size("ML-DSA-65");
            size_t expected_size = ML_DSA_65_SIG_SIZE;  // Default fallback
            if (expected_sig_size.is_ok()) {
                expected_size = expected_sig_size.value();
            }
            
            if (pq_sig.size() != expected_size) {
                return Result<Transaction>::Err(Error(ErrorCode::InvalidSignature,
                    "PQ signature size mismatch: expected " + 
                    std::to_string(expected_size) + 
                    ", got " + std::to_string(pq_sig.size())));
            }
            
            tx.auth = HybridSignature{std::move(classical_sig), std::move(pq_sig)};
        } else {
            return Result<Transaction>::Err(Error(ErrorCode::InvalidAuthTag,
                "Invalid auth tag: " + std::to_string(auth_tag)));
        }
        
        // Strict: no trailing bytes allowed
        if (!reader.at_end()) {
            return Result<Transaction>::Err(Error(ErrorCode::TrailingBytes,
                "Trailing bytes found: " + std::to_string(reader.remaining()) + " bytes"));
        }
        
        return Result<Transaction>::Ok(std::move(tx));
        
    } catch (const std::exception& e) {
        std::string error_msg = e.what();
        // Check for specific error types
        if (error_msg.find("LENGTH_PREFIX_MISMATCH:") == 0) {
            // Extract the length values from error message
            return Result<Transaction>::Err(Error(ErrorCode::MismatchedLength, 
                "Length prefix exceeds remaining buffer: " + error_msg.substr(23)));
        }
        if (error_msg.find("Unexpected end of data") != std::string::npos) {
            return Result<Transaction>::Err(Error(ErrorCode::InvalidLengthPrefix,
                "Unexpected end of data while reading"));
        }
        return Result<Transaction>::Err(Error(ErrorCode::InvalidTransaction, error_msg));
    }
}

Result<Transaction> decode_from_hex(const std::string& hex) {
    try {
        auto bytes = hex_to_bytes(hex);
        return decode(bytes);
    } catch (const std::exception& e) {
        return Result<Transaction>::Err(Error(ErrorCode::InvalidHexEncoding, e.what()));
    }
}

Result<Transaction> decode_from_base64(const std::string& base64) {
    try {
        auto bytes = base64_to_bytes(base64);
        return decode(bytes);
    } catch (const std::exception& e) {
        return Result<Transaction>::Err(Error(ErrorCode::InvalidBase64Encoding, e.what()));
    }
}

} // namespace pqc_ledger::codec

