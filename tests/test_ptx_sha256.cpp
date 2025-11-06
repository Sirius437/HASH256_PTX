/*
 * Test PTX SHA256 kernel
 * Compare against CPU implementation
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
#include "sha256.h"
#include "ptx_sha256.hpp"

void print_hex(const char* label, const uint8_t* data, size_t len) {
    printf("%s: ", label);
    for(size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

bool compare_hashes(const uint8_t* hash1, const uint8_t* hash2, size_t len) {
    return memcmp(hash1, hash2, len) == 0;
}

int main() {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("PTX SHA256 Kernel Test\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Test vector: compressed public key for private key 0x1
    uint8_t test_pubkey[33] = {
        0x02, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC, 0x55, 0xA0, 0x62,
        0x95, 0xCE, 0x87, 0x0B, 0x07, 0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28,
        0xD9, 0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    
    // Compute CPU reference
    uint8_t cpu_hash[32];
    SHA256::Hash(test_pubkey, 33, cpu_hash);
    
    print_hex("Input pubkey", test_pubkey, 33);
    print_hex("CPU SHA256", cpu_hash, 32);
    printf("\n");
    
    // Initialize PTX kernel
    printf("Initializing PTX SHA256 kernel...\n");
    PTX_SHA256 ptx_sha256;
    
    std::string ptx_path = "ptx/sha256_kernel_full.ptx";
    if (!ptx_sha256.initialize(ptx_path)) {
        printf("❌ Failed to initialize PTX kernel\n");
        printf("Make sure ptx/sha256_kernel.ptx exists\n");
        return 1;
    }
    printf("✓ PTX kernel initialized\n\n");
    
    // Test single key
    printf("Testing single key...\n");
    uint8_t gpu_hash[32];
    if (!ptx_sha256.hash_batch(test_pubkey, gpu_hash, 1)) {
        printf("❌ Failed to hash on GPU\n");
        return 1;
    }
    
    print_hex("GPU SHA256", gpu_hash, 32);
    
    if (compare_hashes(cpu_hash, gpu_hash, 32)) {
        printf("✓ GPU hash matches CPU hash!\n\n");
    } else {
        printf("❌ GPU hash does NOT match CPU hash!\n\n");
        return 1;
    }
    
    // Test batch of keys
    printf("Testing batch of 1000 keys...\n");
    const int batch_size = 1000;
    uint8_t* input_batch = new uint8_t[batch_size * 33];
    uint8_t* cpu_batch = new uint8_t[batch_size * 32];
    uint8_t* gpu_batch = new uint8_t[batch_size * 32];
    
    // Generate test data (vary last byte)
    for (int i = 0; i < batch_size; i++) {
        memcpy(input_batch + i * 33, test_pubkey, 33);
        input_batch[i * 33 + 32] = i & 0xFF;  // Vary last byte
        
        // Compute CPU reference
        SHA256::Hash(input_batch + i * 33, 33, cpu_batch + i * 32);
    }
    
    // Compute GPU hashes
    if (!ptx_sha256.hash_batch(input_batch, gpu_batch, batch_size)) {
        printf("❌ Failed to hash batch on GPU\n");
        delete[] input_batch;
        delete[] cpu_batch;
        delete[] gpu_batch;
        return 1;
    }
    
    // Compare all hashes
    int mismatches = 0;
    for (int i = 0; i < batch_size; i++) {
        if (!compare_hashes(cpu_batch + i * 32, gpu_batch + i * 32, 32)) {
            if (mismatches == 0) {
                printf("❌ Mismatch at key %d:\n", i);
                print_hex("  CPU", cpu_batch + i * 32, 32);
                print_hex("  GPU", gpu_batch + i * 32, 32);
            }
            mismatches++;
        }
    }
    
    if (mismatches == 0) {
        printf("✓ All %d hashes match!\n\n", batch_size);
    } else {
        printf("❌ %d/%d hashes do NOT match!\n\n", mismatches, batch_size);
        delete[] input_batch;
        delete[] cpu_batch;
        delete[] gpu_batch;
        return 1;
    }
    
    delete[] input_batch;
    delete[] cpu_batch;
    delete[] gpu_batch;
    
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("✓ All PTX SHA256 tests passed!\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Throughput benchmark
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Throughput Benchmark (10 million keys)\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    const int benchmark_batch = 10000000;  // Process 10M keys at once
    uint8_t* bench_input = new uint8_t[benchmark_batch * 33];
    uint8_t* bench_output = new uint8_t[benchmark_batch * 32];
    
    // Prepare input data
    for (int i = 0; i < benchmark_batch; i++) {
        memcpy(bench_input + i * 33, test_pubkey, 33);
        bench_input[i * 33 + 32] = (i >> 8) & 0xFF;  // Vary for uniqueness
        bench_input[i * 33 + 31] = i & 0xFF;
    }
    
    // Warmup
    printf("Warming up GPU...\n");
    ptx_sha256.hash_batch(bench_input, bench_output, benchmark_batch);
    
    // Single benchmark run with 10M keys
    printf("Running benchmark with %d keys...\n", benchmark_batch);
    auto start = std::chrono::high_resolution_clock::now();
    
    ptx_sha256.hash_batch(bench_input, bench_output, benchmark_batch);
    
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    
    uint64_t total_hashes = benchmark_batch;
    
    double hashes_per_sec = total_hashes / elapsed;
    double mhashes_per_sec = hashes_per_sec / 1000000.0;
    
    printf("\n");
    printf("Keys processed: %lu\n", total_hashes);
    printf("Time: %.6f seconds\n", elapsed);
    printf("Performance: %.2f MHashes/s\n", mhashes_per_sec);
    printf("Performance: %.5f GHashes/s\n", mhashes_per_sec / 1000.0);
    printf("\n");
    
    delete[] bench_input;
    delete[] bench_output;
    
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("✓ Benchmark complete!\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    return 0;
}
