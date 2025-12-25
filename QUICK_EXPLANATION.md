# PQC Ledger Task - Quick Explanation Guide

## What is This Project?

A **Post-Quantum Cryptography (PQC) ledger system** - a blockchain-like transaction system that uses quantum-resistant cryptographic signatures (Dilithium3) to create, sign, and verify transactions.

**Key Capability**: Create, encode, sign, decode, and verify transactions that are secure against both classical and quantum computers.

---

## How to Build (Quick Steps)

### 1. Prerequisites
- CMake 3.15+
- C++17 compiler (MSVC/GCC/Clang)
- liboqs (post-quantum library) - already in `third_party/liboqs/`

### 2. Build Commands

```bash
# Create build directory
mkdir build && cd build

# Configure (CMake auto-finds liboqs in third_party/)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build everything
cmake --build . --config Release

# Outputs:
# - Library: build/lib/Release/pqc_ledger.lib
# - CLI: build/bin/Release/pqc-ledger-cli.exe
# - Tests: build/tests/Release/test_*.exe
# - Benchmarks: build/bin/Release/pqc-ledger-bench.exe
```

### 3. Build System Features
- **Automatic Dependency Management**: GTest and Google Benchmark downloaded automatically via FetchContent
- **liboqs Integration**: Automatically finds liboqs in `third_party/liboqs/build/`
- **OpenSSL**: Optional (tests skip gracefully if not available)

---

## How It Works (Simple Explanation)

### Transaction Flow

```
1. Generate Keys (Dilithium3)
   ↓
2. Create Transaction (unsigned)
   ↓
3. Sign Transaction (with domain separation)
   ↓
4. Verify Transaction (check signature)
```

### Key Components

1. **Library (`pqc_ledger`)**: Core functionality
   - Encoding/decoding transactions
   - Cryptographic operations (signing, verification)
   - Address derivation

2. **CLI Tool (`pqc-ledger-cli`)**: User interface
   - 4 commands: gen-key, make-tx, sign-tx, verify-tx

3. **Transaction Structure**:
   ```
   version (1) + chain_id + nonce + pubkey + to + amount + fee + signature
   ```

4. **Signing Process**:
   ```
   message = SHA256("TXv1" || chain_id || transaction_data)
   signature = Dilithium3.sign(message, private_key)
   ```

5. **Verification Process**:
   ```
   Recreate message → Verify signature → Return valid/invalid
   ```

---

## Key Security Features

### 1. Domain Separation (Replay Prevention)
- **Problem**: Transaction signed for chain 1 could be replayed on chain 2
- **Solution**: Include chain_id in signing message
- **Result**: Transaction signed for chain 1 fails verification on chain 2

### 2. Strict Decoding
- **Problem**: Malformed transactions could cause crashes
- **Solution**: Validate everything (version, sizes, trailing bytes)
- **Result**: All invalid inputs return errors (no crashes)

### 3. DoS Protection
- **Problem**: Attackers could force expensive signature verification
- **Solution**: Do cheap checks first, expensive verification last
- **Result**: Malformed data rejected before expensive operations

---

## Architecture Overview

```
┌─────────────────────────────────────────┐
│         CLI (pqc-ledger-cli)            │
│  gen-key | make-tx | sign-tx | verify-tx│
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│      Library (pqc_ledger)                │
│  ┌──────────┐  ┌──────────┐  ┌────────┐│
│  │  Codec   │  │  Crypto  │  │   TX   ││
│  │encode/   │  │sign/     │  │signing/││
│  │decode    │  │verify    │  │verify  ││
│  └──────────┘  └──────────┘  └────────┘│
└─────────────────┬───────────────────────┘
                  │
      ┌───────────┼───────────┐
      │           │           │
┌─────▼───┐  ┌───▼────┐  ┌───▼────┐
│ liboqs  │  │picosha2│  │OpenSSL │
│(Dilithium)│ │(SHA256)│  │(optional)│
└─────────┘  └────────┘  └────────┘
```

---

## Technical Highlights

### Encoding Format
- **Big-endian integers** (network byte order)
- **Variable-length fields**: `length (u16 BE) || bytes`
- **Fixed-length fields**: No length prefix
- **Strict**: No trailing bytes allowed

### Signing Message
```
SHA256("TXv1" || chain_id_be || transaction_data_without_signatures)
```
- "TXv1": Domain prefix (protocol identifier)
- chain_id_be: Chain ID in big-endian (4 bytes)
- transaction_data: Canonical encoding without signatures

### Verification
- Uses **parameter chain_id** (not tx.chain_id) for replay prevention
- Validates structure before expensive signature verification
- Returns structured errors (no exceptions)

---

## Dependencies

1. **liboqs**: Post-quantum cryptography (Dilithium3)
   - Location: `third_party/liboqs/`
   - Auto-detected by CMake

2. **picosha2**: SHA256 hashing (header-only)
   - Location: `third_party/picosha2/`

3. **OpenSSL**: Optional (SHA256, Ed25519)
   - Tests skip gracefully if not available

4. **GTest**: Testing framework
   - Auto-downloaded via FetchContent

5. **Google Benchmark**: Performance benchmarks
   - Auto-downloaded via FetchContent

---

## Project Structure

```
pqc-ledger-task/
├── include/pqc_ledger/    # Public API
│   ├── codec/            # Encoding/decoding headers
│   ├── crypto/            # Crypto headers
│   └── tx/                # Transaction headers
├── src/                   # Implementation
│   ├── cli/               # CLI tool
│   ├── codec/             # Binary encoding/decoding
│   ├── crypto/             # Crypto operations
│   └── tx/                 # Transaction operations
├── tests/                 # Unit tests
├── benches/               # Benchmarks
└── scripts/               # Utility scripts
```

---

## Usage Example

```bash
# 1. Generate keys
./pqc-ledger-cli gen-key --algo pq --out ./keys

# 2. Create transaction
./pqc-ledger-cli make-tx \
    --to "0000...0000" \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey ./keys/pubkey.bin

# 3. Sign transaction
./pqc-ledger-cli sign-tx \
    --tx <hex_from_step2> \
    --pq-key ./keys/privkey.bin

# 4. Verify transaction
./pqc-ledger-cli verify-tx \
    --tx <signed_hex> \
    --chain 1
```

---

## Key Points for Discussion

### Why This Approach?
1. **Post-Quantum Security**: Uses Dilithium3 (quantum-resistant)
2. **Canonical Encoding**: Ensures unique binary representation
3. **Domain Separation**: Prevents replay attacks
4. **Strict Validation**: Prevents DoS and parsing attacks
5. **Error Handling**: No crashes, structured errors

### What Makes It Production-Ready?
- ✅ Comprehensive testing (100% pass rate)
- ✅ Robust error handling (no panics)
- ✅ Performance benchmarks (1.52 ms for 100 transactions)
- ✅ Complete documentation
- ✅ Security features (replay prevention, DoS protection)

### Technical Decisions
- **liboqs**: Industry-standard PQC library
- **Big-endian**: Network byte order standard
- **Domain separation**: Industry best practice
- **Strict decoding**: Security requirement

---

## Performance

- **100 Transactions**: 1.52 ms average
- **Single Transaction**: 13.8 μs
- **Throughput**: 64,000 transactions/second
- **Encoding**: 0.682 μs
- **Decoding**: 0.584 μs

---

## Testing Summary

- **8 Phases**: All passed (100%)
- **32 Test Suites**: All passed
- **50+ Test Cases**: All passed
- **Coverage**: Unit tests, integration tests, error handling, benchmarks

See `TEST_SUMMARY_REPORT.md` for complete details.

---

## Quick Answers to Common Questions

**Q: How do you build it?**
A: `mkdir build && cd build && cmake .. && cmake --build .`

**Q: How does signing work?**
A: Create domain-separated message `SHA256("TXv1" || chain_id || tx_data)`, then sign with Dilithium3.

**Q: How do you prevent replay attacks?**
A: Domain separation - signing message includes chain_id, verification uses parameter chain_id (not tx.chain_id).

**Q: What if someone sends invalid data?**
A: Strict validation catches all errors, returns structured errors (no crashes).

**Q: What's the performance?**
A: 1.52 ms for 100 transactions, 13.8 μs per transaction.

**Q: What dependencies are needed?**
A: liboqs (included), CMake, C++17 compiler. GTest and Benchmark auto-downloaded.

