//
// Created by nikop on 7/21/2025.
//

#include "writer.h"
void diskWrite(PPFN pfn) {
    //map physical page to the transfer VA
    if (MapUserPhysicalPages(transferVA, 1, &pfn->frameNumber) == FALSE) {
        printf("diskWrite: failed to map\n");
        DebugBreak();
        return;
    }
    ULONG64 counter = 0;
#define DISK_SLOT_IN_USE 1
    // Look for a disk_page that is available
    while(diskPages[diskPageIndex] == DISK_SLOT_IN_USE && counter != 2) {
        diskPageIndex++;
        // Check if we are at the end of the array
        if (diskPageIndex == DISK_SIZE / PAGE_SIZE){
            // Wrap around the disk
            diskPageIndex = 1;
            counter++;
        }
    }
    if (counter == 2) {
        printf("diskWrite: no available disk pages\n");
        DebugBreak();
    }
    PVOID diskAddress = (PVOID)((ULONG64) disk + diskPageIndex * PAGE_SIZE);
    // Copy contents from transfer va to diskAddress
    memcpy(diskAddress, transferVA, PAGE_SIZE);
    // Make disk page unavailable
    diskPages[diskPageIndex] = DISK_SLOT_IN_USE;
    // Unmap transfer VA
    if (MapUserPhysicalPages(transferVA, 1, NULL) == FALSE) {
        printf("diskWrite: failed to unmap\n");
        DebugBreak();
    }
}
