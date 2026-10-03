#ifndef FLASHUX_WIRE_H
#define FLASHUX_WIRE_H
#include <stddef.h>
#include <stdint.h>
static inline void flashux_put_u32(unsigned char *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) { p[i] = (unsigned char)(v >> (i * 8)); }
}
static inline uint32_t flashux_get_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void flashux_put_u64(unsigned char *p, uint64_t v)
{
    for (unsigned i = 0; i < 8; ++i) { p[i] = (unsigned char)(v >> (i * 8)); }
}
static inline uint64_t flashux_get_u64(const unsigned char *p)
{
    uint64_t v = 0;
    for (unsigned i = 0; i < 8; ++i) { v |= (uint64_t)p[i] << (i * 8); }
    return v;
}
#endif
