

typedef unsigned long long u64;

#define GDT_ENTRY(base, limit, flags, access)  \
    GDT_ENTRY_((u64)(base), (u64)(limit), (u64)(flags), (u64)(access))

#define GDT_ENTRY_(base, limit, flags, access) \
  (                                                                            \
    (((((base)) >> 24) & 0xFF) << 56) |                                        \
    ((((flags)) & 0xF) << 52) |                                                \
    (((((limit)) >> 16) & 0xF) << 48) |                                        \
    (((((access) | (1 << 4))) & 0xFF) << 40) |                                 \
    ((((base)) & 0xFFF) << 16) |                                               \
    (((limit)) & 0xFFFF)                                                       \
  )



void x86_setup_gdt(u64 *table)
{
    /* GDT - Global Descriptor Table */
    /* Create a first empty descriptor */
    table[0] = GDT_ENTRY(0, 0, 0, 0);
    /* Entry for kernel code */
    table[1] = GDT_ENTRY(0, 0xfffff, 0x0D, 0x9B);
    /* Entry for kernel data */
    table[2] = GDT_ENTRY(0, 0xfffff, 0x0D, 0x93);
    /* Entry for kernel stack */
    table[3] = GDT_ENTRY(0, 0, 0x0D, 0x97);
    /* Entry for user code */
    table[4] = GDT_ENTRY(0, 0xfffff, 0x0D, 0xff);
    /* Entry for user data */
    table[5] = GDT_ENTRY(0, 0xfffff, 0x0D, 0xf3);
    /* Entry for user stack */
    table[6] = GDT_ENTRY(0, 0, 0x0D, 0xf7);
}
