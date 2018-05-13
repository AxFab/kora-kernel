// #include <kernel/core.h>
// #include <assert.h>
// #include <string.h>

// #define ACPI_SDT_SIZE 36  // size of acpi sdt header

// void acpi_enter();
// void acpi_leave();


// PACK(struct ACPI_RSD {
//     uint8_t signature[8];
//     uint8_t unk0[7];
//     uint8_t revision;
//     // RSDT
//     uint32_t unk1;
//     uint32_t table_size;
// });

// int acpi_initialize()
// {
//     /* Look for ACPI RSDP in low memory between 0xE0000 -> 0xFFFFF */
//     struct ACPI_RSD *rsdp = NULL;
//     uint8_t *ptr = (uint8_t *)0xE0000;
//     for (; (size_t)ptr < 0xFFFFF; ptr += 16) {
//         if (memcmp(ptr, "RSD PTR ", 8) == 0) {
//             rsdp = (struct ACPI_RSD *)ptr;
//             break;
//         }
//     }

//     if (rsdp == NULL) {
//         kprintf(KLOG_ERR, "[ACPI] Unable to find RSDP\n");
//         return -1;
//     }

//     kprintf(KLOG_DBG, "[ACPI] Found RSDP, revision %d, %d tables\n", rsdp->revision,
//             (rsdp->table_size - ACPI_SDT_SIZE) / 16);

//     acpi_enter();
//     // Enable ACPI; !?


//     return 0;

// }

// // void acpi_AML()
// // {
// //   uint8_t* stack = kmap(PAGE_SIZE, NULL, 0, VMA_FP_STACK);
// //   uint8_t* sp = &stack[PAGE_SIZE];

// //   acpi_enter();

// // }

// // void acpi_package(const char* pname, int scope)
// // {
// // }

// // /**
// //  * Put the system to sleep using ACPI power managment
// //  */
// // void acpi_sleep_x86(int type)
// // {
// //   char pname[8];
// //   assert(type >= 0 && type <= 5);
// //   snprintf(pname, 8, "_S%d_", type);

// //   acpi_package(pname, 0);
// // }


