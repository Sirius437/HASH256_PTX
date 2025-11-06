# SHA256 PTX Implementation

A high-performance SHA256 implementation using NVIDIA PTX (Parallel Thread Execution) assembly for GPU acceleration.

**Author**: Sirius437  
**License**: MIT  
**Status**: Educational/Research - For production use, see [Performance Comparison](#performance-comparison)

## Overview

This project provides a complete SHA256 hash implementation optimized for NVIDIA GPUs using hand-written PTX assembly. It's designed for hashing 33-byte compressed Bitcoin public keys but can be adapted for other use cases.

**Note**: This is an educational project demonstrating PTX assembly programming. For production Bitcoin address search, the CUDA C++ implementation achieves 32x better performance (see comparison below).

## Features

- ✅ **Pure PTX Assembly**: Hand-optimized PTX code for maximum performance
- ✅ **Fully Unrolled**: All 64 SHA256 rounds are unrolled for speed
- ✅ **Verified Correctness**: Passes all test vectors against reference implementation
- ✅ **Python Generator**: Automated PTX code generation with `generate_sha256_ptx.py`
- ✅ **Debug Support**: Built-in debugging capabilities for development

## Project Structure

```
HASH256_PTX/
├── README.md                      # This file
├── Makefile                       # Build system
├── generate_sha256_ptx.py         # PTX code generator
├── compute_sha256_reference.py    # Reference implementation for testing
├── src/
│   └── sha256.cpp                 # CPU reference implementation
├── include/
│   ├── sha256.h                   # SHA256 header
│   └── ptx_sha256.hpp             # PTX kernel wrapper
├── ptx/
│   └── sha256_kernel_full.ptx     # Generated PTX kernel (5000+ lines)
└── tests/
    └── test_ptx_sha256.cpp        # Test suite
```

## Requirements

- NVIDIA GPU with CUDA Compute Capability 12.0+ (tested on RTX 5070, CC 12.0)
- CUDA Toolkit 12.8+
- g++ compiler with C++11 support
- Python 3.6+ (for PTX generation)

## Building

### Using CMake (Recommended)

```bash
mkdir build && cd build
cmake ..
make -j8
ln -s ../ptx .
./test_ptx_sha256
```

### Using Makefile

```bash
make
make test
```

Both methods will:
1. Generate the PTX kernel using `generate_sha256_ptx.py`
2. Compile the test program with optimizations
3. Run the test suite and benchmark

## Usage

### Basic Example

```cpp
#include "include/ptx_sha256.hpp"

// Initialize PTX kernel
PTX_SHA256 sha256;
sha256.initialize("ptx/sha256_kernel_full.ptx");

// Hash a 33-byte compressed public key
uint8_t pubkey[33] = { /* your data */ };
uint8_t hash[32];
sha256.hash_batch(pubkey, hash, 1);
```

### Batch Processing

```cpp
const int batch_size = 1000;
uint8_t* input = new uint8_t[batch_size * 33];
uint8_t* output = new uint8_t[batch_size * 32];

// Fill input with data...

sha256.hash_batch(input, output, batch_size);
```

```

### Performance Characteristics

- **Registers Used**: 40 (out of available register file)
- **Shared Memory**: 0 bytes
- **Constant Memory**: 916 bytes (K constants + parameters)
- **Kernel Size**: ~4900 lines of PTX, ~163 KB

#### Benchmark Results

**Hardware**: NVIDIA GeForce RTX 5070 (Compute Capability 12.0, 48 SMs)

**Test Configuration**:
- Batch size: 10,000,000 keys
- Build: CMake Release with -O3 -march=native
- Input: 33-byte compressed public keys

**PTX Implementation Results**:
- **Throughput**: 116.51 MH/s (Million Hashes per Second)
- **Total hashes**: 10,000,000 in 0.086 seconds
- **Latency**: ~8.6 nanoseconds per hash
- **Speedup vs CPU**: 42x faster

**Performance Notes**:
- Pure hand-written PTX assembly
- Fully unrolled 64 rounds for maximum throughput
- Uses 40 registers per thread, 128 threads per block
- JIT compiled with optimization level 4

## Performance Comparison

### PTX vs CUDA C++ Implementation

For production use, we recommend the CUDA C++ implementation:

| Implementation | Throughput | Type | Build |
|----------------|------------|------|-------|
| **CUDA Hash160** | **3,790 MH/s** | SHA256 + RIPEMD160 | nvcc optimized |
| **PTX SHA256 (CMake)** | **116.51 MH/s** | SHA256 only | Hand-written PTX |
| **PTX SHA256 (Makefile)** | 109.30 MH/s | SHA256 only | Hand-written PTX |
| **CPU SHA256** | 2.77 MH/s | SHA256 only | g++ -O3 |

**Key Findings**:
- CUDA C++ is **32x faster** than hand-written PTX
- PTX is **42x faster** than CPU
- CUDA compiler (nvcc) produces superior optimizations
- For production Bitcoin address search: **Use CUDA implementation**

**Why CUDA is Faster**:
1. **Compiler optimizations**: nvcc applies advanced instruction scheduling
2. **Memory access patterns**: Better coalescing and caching
3. **Register allocation**: More efficient than manual PTX
4. **Instruction selection**: Uses optimal GPU instructions

**When to Use PTX**:
- Educational purposes (learning GPU assembly)
- Research into low-level GPU programming
- Debugging and understanding CUDA internals
- Custom operations not well-supported by CUDA C++

**Scaling**:
- Performance scales with batch size (optimal: 1M-10M keys)
- Larger batches provide better GPU utilization
- Multiple GPUs scale linearly

### Input Format

The kernel expects 33-byte inputs (compressed Bitcoin public keys) and automatically applies SHA256 padding:
- Bytes 0-32: Input data
- Byte 33: 0x80 (padding marker)
- Bytes 34-55: 0x00 (padding)
- Bytes 56-63: 0x0000000000000108 (length = 264 bits in big-endian)

## Testing

Run the test suite:

```bash
./test_ptx_sha256
```

The test suite verifies:
- ✅ Correct W value loading (rounds 0-15)
- ✅ Correct message schedule extension (rounds 16-63)
- ✅ Correct round computations
- ✅ Final hash matches CPU reference

### Test Vector

Input (33-byte compressed pubkey for private key 0x1):
```
0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
```

Expected SHA256:
```
0f715baf5d4c2ed329785cef29e562f73488c8a2bb9dbc5700b361d54b9b0554
```

## Development

### Regenerating PTX

To regenerate the PTX kernel after modifications:

```bash
python3 generate_sha256_ptx.py
```

### Debug Mode

The generator includes debug output for key rounds. To enable, modify `generate_sha256_ptx.py`:

```python
debug_rounds = {
    0: 48,   # Round 0
    1: 56,   # Round 1
    15: 64,  # Last original W
    16: 72,  # First extended W (critical!)
    17: 80,  # Second extended W
}
```

### Reference Computation

To compute reference values for debugging:

```bash
python3 compute_sha256_reference.py
```

This outputs all W values and round states for comparison.

## Algorithm Details

SHA256 implementation follows FIPS 180-4 specification:

1. **Message Schedule**: 
   - W[0..15]: Direct from input (big-endian)
   - W[16..63]: Extended using σ0 and σ1 functions

2. **Compression Function**:
   - 64 rounds of Ch, Maj, Σ0, Σ1 operations
   - Working variables: a, b, c, d, e, f, g, h

3. **Output**: 
   - Final hash = initial hash + compressed values
   - 32 bytes (256 bits) in big-endian format

## License

This code is part of the HashRuda project. See main project for license details.

## Credits

- **Author**: Sirius437
- Developed for Bitcoin address search acceleration
- Based on FIPS 180-4 SHA256 specification
- Optimized for NVIDIA GPU architectures
- Part of the RCKangaroo/HashRuda project

## Contributing

Contributions welcome! Areas for improvement:
- [ ] Support for arbitrary-length inputs
- [ ] RIPEMD160 PTX implementation
- [ ] Performance benchmarking suite
- [ ] Multi-GPU support

## Changelog

### v1.0.0 (2025-11-06)
- ✅ Initial working implementation
- ✅ Fixed circular buffer bug in message schedule
- ✅ All test vectors passing
- ✅ Verified on RTX 5070 (CC 12.0)
- ✅ CMake build system added
- ✅ Comprehensive performance comparison with CUDA
- ✅ Achieved 116.51 MH/s throughput
- ✅ Production recommendation: Use CUDA implementation (32x faster)

## Acknowledgments

Special thanks to the CUDA and PTX communities for documentation and examples. This project demonstrates the power and complexity of GPU assembly programming while highlighting the impressive optimizations modern CUDA compilers achieve.
