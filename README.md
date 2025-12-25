# PQC Ledger Task

A small "ledger-core" library + CLI that can create, encode, sign, decode, and verify blockchain-like transactions using a post-quantum signature scheme (Dilithium preferred).

## Features

- **Post-Quantum Signatures**: Uses Dilithium via liboqs
- **Canonical Encoding**: Strict binary encoding/decoding with validation
- **Domain Separation**: Proper signature domain separation to prevent replay attacks
- **Error Handling**: Structured error handling (no panics in decode/verify paths)
- **CLI Tool**: Command-line interface for key generation, transaction creation, signing, and verification
- **Testing**: Comprehensive test suite including round-trip, mutation, and replay tests
- **Benchmarking**: Performance benchmarks for signature verification with graph generation
- **CLI**: Full command-line interface with hex and base64 output support

## How to Build/Run

### Prerequisites

- CMake 3.15+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- liboqs (see below)
- OpenSSL (optional - for Ed25519 hybrid mode; uses picosha2 for SHA256 otherwise)

### Build Steps

1. **Build liboqs**:
```bash
git clone -b main https://github.com/open-quantum-safe/liboqs.git
cd liboqs && mkdir build && cd build
cmake -GNinja .. && ninja
# Library: liboqs/build/lib/liboqs.a
# Headers: liboqs/build/include/
```

2. **Build this project**:
```bash
mkdir build && cd build
cmake .. -DLIBOQS_INCLUDE_DIR=/path/to/liboqs/build/include \
         -DLIBOQS_LIBRARY=/path/to/liboqs/build/lib/liboqs.a
cmake --build .
```

3. **Run**:
```bash
# Tests
ctest

# CLI
./bin/pqc-ledger-cli gen-key --algo pq --out ./keys
./bin/pqc-ledger-cli make-tx --to <hex32> --amount 1000 --fee 10 --nonce 1 --chain 1 --pubkey ./keys/pubkey.bin
./bin/pqc-ledger-cli sign-tx --tx <hex> --pq-key ./keys/privkey.bin
./bin/pqc-ledger-cli verify-tx --tx <hex> --chain 1
```

## Encoding Specification

**Binary Format** (big-endian):
```
version: u8 (must be 1)
chain_id: u32
nonce: u64
from_pubkey: len(u16) || bytes (1952 bytes for ML-DSA-65/Dilithium3)
to: [u8; 32] (fixed, no length prefix)
amount: u64
fee: u64
auth_tag: u8 (0=pq-only, 1=hybrid)
auth_payload:
  - pq_sig: len(u16) || bytes (3309 bytes for ML-DSA-65)
  - [if hybrid] classical_sig: len(u16) || bytes (64 bytes for Ed25519)
```

**Rules**: All integers big-endian; variable fields prefixed with `len(u16 BE)`; fixed fields have no prefix; **no trailing bytes**; strict validation.

**Signing**: `SHA256("TXv1" || chain_id_be || tx_data_without_sigs)` - domain separation prevents replay.

## Chosen PQ Library

**liboqs** (Open Quantum Safe) with **ML-DSA-65** (NIST standard, equivalent to Dilithium3)

**Rationale:**
- **NIST Standardized**: ML-DSA-65 is the NIST PQC standard (FIPS 204)
- **Industry Standard**: liboqs is the reference implementation, widely used and maintained
- **Consistent API**: Unified interface across PQC algorithms
- **Production Ready**: Actively developed by Open Quantum Safe project with comprehensive testing
- **Performance**: ~1.5ms per signature verification (benchmarked)

## Threat Model Notes

**Replay Attacks**: Domain separation via `"TXv1"` prefix + `chain_id` in signing message. Verification uses **parameter** `chain_id` (not `tx.chain_id`), preventing cross-chain replay.

**Non-Canonical Parsing**: Strict decoding enforces canonical encoding - no trailing bytes, length prefixes validated, fixed sizes enforced, version must be exactly 1.

**DoS via Ordering**: Validation pipeline: (1) parse/decode (cheap), (2) validate structure (version, lengths), (3) verify signatures (expensive). Prevents DoS by rejecting malformed data before expensive crypto operations.

## Project Structure

```
pqc-ledger-task/
├── CMakeLists.txt
├── README.md
├── include/pqc_ledger/     # Public headers
├── src/                     # Implementation
├── tests/                   # Test suite
└── benches/                 # Benchmarks
```

## License

[Add license information]

