/* Xbox x86 implementation: PDCLIB's scalar byte loop dominates repeated
   effect-state clearing under emulation. Preserve standard memset semantics. */
#include <stddef.h>
#include <stdint.h>

void *memset(void *destination, int value, size_t length)
{
    void *original=destination;
    uint32_t repeated=(uint32_t)(unsigned char)value*UINT32_C(0x01010101);
    size_t words=length/4;
    __asm__ volatile("rep stosl" : "+D"(destination), "+c"(words) : "a"(repeated) : "memory");
    length%=4;
    __asm__ volatile("rep stosb" : "+D"(destination), "+c"(length) : "a"(repeated) : "memory");
    return original;
}

int jpb_XboxMemorySelfTest(void)
{
    volatile unsigned char storage[1088];
    const int values[]={0,1,255,-1,0x1234};
    for(unsigned offset=0;offset<16;++offset)
        for(unsigned length=0;length<=1025;length+=(length<32?1:31))
            for(unsigned choice=0;choice<sizeof(values)/sizeof(values[0]);++choice) {
                for(unsigned i=0;i<sizeof(storage);++i)storage[i]=0xa7;
                void *start=(void *)(storage+16+offset);
                if(memset(start,values[choice],length)!=start)return 0;
                for(unsigned i=0;i<sizeof(storage);++i) {
                    unsigned char expected=i>=16+offset && i<16+offset+length ?
                        (unsigned char)values[choice]:0xa7;
                    if(storage[i]!=expected)return 0;
                }
            }
    return 1;
}
