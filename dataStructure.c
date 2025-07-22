#include "dataStructure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfn.h"
#include "macros.h"
#include "list.h"
//
// Created by nikop on 7/21/2025.
//

VOID initializeDisk() {
    disk = malloc(DISK_SIZE);
    //checks to make sure previous step was successful
    if (disk == NULL) {
        printf("initializeDisk : disk space malloc unsuccessful");
    }
    //
    memset(disk, 0, DISK_SIZE);
    diskPages = malloc(DISK_SIZE / PAGE_SIZE);
    if (diskPages == NULL) {
        printf("initializeDisk : disk page malloc unsuccessful");
    }
    //If 1, then page is already in use and is unaccessible
    memset(diskPages, 0, DISK_SIZE / PAGE_SIZE);
    //We skip the first index 0 so when we page fault, we can check the diskIndex in PTE
    diskPageIndex = 1;
}

void initialize_lists (PULONG_PTR physical_page_number, PPFN pfnarray, ULONG_PTR physical_page_count) {
    InitializeListHead(&headFreeList);
    InitializeListHead(&headActiveList);
    InitializeListHead(&headStandbyList);
    InitializeListHead(&headModifiedList);
    PPFN pfn;
    PLIST_ENTRY entry = &headFreeList;
    for (int i = 0; i < physical_page_count; i++) {
        pfn = &pfnarray[i];
        pfn->frameNumber = physical_page_number[i];
        add_entry(entry, pfn);
    }
}
