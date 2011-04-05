#ifndef __KLIB_H__
#define __KLIB_H__

#define ABS(n)  (n > 0 ? n : -n)
#define LS_INLINE __inline __attribute__((always_inline))

typedef unsigned long long uint64_t;
typedef unsigned int       uint32_t;
typedef unsigned short     uint16_t;
typedef unsigned char      uint8_t;
typedef uint32_t bool;

typedef long long int64_t;
typedef int       int32_t;
typedef short     int16_t;
typedef char      int8_t;


/* Prototypes */
void test_colors(void);

LS_INLINE void halt(void);
LS_INLINE void debug(void);
LS_INLINE uint8_t inb(uint16_t port);
LS_INLINE void outb(uint16_t port, uint8_t value);
LS_INLINE void lcr0(uint32_t value);
LS_INLINE uint32_t rcr0(void);
LS_INLINE uint32_t rcr2(void);
LS_INLINE uint32_t rcr3(void);
LS_INLINE uint32_t rcr4(void);


#endif /* __KLIB_H__ */
