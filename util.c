//
// Created by nikop on 7/22/2025.
//

#include "util.h"

void checkVa(PULONG64 va) {
    va = (PULONG64) ((ULONG64)va & ~(PAGE_SIZE - 1));
    for (int i = 0; i < PAGE_SIZE / 8; ++i) {
        if (!(*va == 0 || *va == (ULONG64) va)) {
            DebugBreak();
        }
        va += 1;
    }
}

void zeroPage(ULONG64 frameNumber) {
    if (MapUserPhysicalPages(transferVA, 1, &frameNumber) == FALSE) {
        printf("diskRead: failed to map\n");
        DebugBreak();
    }

    memset(transferVA, 0, PAGE_SIZE);

    if (MapUserPhysicalPages(transferVA, 1, NULL) == FALSE) {
        printf("diskRead: failed to unmap\n");
        DebugBreak();
    }
}