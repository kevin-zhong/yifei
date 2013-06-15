#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

#if ((__i386__ || __amd64__) && (__GNUC__ || __INTEL_COMPILER))


static inline void yf_cpuid(yf_u32_t i, yf_u32_t *buf);


#if (__i386__)

static inline void
yf_cpuid(yf_u32_t i, yf_u32_t *buf)
{
        __asm__(

                "    mov    %%ebx, %%esi;  "

                "    cpuid;                "
                "    mov    %%eax, (%1);   "
                "    mov    %%ebx, 4(%1);  "
                "    mov    %%edx, 8(%1);  "
                "    mov    %%ecx, 12(%1); "

                "    mov    %%esi, %%ebx;  "

                : : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory");
}


#else /* __amd64__ */


static inline void
yf_cpuid(yf_u32_t i, yf_u32_t *buf)
{
        yf_u32_t eax, ebx, ecx, edx;

        __asm__(

                "cpuid"

                : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i));

        buf[0] = eax;
        buf[1] = ebx;
        buf[2] = edx;
        buf[3] = ecx;
}


#endif


/* auto detect the L2 cache line size of modern and widespread CPUs */

void
yf_cpuinfo(void)
{
        u_char *vendor;
        yf_u32_t vbuf[5], cpu[4], model;

        vbuf[0] = 0;
        vbuf[1] = 0;
        vbuf[2] = 0;
        vbuf[3] = 0;
        vbuf[4] = 0;

        yf_cpuid(0, vbuf);

        vendor = (u_char *)&vbuf[1];

        if (vbuf[0] == 0)
        {
                return;
        }

        yf_cpuid(1, cpu);

        if (yf_strcmp(vendor, "GenuineIntel") == 0)
        {
                switch ((cpu[0] & 0xf00) >> 8)
                {
                /* Pentium */
                case 5:
                        yf_cacheline_size = 32;
                        break;

                /* Pentium Pro, II, III */
                case 6:
                        yf_cacheline_size = 32;

                        model = ((cpu[0] & 0xf0000) >> 8) | (cpu[0] & 0xf0);

                        if (model >= 0xd0)
                        {
                                /* Intel Core, Core 2, Atom */
                                yf_cacheline_size = 64;
                        }

                        break;

                /*
                 * Pentium 4, although its cache line size is 64 bytes,
                 * it prefetches up to two cache lines during memory read
                 */
                case 15:
                        yf_cacheline_size = 128;
                        break;
                }
        }
        else if (yf_strcmp(vendor, "AuthenticAMD") == 0)
        {
                yf_cacheline_size = 64;
        }
}

#else


void
yf_cpuinfo(void)
{
        yf_cacheline_size = 64; //default
}


#endif
