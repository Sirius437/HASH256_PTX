#!/usr/bin/env python3
"""
Compute complete SHA256 reference values for debugging
This generates all W values and round states for comparison with GPU output
"""

def rotr(x, n):
    """Rotate right"""
    return ((x >> n) | (x << (32 - n))) & 0xffffffff

def shr(x, n):
    """Shift right"""
    return x >> n

def ch(x, y, z):
    """Choice function"""
    return (x & y) ^ (~x & z) & 0xffffffff

def maj(x, y, z):
    """Majority function"""
    return ((x & y) ^ (x & z) ^ (y & z)) & 0xffffffff

def ep0(x):
    """Sigma0 (uppercase) - for rounds"""
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22)

def ep1(x):
    """Sigma1 (uppercase) - for rounds"""
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25)

def sig0(x):
    """sigma0 (lowercase) - for message schedule"""
    return rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3)

def sig1(x):
    """sigma1 (lowercase) - for message schedule"""
    return rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10)

# SHA256 K constants
K = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
]

def sha256_transform(message_bytes):
    """
    Perform SHA256 transform on a 64-byte block
    Returns all intermediate values for debugging
    """
    # Initial hash values
    h0 = 0x6a09e667
    h1 = 0xbb67ae85
    h2 = 0x3c6ef372
    h3 = 0xa54ff53a
    h4 = 0x510e527f
    h5 = 0x9b05688c
    h6 = 0x1f83d9ab
    h7 = 0x5be0cd19
    
    # Prepare message schedule W[0..63]
    W = [0] * 64
    
    # First 16 words from message (big-endian)
    for i in range(16):
        W[i] = int.from_bytes(message_bytes[i*4:(i+1)*4], 'big')
    
    # Extend to 64 words
    for i in range(16, 64):
        W[i] = (sig1(W[i-2]) + W[i-7] + sig0(W[i-15]) + W[i-16]) & 0xffffffff
    
    # Initialize working variables
    a, b, c, d, e, f, g, h = h0, h1, h2, h3, h4, h5, h6, h7
    
    # Store states after each round
    states = []
    
    # 64 rounds
    for i in range(64):
        t1 = (h + ep1(e) + ch(e, f, g) + K[i] + W[i]) & 0xffffffff
        t2 = (ep0(a) + maj(a, b, c)) & 0xffffffff
        
        h = g
        g = f
        f = e
        e = (d + t1) & 0xffffffff
        d = c
        c = b
        b = a
        a = (t1 + t2) & 0xffffffff
        
        states.append({
            'round': i,
            'W': W[i],
            'a': a, 'b': b, 'c': c, 'd': d,
            'e': e, 'f': f, 'g': g, 'h': h,
            't1': t1, 't2': t2
        })
    
    # Final hash
    h0 = (h0 + a) & 0xffffffff
    h1 = (h1 + b) & 0xffffffff
    h2 = (h2 + c) & 0xffffffff
    h3 = (h3 + d) & 0xffffffff
    h4 = (h4 + e) & 0xffffffff
    h5 = (h5 + f) & 0xffffffff
    h6 = (h6 + g) & 0xffffffff
    h7 = (h7 + h) & 0xffffffff
    
    final_hash = b''.join(x.to_bytes(4, 'big') for x in [h0, h1, h2, h3, h4, h5, h6, h7])
    
    return W, states, final_hash

def main():
    # Test vector: compressed public key for private key 0x1
    pubkey = bytes.fromhex('0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798')
    
    # Prepare 64-byte padded block
    block = bytearray(64)
    block[:33] = pubkey
    block[33] = 0x80  # Padding
    # bytes 34-55 are zeros
    block[56:64] = (len(pubkey) * 8).to_bytes(8, 'big')  # Length in bits
    
    print("="*80)
    print("SHA256 Reference Computation")
    print("="*80)
    print(f"\nInput (33 bytes): {pubkey.hex()}")
    print(f"\nPadded block (64 bytes):")
    for i in range(0, 64, 16):
        print(f"  {i:02d}: {block[i:i+16].hex()}")
    
    # Compute SHA256
    W, states, final_hash = sha256_transform(bytes(block))
    
    print(f"\n{'='*80}")
    print("MESSAGE SCHEDULE (W values)")
    print(f"{'='*80}")
    for i in range(64):
        if i < 16:
            print(f"W[{i:2d}] = 0x{W[i]:08x}  (from input)")
        else:
            print(f"W[{i:2d}] = 0x{W[i]:08x}  (extended)")
    
    print(f"\n{'='*80}")
    print("ROUND STATES")
    print(f"{'='*80}")
    print(f"Initial: a=0x6a09e667 b=0xbb67ae85 c=0x3c6ef372 d=0xa54ff53a")
    print(f"         e=0x510e527f f=0x9b05688c g=0x1f83d9ab h=0x5be0cd19")
    print()
    
    for state in states:
        i = state['round']
        print(f"Round {i:2d}: W=0x{state['W']:08x} K=0x{K[i]:08x}")
        print(f"         a=0x{state['a']:08x} e=0x{state['e']:08x}")
        if i < 5 or i >= 60:  # Print details for first 5 and last 4 rounds
            print(f"         b=0x{state['b']:08x} c=0x{state['c']:08x} d=0x{state['d']:08x}")
            print(f"         f=0x{state['f']:08x} g=0x{state['g']:08x} h=0x{state['h']:08x}")
            print(f"         t1=0x{state['t1']:08x} t2=0x{state['t2']:08x}")
        print()
    
    print(f"{'='*80}")
    print(f"FINAL HASH: {final_hash.hex()}")
    print(f"{'='*80}")
    
    # Generate C code for debug output
    print(f"\n{'='*80}")
    print("PTX DEBUG CODE TO ADD")
    print(f"{'='*80}")
    print("\nAdd this to generate_sha256_ptx.py after each round:")
    print("""
    if round_num in [0, 1, 2, 15, 16, 17, 63]:
        offset = 48 + round_num * 8
        code += f'''    // DEBUG: Store a,e after round {round_num}
    st.global.u32   [%output_ptr+{offset}], %a;
    st.global.u32   [%output_ptr+{offset+4}], %e;
'''
    """)
    
    print("\nKey rounds to check:")
    print("  - Rounds 0-2: Basic round function")
    print("  - Round 15: Last round using original W values")
    print("  - Round 16: First round using extended W values")
    print("  - Round 17: Second extended round")
    print("  - Round 63: Final round")

if __name__ == "__main__":
    main()
