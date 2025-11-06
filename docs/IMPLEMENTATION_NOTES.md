# SHA256 PTX Implementation Notes

## Development Journey

This document chronicles the development and debugging of the SHA256 PTX kernel.

### Initial Goal

Create a high-performance SHA256 implementation in PTX assembly for hashing Bitcoin compressed public keys (33 bytes) on NVIDIA GPUs.

### The Bug Hunt

#### Symptom
The generated PTX kernel produced incorrect SHA256 hashes. The output was consistently wrong: `35e1411bf3048385...` instead of the expected `0f715baf5d4c2ed3...`

#### Debugging Process

1. **Verified Input Loading** ✓
   - Checked that W[0-3] were loaded correctly as big-endian
   - All input values matched expected

2. **Verified Initial Rounds** ✓
   - Round 0: PASS (a=0xfe8246b3, e=0x9b41a108)
   - Round 1: PASS (a=0x8191bf6b, e=0x00d721f2)
   - Round 15: PASS (a=0xe96102c8, e=0x0ffdedc0)

3. **Found the Divergence** ✗
   - Round 16: FAIL (first round using extended W values)
   - GPU: a=0xf924cdca, e=0x1a6dc230
   - Expected: a=0xed9274a0, e=0x0edb6906

4. **Identified Root Cause**
   - W[16] was computed incorrectly
   - GPU: 0x1c182f20
   - Expected: 0x1085d5f6

#### The Bug

In the message schedule extension, we use a circular buffer for W values (W[0-15] stored in %w0-%w15). When computing W[16], we need to use W[0] as input, but we're also storing the result in W[0]:

```ptx
// BUGGY CODE:
add.u32  %w0, %s1, %w9      // w0 = sigma1(W[14]) + W[9]
add.u32  %w0, %w0, %s0      // w0 = w0 + sigma0(W[1])
add.u32  %w0, %w0, %w0      // BUG! w0 = w0 + w0 (uses NEW w0, not OLD)
```

The problem: `w_i` and `w_i_16` both map to `%w0` when i=16, so the third addition uses the partially computed value instead of the original W[0].

#### The Fix

Save W[i-16] to a temporary register before starting the computation:

```ptx
// FIXED CODE:
mov.u32  %r70, %w0          // Save original W[0]
add.u32  %w0, %s1, %w9      // w0 = sigma1(W[14]) + W[9]
add.u32  %w0, %w0, %s0      // w0 = w0 + sigma0(W[1])
add.u32  %w0, %w0, %r70     // w0 = w0 + (original W[0])
```

### Verification

After the fix:
- ✅ W[16] = 0x1085d5f6 (correct!)
- ✅ W[17] = 0x38699114 (correct!)
- ✅ Round 16: a=0xed9274a0, e=0x0edb6906 (correct!)
- ✅ Round 17: a=0x5837c1b6, e=0x481441c0 (correct!)
- ✅ Final hash matches CPU reference

## Implementation Details

### Register Usage

- `%w0-%w15`: Message schedule (circular buffer)
- `%a-%h`: Working variables
- `%h0-%h7`: Initial/final hash values
- `%s0, %s1`: Message schedule sigma functions (lowercase)
- `%S0, %S1`: Round Sigma functions (uppercase)
- `%r0-%r99`: Temporary registers
- `%t1, %t2`: Round temporaries
- `%ch, %maj`: Choice and Majority functions

### Key Design Decisions

1. **Circular Buffer for W**: Saves registers by reusing W[0-15] for W[16-63]
2. **Separate Sigma Registers**: Prevents conflicts between message schedule and round functions
3. **Fully Unrolled**: All 64 rounds explicitly coded for maximum performance
4. **Big-Endian I/O**: Matches SHA256 specification

### Performance Characteristics

- **Kernel Size**: ~5000 lines PTX, 168 KB
- **Register Pressure**: 40 registers (moderate)
- **Memory Access**: Minimal (only input/output)
- **Throughput**: Limited by ALU operations, not memory

### Current Limitations

1. **Fixed Input Size**: Hardcoded for 33-byte inputs
2. **Single Block**: Only processes one 512-bit block
3. **No Streaming**: Cannot hash arbitrary-length data
4. **Debug Overhead**: Current version includes debug output

### Future Improvements

1. **Remove Debug Code**: ~10% size reduction
2. **Variable Input Length**: Support arbitrary message sizes
3. **Multi-Block**: Handle inputs > 55 bytes
4. **Batch Optimization**: Better memory coalescing
5. **RIPEMD160**: Complete Hash160 implementation

## Testing Strategy

### Test Vectors

**Primary Test**: Bitcoin Generator Point (private key = 1)
```
Input:  0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
SHA256: 0f715baf5d4c2ed329785cef29e562f73488c8a2bb9dbc5700b361d54b9b0554
```

### Debug Points

Critical rounds to verify:
- **Round 0**: First round, tests basic operations
- **Round 1**: Second round, tests state propagation
- **Round 15**: Last round with original W values
- **Round 16**: First extended W (CRITICAL - where bug was)
- **Round 17**: Second extended W
- **Round 63**: Final round

### Verification Method

1. Generate reference values with `compute_sha256_reference.py`
2. Add debug output to PTX for key rounds
3. Compare GPU vs CPU values at each checkpoint
4. Binary search to find first divergence

## Lessons Learned

1. **PTX Register Names Are Case-Sensitive**: %s0 ≠ %S0
2. **Circular Buffers Need Care**: Save values before overwriting
3. **Debug Early**: Instrument critical points from the start
4. **Reference Implementation**: Essential for verification
5. **Incremental Testing**: Test each round individually

## References

- FIPS 180-4: SHA-256 Specification
- NVIDIA PTX ISA Documentation
- Bitcoin BIP-173: Address encoding

## Acknowledgments

Developed as part of the HashRuda/RCKangaroo project for Bitcoin address search acceleration.

---

**Status**: ✅ Working and verified
**Date**: 2025-11-06
**GPU Tested**: NVIDIA GeForce RTX 5070 (Compute Capability 12.0)
