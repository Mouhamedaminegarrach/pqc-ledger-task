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

## Building

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- OpenSSL development libraries
- liboqs (Post-Quantum Cryptography library)

### Building liboqs

First, you need to build liboqs:

```bash
# Clone liboqs
git clone -b main https://github.com/open-quantum-safe/liboqs.git
cd liboqs

# Build liboqs
mkdir build && cd build
cmake -GNinja ..
ninja
# Or use make: cmake .. && make
```

This will build liboqs in `liboqs/build/`. The library will be at `liboqs/build/lib/liboqs.a` and headers at `liboqs/build/include/`.

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure (point to liboqs if not in system paths)
cmake .. -DLIBOQS_INCLUDE_DIR=/path/to/liboqs/build/include \
         -DLIBOQS_LIBRARY=/path/to/liboqs/build/lib/liboqs.a

# Or if liboqs is installed system-wide:
cmake ..

# Build
cmake --build .

# Run tests
ctest

# Run benchmarks
./bin/pqc-ledger-bench

# Generate benchmark CSV and graph
./bin/pqc-ledger-bench --benchmark_format=csv > benchmark_results.csv
python scripts/generate_benchmark_graph.py benchmark_results.csv
```

## Usage

### CLI Commands

#### Generate Key Pair
```bash
./bin/pqc-ledger-cli gen-key --algo pq --out ./keys
```

#### Create Transaction
```bash
./bin/pqc-ledger-cli make-tx \
    --to <hex32> \
    --amount <u64> \
    --fee <u64> \
    --nonce <u64> \
    --chain <u32> \
    --pubkey <path>
```

#### Sign Transaction
```bash
./bin/pqc-ledger-cli sign-tx \
    --tx <hex> \
    --pq-key <path> \
    [--ed25519-key <path>]  # Optional for hybrid mode
```

#### Verify Transaction
```bash
./bin/pqc-ledger-cli verify-tx \
    --tx <hex> \
    --chain <u32>
```

## Encoding Specification

### Transaction Structure

```
version: u8 (must be 1)
chain_id: u32 (big-endian)
nonce: u64 (big-endian)
from_pubkey: bytes (fixed length for chosen PQ scheme)
to: [u8; 32] (fixed, no length prefix)
amount: u64 (big-endian)
fee: u64 (big-endian)
auth_tag: u8 (0=pq-only, 1=hybrid)
auth_payload:
  - pq_sig: len(u16 BE) || bytes
  - [if hybrid] classical_sig: len(u16 BE) || bytes
```

### Encoding Rules

- All integers are big-endian
- Variable-length byte arrays: `len (u16 BE) || bytes`
- Fixed-length fields (like `to`) have no length prefix
- No trailing bytes allowed
- Strict length validation

### Signing Message

The signature is computed over:
```
msg = SHA256("TXv1" || chain_id_be || canonical_encode(tx_without_sigs))
```

This provides domain separation and prevents replay attacks across chains.

## Chosen PQ Library

**liboqs** - Open Quantum Safe library

**Rationale:**
- Well-maintained and widely used
- Supports multiple PQC algorithms including Dilithium
- Provides consistent API across algorithms
- Actively developed by the Open Quantum Safe project
- Good documentation and examples

## Threat Model Notes

### Replay Attacks
- **Mitigation**: Domain separation via "TXv1" prefix and chain_id in signing message
- Transactions are chain-specific and cannot be replayed across chains

### Non-Canonical Parsing
- **Mitigation**: Strict decoding rules enforce canonical encoding
- No trailing bytes allowed
- All length prefixes must match actual data
- Fixed-size fields are strictly enforced

### DoS via Ordering
- **Mitigation**: Validation pipeline ordering:
  1. Parse and decode (cheap checks first)
  2. Validate structure (version, lengths)
  3. Verify signatures (expensive operation last)
- This prevents DoS by forcing expensive signature verification on malformed data

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

