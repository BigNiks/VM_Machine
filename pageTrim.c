//
// Created by nikop on 7/21/2025.
//

#include "pageTrim.h"

#include "dataStructure.h"
#include "macros.h"
#include "debug.h"
#include "writer.h"

VOID pageTrim() {
    //Choose a victim to remove from active list
    PPFN victim = getVictim(&headActiveList);
    victim->status = PFN_MODIFIED;
    add_entry(&headModifiedList, victim);
    NULL_CHECK(victim, "pageTrim : Vicitm is null");
    //Get the PTE associated with the victim
    PPTE pte = victim->PTE;
    NULL_CHECK(pte, "pageTrim : pte is null");
    //Get the virtual address for the PTE
    ULONG64 va = (ULONG64) pte_to_va(pte);
    //Unmap the virtual address from the victim's physical frame
    if (MapUserPhysicalPages(va, 1, NULL) == FALSE) {
        DebugBreak();
        printf("trim_page : VA unmap unsuccessful");
    }
    //Save contents of victim to disk
    diskWrite(victim);
    removeFromList(&headModifiedList);
    victim->status = PFN_STANDBY;
    add_entry(&headStandbyList, victim);
    //Update the PTE to be invalid
    victim->PTE->invalidFormat.mustBeZero = 0;
    //Stamp the PTE with the disk index related to the page
    victim->PTE->invalidFormat.diskIndex = diskPageIndex;
}
