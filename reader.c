//
// Created by nikop on 7/22/2025.
//

#include "reader.h"

VOID diskRead (ULONG64 diskIndex, PPFN pfn) {
    PVOID diskAddress = (PVOID)((ULONG64) disk + (diskIndex * PAGE_SIZE));
    if (MapUserPhysicalPages(transferVA, 1, &pfn->frameNumber) == FALSE) {
        printf("diskRead: failed to map\n");
        DebugBreak();
        return;
    }

    memcpy(transferVA, diskAddress, PAGE_SIZE);

    if (MapUserPhysicalPages(transferVA, 1, NULL) == FALSE) {
        printf("diskRead: failed to unmap\n");
        DebugBreak();
        return;
    }
    //check that disk index is in use
    if (diskPages[diskIndex] == 0) {
        DebugBreak();
    }
    //check that disk index is in bounds
    if (diskIndex > MAX_DISK_INDEX) {
        DebugBreak();
    }

    // this zeroes out the disk
    diskPages[diskIndex] = 0;
}