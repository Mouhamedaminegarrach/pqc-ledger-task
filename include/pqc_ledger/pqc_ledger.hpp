#pragma once

// Main library header - re-exports core modules

#include "pqc_ledger/error.hpp"
#include "pqc_ledger/types.hpp"

// Codec
#include "pqc_ledger/codec/encode.hpp"
#include "pqc_ledger/codec/decode.hpp"

// Crypto
#include "pqc_ledger/crypto/hash.hpp"
#include "pqc_ledger/crypto/pq.hpp"
#include "pqc_ledger/crypto/address.hpp"
#include "pqc_ledger/crypto/classical.hpp"

// Transaction
#include "pqc_ledger/tx/signing.hpp"
#include "pqc_ledger/tx/validation.hpp"

// Main namespace
namespace pqc_ledger {
    // All types and functions are available through the pqc_ledger namespace
}

