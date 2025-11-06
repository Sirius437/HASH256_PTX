/*
 * SHA256 implementation for HASH256_PTX
 * Based on public domain implementations
 */

#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <string.h>

class SHA256 {
public:
    SHA256();
    void Init();
    void Update(const uint8_t* data, size_t len);
    void Final(uint8_t* hash);
    
    // Convenience function for single-shot hashing
    static void Hash(const uint8_t* data, size_t len, uint8_t* hash);
    
private:
    void Transform(const uint8_t* data);
    
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
};

#endif // SHA256_H
