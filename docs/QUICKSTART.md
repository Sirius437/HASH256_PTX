# Quick Start Guide

Get up and running with SHA256 PTX in 5 minutes!

## Prerequisites

```bash
# Check CUDA installation
nvcc --version

# Check GPU
nvidia-smi

# Check Python
python3 --version
```

## Installation

```bash
# Clone or download the repository
cd HASH256_PTX

# Build everything
make

# Run tests
make test
```

## Expected Output

```
Running SHA256 PTX tests...
==============================
PTX SHA256 Kernel Test
═══════════════════════════════════════════════════════════════

Input pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
CPU SHA256: 0f715baf5d4c2ed329785cef29e562f73488c8a2bb9dbc5700b361d54b9b0554

Initializing PTX kernel...
✓ PTX kernel initialized

Testing single key...
GPU SHA256: 0f715baf5d4c2ed329785cef29e562f73488c8a2bb9dbc5700b361d54b9b0554
✓ GPU hash matches CPU hash!

Testing batch of 1000 keys...
✓ All 1000 hashes match!

═══════════════════════════════════════════════════════════════
✓ All PTX SHA256 tests passed!
═══════════════════════════════════════════════════════════════

═══════════════════════════════════════════════════════════════
Throughput Benchmark (10 seconds)
═══════════════════════════════════════════════════════════════
Warming up GPU...
Running benchmark...

Results:
  Duration:        10.00 seconds
  Total hashes:    326,100,000
  Iterations:      32,610
  Throughput:      32.61 MH/s
  Avg batch time:  0.307 ms

═══════════════════════════════════════════════════════════════
✓ Benchmark complete!
═══════════════════════════════════════════════════════════════
```

## Basic Usage Example

```cpp
#include "ptx_sha256.hpp"

int main() {
    // Initialize
    PTX_SHA256 sha256;
    if (!sha256.initialize("ptx/sha256_kernel_full.ptx")) {
        return 1;
    }
    
    // Hash a 33-byte compressed public key
    uint8_t pubkey[33] = {
        0x02, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB,
        0xAC, 0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B,
        0x07, 0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28,
        0xD9, 0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    
    uint8_t hash[32];
    sha256.hash_batch(pubkey, hash, 1);
    
    // Print result
    printf("SHA256: ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
    
    return 0;
}
```

## Troubleshooting

### "CUDA driver/runtime version mismatch"
```bash
# Reboot or reload NVIDIA driver
sudo rmmod nvidia_uvm nvidia_drm nvidia_modeset nvidia
sudo modprobe nvidia
```

### "PTX kernel failed to load"
```bash
# Regenerate PTX
make distclean
make
```

### "Compute capability not supported"
Check your GPU's compute capability:
```bash
nvidia-smi --query-gpu=compute_cap --format=csv
```

Minimum required: 8.0 (edit `generate_sha256_ptx.py` line 5 to change target)

## Next Steps

1. Read [README.md](README.md) for detailed documentation
2. Check [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md) for technical details
3. Explore `generate_sha256_ptx.py` to customize the kernel
4. Run `make reference` to see SHA256 computation details

## Support

For issues or questions:
- Check existing documentation
- Review test output for specific errors
- Verify CUDA installation and GPU compatibility

## Performance Tips

1. **Batch Processing**: Hash multiple keys at once for better GPU utilization
2. **Memory Pinning**: Use pinned memory for faster transfers
3. **Streams**: Use CUDA streams for overlapping computation and transfer
4. **Remove Debug**: Comment out debug output in production for smaller kernel

Happy hashing! 🚀
