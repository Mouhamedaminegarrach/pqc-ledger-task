#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <stdexcept>

namespace pqc_ledger {

enum class ErrorCode : uint8_t {
    // Codec errors
    InvalidVersion,
    TrailingBytes,
    InvalidLengthPrefix,
    MismatchedLength,
    InvalidAuthTag,
    
    // Crypto errors
    InvalidPublicKey,
    InvalidSignature,
    SignatureVerificationFailed,
    KeyGenerationFailed,
    HashError,
    
    // Transaction errors
    InvalidTransaction,
    InvalidChainId,
    InvalidAddress,
    InvalidAmount,
    InvalidFee,
    
    // I/O errors
    FileReadError,
    FileWriteError,
    InvalidHexEncoding,
    InvalidBase64Encoding,
    
    // Unknown
    UnknownError
};

struct Error {
    ErrorCode code;
    std::string message;
    
    Error(ErrorCode c, const std::string& msg) : code(c), message(msg) {}
    Error(ErrorCode c) : code(c), message("") {}
};

// Result type for error handling
template<typename T>
class Result {
public:
    static Result<T> Ok(T value) {
        Result<T> result;
        result.value_ = std::move(value);
        result.is_ok_ = true;
        return result;
    }
    
    static Result<T> Err(Error error) {
        Result<T> result;
        result.error_ = std::move(error);
        result.is_ok_ = false;
        return result;
    }
    
    bool is_ok() const { return is_ok_; }
    bool is_err() const { return !is_ok_; }
    
    const T& value() const {
        if (!is_ok_) {
            throw std::runtime_error("Attempted to access value of error Result");
        }
        return value_;
    }
    
    T& value() {
        if (!is_ok_) {
            throw std::runtime_error("Attempted to access value of error Result");
        }
        return value_;
    }
    
    const Error& error() const {
        if (is_ok_) {
            throw std::runtime_error("Attempted to access error of ok Result");
        }
        return error_;
    }
    
private:
    bool is_ok_ = false;
    T value_{};
    Error error_{ErrorCode::UnknownError, ""};
};

// Specialization for void
template<>
class Result<void> {
public:
    static Result<void> Ok() {
        Result<void> result;
        result.is_ok_ = true;
        return result;
    }
    
    static Result<void> Err(Error error) {
        Result<void> result;
        result.error_ = std::move(error);
        result.is_ok_ = false;
        return result;
    }
    
    bool is_ok() const { return is_ok_; }
    bool is_err() const { return !is_ok_; }
    
    const Error& error() const {
        if (is_ok_) {
            throw std::runtime_error("Attempted to access error of ok Result");
        }
        return error_;
    }
    
private:
    bool is_ok_ = false;
    Error error_{ErrorCode::UnknownError, ""};
};

} // namespace pqc_ledger

