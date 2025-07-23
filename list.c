//
// Created by nikop on 7/21/2025.
//

#include "list.h"

PPFN listRemove(PLIST_ENTRY head) {
    //Check if list is empty
    if (head->Flink == head) {
        printf("List_remove : empty list");
        return NULL;
    }
    //Cast the first entry in the list as a PPFN, then get the next PFN after the one we're removing
    PPFN freePage = (PPFN) head->Flink;
    PPFN nextpage = (PPFN) freePage->entry.Flink;
    //Make it so that head's flink skips the removed entry
    head->Flink = &nextpage->entry;
    //Update nextpage blink to point back to head
    nextpage->entry.Blink = head;

    return freePage;
}

void add_entry (PLIST_ENTRY head, PFN* newpfn) {
    PLIST_ENTRY Flink;
    //Get the first element after head
    Flink = head->Flink;
    //Link the new entry's flink to current first element
    newpfn->entry.Flink = Flink;
    //Do the same thing but now with the blink pointing to the head
    newpfn->entry.Blink = head;
    //Update the backward pointer to the new entry
    Flink->Blink = &newpfn->entry;
    //Make head's flink point to the new entry
    head->Flink = &newpfn->entry;
}

PPFN getFreePage() {
    PLIST_ENTRY head = &headFreeList;
    PPFN freePage;
    // Check if Free list is empty
    if(IsListEmpty(head)){
        pageTrim();
        head = &headStandbyList;
    }
    // Get a free page from the free list or standby list
    freePage = listRemove(head);
    if (freePage == NULL) {
        printf("full_virtual_memory_test: freePage is null");
    }
    return freePage;
}

PPFN getVictim(PLIST_ENTRY head) {
    if (head->Flink == head) {
        printf("find_victim : empty list");
    }
    PPFN victim = (PPFN)head->Blink;
    PPFN previousPage = (PPFN)victim->entry.Blink;
    previousPage->entry.Flink = head;
    head->Blink = &previousPage->entry;
    return victim;
}

void removeFromList(PLIST_ENTRY head) {
    if (head->Flink == head) {
        printf("removePage : empty list");
        return;
    }
    PPFN pageRemoved = (PPFN)head->Flink;
    PPFN nextPage = (PPFN)pageRemoved->entry.Flink;
    if (nextPage == NULL) {
        DebugBreak();
    }
    head->Flink = &nextPage->entry;
    nextPage->entry.Blink = head;
}
