#include "pqc_ledger/pqc_ledger.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

// Simple CLI argument parser (can be replaced with CLI11 or similar)
class ArgParser {
public:
    ArgParser(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            args_.push_back(argv[i]);
        }
    }
    
    bool has(const std::string& flag) const {
        return std::find(args_.begin(), args_.end(), flag) != args_.end();
    }
    
    std::string get(const std::string& flag, const std::string& default_val = "") const {
        auto it = std::find(args_.begin(), args_.end(), flag);
        if (it != args_.end() && (it + 1) != args_.end()) {
            return *(it + 1);
        }
        return default_val;
    }
    
private:
    std::vector<std::string> args_;
};

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
    }
    return bytes;
}

void print_usage() {
    std::cout << "Usage: pqc-ledger-cli <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  gen-key --algo <pq> --out <dir>\n";
    std::cout << "  make-tx --to <hex32> --amount <u64> --fee <u64> --nonce <u64> --chain <u32> --pubkey <path> [--format <hex|base64>]\n";
    std::cout << "  sign-tx --tx <hex> --pq-key <path> [--ed25519-key <path>]\n";
    std::cout << "  verify-tx --tx <hex> --chain <u32>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --format: Output format for make-tx (hex or base64, default: hex)\n";
}

int cmd_gen_key(const ArgParser& parser) {
    std::string algo = parser.get("--algo", "pq");
    std::string out_dir = parser.get("--out");
    
    if (out_dir.empty()) {
        std::cerr << "Error: --out is required\n";
        return 1;
    }
    
    if (algo != "pq") {
        std::cerr << "Error: Only 'pq' algorithm is supported\n";
        return 1;
    }
    
    // Generate PQ keypair
    auto keypair_result = pqc_ledger::crypto::generate_keypair("Dilithium3");
    if (keypair_result.is_err()) {
        std::cerr << "Error generating keypair: " << keypair_result.error().message << "\n";
        return 1;
    }
    
    const auto& [pubkey, privkey] = keypair_result.value();
    
    // Save keys
    std::string pubkey_path = out_dir + "/pubkey.bin";
    std::string privkey_path = out_dir + "/privkey.bin";
    
    auto save_pub_result = pqc_ledger::crypto::save_public_key(pubkey, pubkey_path);
    if (save_pub_result.is_err()) {
        std::cerr << "Error saving public key: " << save_pub_result.error().message << "\n";
        return 1;
    }
    
    auto save_priv_result = pqc_ledger::crypto::save_private_key(privkey, privkey_path);
    if (save_priv_result.is_err()) {
        std::cerr << "Error saving private key: " << save_priv_result.error().message << "\n";
        return 1;
    }
    
    std::cout << "Keypair generated successfully:\n";
    std::cout << "  Public key: " << pubkey_path << "\n";
    std::cout << "  Private key: " << privkey_path << "\n";
    
    return 0;
}

int cmd_make_tx(const ArgParser& parser) {
    std::string to_hex = parser.get("--to");
    std::string amount_str = parser.get("--amount");
    std::string fee_str = parser.get("--fee");
    std::string nonce_str = parser.get("--nonce");
    std::string chain_str = parser.get("--chain");
    std::string pubkey_path = parser.get("--pubkey");
    std::string format = parser.get("--format", "hex");  // Default to hex
    
    if (to_hex.empty() || amount_str.empty() || fee_str.empty() || 
        nonce_str.empty() || chain_str.empty() || pubkey_path.empty()) {
        std::cerr << "Error: All arguments are required\n";
        return 1;
    }
    
    // Validate format
    if (format != "hex" && format != "base64") {
        std::cerr << "Error: --format must be 'hex' or 'base64'\n";
        return 1;
    }
    
    // Parse arguments
    uint64_t amount = std::stoull(amount_str);
    uint64_t fee = std::stoull(fee_str);
    uint64_t nonce = std::stoull(nonce_str);
    uint32_t chain_id = std::stoul(chain_str);
    
    // Parse 'to' address
    auto to_bytes = hex_to_bytes(to_hex);
    if (to_bytes.size() != 32) {
        std::cerr << "Error: 'to' must be 64 hex characters (32 bytes)\n";
        return 1;
    }
    pqc_ledger::Address to_addr;
    std::copy(to_bytes.begin(), to_bytes.end(), to_addr.begin());
    
    // Load public key
    auto pubkey_result = pqc_ledger::crypto::load_public_key(pubkey_path);
    if (pubkey_result.is_err()) {
        std::cerr << "Error loading public key: " << pubkey_result.error().message << "\n";
        return 1;
    }
    
    // Create unsigned transaction
    pqc_ledger::Transaction tx;
    tx.version = 1;
    tx.chain_id = chain_id;
    tx.nonce = nonce;
    tx.from_pubkey = pubkey_result.value();
    tx.to = to_addr;
    tx.amount = amount;
    tx.fee = fee;
    tx.auth_mode = pqc_ledger::AuthMode::PqOnly;
    tx.auth = pqc_ledger::PqSignature{{}};  // Empty signature for unsigned tx
    
    // Encode transaction
    auto encoded_result = pqc_ledger::codec::encode(tx);
    if (encoded_result.is_err()) {
        std::cerr << "Error encoding transaction: " << encoded_result.error().message << "\n";
        return 1;
    }
    
    // Output in requested format
    if (format == "base64") {
        std::cout << pqc_ledger::codec::encode_to_base64(encoded_result.value()) << "\n";
    } else {
        std::cout << bytes_to_hex(encoded_result.value()) << "\n";
    }
    
    return 0;
}

int cmd_sign_tx(const ArgParser& parser) {
    std::string tx_hex = parser.get("--tx");
    std::string pq_key_path = parser.get("--pq-key");
    std::string ed25519_key_path = parser.get("--ed25519-key", "");
    
    if (tx_hex.empty() || pq_key_path.empty()) {
        std::cerr << "Error: --tx and --pq-key are required\n";
        return 1;
    }
    
    // Decode transaction
    auto tx_bytes = hex_to_bytes(tx_hex);
    auto decode_result = pqc_ledger::codec::decode(tx_bytes);
    if (decode_result.is_err()) {
        std::cerr << "Error decoding transaction: " << decode_result.error().message << "\n";
        return 1;
    }
    
    auto tx = decode_result.value();
    
    // Load private keys
    auto pq_privkey_result = pqc_ledger::crypto::load_private_key(pq_key_path);
    if (pq_privkey_result.is_err()) {
        std::cerr << "Error loading PQ private key: " << pq_privkey_result.error().message << "\n";
        return 1;
    }
    
    // Sign transaction
    if (!ed25519_key_path.empty()) {
        // Hybrid mode
        auto ed25519_privkey_result = pqc_ledger::crypto::load_ed25519_private_key(ed25519_key_path);
        if (ed25519_privkey_result.is_err()) {
            std::cerr << "Error loading Ed25519 private key: " << ed25519_privkey_result.error().message << "\n";
            return 1;
        }
        
        auto sign_result = pqc_ledger::tx::sign_transaction_hybrid(
            tx, pq_privkey_result.value(), ed25519_privkey_result.value());
        if (sign_result.is_err()) {
            std::cerr << "Error signing transaction: " << sign_result.error().message << "\n";
            return 1;
        }
    } else {
        // PQ-only mode
        auto sign_result = pqc_ledger::tx::sign_transaction(tx, pq_privkey_result.value());
        if (sign_result.is_err()) {
            std::cerr << "Error signing transaction: " << sign_result.error().message << "\n";
            return 1;
        }
    }
    
    // Encode signed transaction
    auto encoded_result = pqc_ledger::codec::encode(tx);
    if (encoded_result.is_err()) {
        std::cerr << "Error encoding signed transaction: " << encoded_result.error().message << "\n";
        return 1;
    }
    
    // Output as hex
    std::cout << bytes_to_hex(encoded_result.value()) << "\n";
    
    return 0;
}

int cmd_verify_tx(const ArgParser& parser) {
    std::string tx_hex = parser.get("--tx");
    std::string chain_str = parser.get("--chain");
    
    if (tx_hex.empty() || chain_str.empty()) {
        std::cerr << "Error: --tx and --chain are required\n";
        return 1;
    }
    
    uint32_t chain_id = std::stoul(chain_str);
    
    // Decode transaction
    auto tx_bytes = hex_to_bytes(tx_hex);
    auto decode_result = pqc_ledger::codec::decode(tx_bytes);
    if (decode_result.is_err()) {
        std::cerr << "Error decoding transaction: " << decode_result.error().message << "\n";
        std::cout << "valid: false\n";
        return 1;
    }
    
    auto tx = decode_result.value();
    
    // Verify transaction
    auto verify_result = pqc_ledger::tx::verify_transaction(tx, chain_id);
    if (verify_result.is_err()) {
        std::cerr << "Error verifying transaction: " << verify_result.error().message << "\n";
        std::cout << "valid: false\n";
        return 1;
    }
    
    bool valid = verify_result.value();
    
    // Derive address
    auto addr_result = pqc_ledger::crypto::derive_address(tx.from_pubkey);
    if (addr_result.is_err()) {
        std::cerr << "Error deriving address: " << addr_result.error().message << "\n";
    } else {
        std::cout << "from_address: " << pqc_ledger::crypto::address_to_hex(addr_result.value()) << "\n";
    }
    
    if (valid) {
        std::cout << "valid: true\n";
        return 0;
    } else {
        std::cout << "valid: false\n";
        std::cout << "error: signature verification failed\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    ArgParser parser(argc, argv);
    std::string command = argv[1];
    
    if (command == "gen-key") {
        return cmd_gen_key(parser);
    } else if (command == "make-tx") {
        return cmd_make_tx(parser);
    } else if (command == "sign-tx") {
        return cmd_sign_tx(parser);
    } else if (command == "verify-tx") {
        return cmd_verify_tx(parser);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        print_usage();
        return 1;
    }
}

