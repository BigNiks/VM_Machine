//
// Created by nikop on 7/22/2025.
//

#include "handler.h"

VOID set_pte_valid(PPFN freePage, PPTE pte, PULONG_PTR faultedVA) {
    PLIST_ENTRY head;
    ULONG64 frameNumber;
    frameNumber = freePage->frameNumber;
    freePage->PTE = pte;
    pte->validFormat.valid = 1;
    freePage->status = PFN_ACTIVE;
    head = &headActiveList;
    add_entry(head, freePage);
    if (MapUserPhysicalPages (faultedVA, 1, &frameNumber) == FALSE) {
        DebugBreak();
        printf ("full_virtual_memory_test : could not map VA %p to page %llX\n", faultedVA, frameNumber);
    }
}

void handleFault(PULONG_PTR arbitraryVA) {
    PPFN freePage;
    PPTE pte = va_to_pte(arbitraryVA);
    //Checks if free list is empty, if it is then we page trim
    freePage = getFreePage();
    NULL_CHECK(freePage, "Page fault handler : free page is null");
    ULONG64 frameNumber = freePage->frameNumber;
    //checks if the pte has a saved disk index connected to it
    //If so then we read the contents from disk
    if (pte->invalidFormat.diskIndex != 0) {
        diskRead(pte->invalidFormat.diskIndex, freePage);
    }
    // If this PTE has never been accessed before, we need
    // to zero the page we got because it was NOT read from disk.
    else {
        zeroPage(frameNumber);
    }
    //This function sets the pte to be valid
    set_pte_valid(freePage, pte, arbitraryVA);
}