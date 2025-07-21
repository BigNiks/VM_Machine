//
// Created by niko.madriz on 7/10/2025.
//

#ifndef DATASTRUCTURE_H
#define DATASTRUCTURE_H

#define SUPPORT_MULTIPLE_VA_TO_SAME_PAGE 0

#pragma comment(lib, "advapi32.lib")

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE
#pragma comment(lib, "onecore.lib")
#endif

#define PAGE_SIZE                   4096

#define MB(x)                       ((x) * 1024 * 1024)

#define NUM_OF_ITERATIONS           1000000

//
// This is intentionally a power of two so we can use masking to stay
// within bounds.
//

#define VIRTUAL_ADDRESS_SIZE        MB(16)

#define VIRTUAL_ADDRESS_SIZE_IN_UNSIGNED_CHUNKS        (VIRTUAL_ADDRESS_SIZE / sizeof (ULONG_PTR))

//
// Deliberately use a physical page pool that is approximately 1% of the
// virtual address space !
//

#define NUMBER_OF_PHYSICAL_PAGES   ((VIRTUAL_ADDRESS_SIZE / PAGE_SIZE) / 64)

#define DISK_SIZE (VIRTUAL_ADDRESS_SIZE - (NUMBER_OF_PHYSICAL_PAGES * PAGE_SIZE) + (PAGE_SIZE * 2))

#define MAX_DISK_INDEX (DISK_SIZE / PAGE_SIZE)

#endif //DATASTRUCTURE_H
