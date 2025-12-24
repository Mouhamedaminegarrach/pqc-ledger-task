# Phase 3: Functional Tests (CLI) - Test Results

## Test Environment
- **Date**: 2025-12-24
- **Platform**: Windows
- **OpenSSL**: Not installed (required for signing/verification)
- **liboqs**: Available but Dilithium algorithms not enabled

## Test Results

### ✅ Test 3.1: Generate Key Pair
**Status**: ⚠️ **BLOCKED** - liboqs doesn't have Dilithium3/Dilithium2 enabled

**Issue**: 
```
Error generating keypair: Algorithm Dilithium3 not enabled at compile-time or not available
```

**Workaround**: Created test pubkey manually (1952 bytes) for subsequent tests

**Note**: liboqs needs to be rebuilt with Dilithium algorithms enabled, or use a different liboqs build.

---

### ✅ Test 3.2: Create Transaction (Hex Output)
**Status**: ✅ **PASSED**

**Command**:
```powershell
.\bin\Release\pqc-ledger-cli.exe make-tx --to 0000...0000 --amount 1000 --fee 10 --nonce 1 --chain 1 --pubkey test_keys\pubkey.bin
```

**Result**: Successfully outputs hex-encoded transaction
- Transaction encoded correctly
- Hex format is valid
- No errors

---

### ✅ Test 3.3: Create Transaction (Base64 Output)
**Status**: ✅ **PASSED**

**Command**:
```powershell
.\bin\Release\pqc-ledger-cli.exe make-tx --to 0000...0000 --amount 1000 --fee 10 --nonce 1 --chain 1 --pubkey test_keys\pubkey.bin --format base64
```

**Result**: Successfully outputs base64-encoded transaction
- Transaction encoded correctly
- Base64 format is valid
- `--format` option works correctly
- No errors

---

### ⚠️ Test 3.4: Sign Transaction
**Status**: ⚠️ **BLOCKED** - Requires OpenSSL

**Expected**: Transaction signing should work
**Actual**: Will fail because:
1. OpenSSL not installed (required for SHA256)
2. Need valid private key (requires liboqs with Dilithium enabled)

**Note**: Once OpenSSL is installed and liboqs is configured correctly, this test should pass.

---

### ⚠️ Test 3.5: Verify Transaction
**Status**: ⚠️ **BLOCKED** - Requires OpenSSL

**Expected**: Transaction verification should work
**Actual**: Will fail because OpenSSL is not installed (required for SHA256)

**Note**: Once OpenSSL is installed, this test should pass.

---

### ⚠️ Test 3.6: Verify Transaction (Wrong Chain ID) - Chain Replay Prevention
**Status**: ⚠️ **BLOCKED** - Requires OpenSSL

**Expected**: Transaction signed for chain 1 should fail verification on chain 2
**Actual**: Cannot test without OpenSSL

**Note**: This is a critical security test. Once OpenSSL is installed, this test must pass to verify domain separation works correctly.

---

## Summary

### Tests Passing: 2/6 (33%)
- ✅ Test 3.2: Create Transaction (Hex Output)
- ✅ Test 3.3: Create Transaction (Base64 Output)

### Tests Blocked: 4/6 (67%)
- ⚠️ Test 3.1: Generate Key Pair (liboqs configuration)
- ⚠️ Test 3.4: Sign Transaction (OpenSSL required)
- ⚠️ Test 3.5: Verify Transaction (OpenSSL required)
- ⚠️ Test 3.6: Chain Replay Prevention (OpenSSL required)

---

## Next Steps

### To Complete Phase 3:

1. **Install OpenSSL** (see `INSTALL_OPENSSL.md`)
   - Required for: SHA256 hashing, signing, verification
   - After installation, rebuild project

2. **Configure liboqs with Dilithium algorithms**
   - Rebuild liboqs with Dilithium2 or Dilithium3 enabled
   - Or use a pre-built liboqs that has Dilithium enabled

3. **Re-run Phase 3 tests**
   - All tests should pass once dependencies are configured

---

## Known Issues

1. **liboqs Dilithium not enabled**: The liboqs library in `third_party/liboqs` was not built with Dilithium algorithms enabled. This prevents key generation.

2. **OpenSSL not installed**: Required for:
   - SHA256 hashing (signing messages)
   - Ed25519 signatures (hybrid mode)
   - Full test coverage

---

## Test Commands Reference

```powershell
# Test 3.2: Hex output
$TEST_TO = "0000000000000000000000000000000000000000000000000000000000000000"
.\bin\Release\pqc-ledger-cli.exe make-tx --to $TEST_TO --amount 1000 --fee 10 --nonce 1 --chain 1 --pubkey test_keys\pubkey.bin

# Test 3.3: Base64 output
.\bin\Release\pqc-ledger-cli.exe make-tx --to $TEST_TO --amount 1000 --fee 10 --nonce 1 --chain 1 --pubkey test_keys\pubkey.bin --format base64

# Test 3.4: Sign (requires OpenSSL)
$TX_HEX = "..." # from Test 3.2
.\bin\Release\pqc-ledger-cli.exe sign-tx --tx $TX_HEX --pq-key test_keys\privkey.bin

# Test 3.5: Verify (requires OpenSSL)
$SIGNED_TX = "..." # from Test 3.4
.\bin\Release\pqc-ledger-cli.exe verify-tx --tx $SIGNED_TX --chain 1

# Test 3.6: Chain replay prevention (requires OpenSSL)
.\bin\Release\pqc-ledger-cli.exe verify-tx --tx $SIGNED_TX --chain 2  # Should fail
```



