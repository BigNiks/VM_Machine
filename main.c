#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "macros.h"
#include "debug.h"
#include "pte.h"
#include "dataStructure.h"

//
// This define enables code that lets us create multiple virtual address
// mappings to a single physical page.  We only/need want this if/when we
// start using reference counts to avoid holding locks while performing
// pagefile I/Os - because otherwise disallowing this makes it easier to
// detect and fix unintended failures to unmap virtual addresses properly.
//


BOOL
GetPrivilege  (
    VOID
    )
{
    struct {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege [1];
    } Info;

    //
    // This is Windows-specific code to acquire a privilege.
    // Understanding each line of it is not so important for
    // our efforts.
    //

    HANDLE hProcess;
    HANDLE Token;
    BOOL Result;

    //
    // Open the token.
    //

    hProcess = GetCurrentProcess ();

    Result = OpenProcessToken (hProcess,
                               TOKEN_ADJUST_PRIVILEGES,
                               &Token);

    if (Result == FALSE) {
        printf ("Cannot open process token.\n");
        return FALSE;
    }

    //
    // Enable the privilege.
    //

    Info.Count = 1;
    Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Get the LUID.
    //

    Result = LookupPrivilegeValue (NULL,
                                   SE_LOCK_MEMORY_NAME,
                                   &(Info.Privilege[0].Luid));

    if (Result == FALSE) {
        printf ("Cannot get privilege\n");
        return FALSE;
    }

    //
    // Adjust the privilege.
    //

    Result = AdjustTokenPrivileges (Token,
                                    FALSE,
                                    (PTOKEN_PRIVILEGES) &Info,
                                    0,
                                    NULL,
                                    NULL);

    //
    // Check the result.
    //

    if (Result == FALSE) {
        printf ("Cannot adjust token privileges %u\n", GetLastError ());
        return FALSE;
    }

    if (GetLastError () != ERROR_SUCCESS) {
        printf ("Cannot enable the SE_LOCK_MEMORY_NAME privilege - check local policy\n");
        return FALSE;
    }

    CloseHandle (Token);

    return TRUE;
}

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE

HANDLE
CreateSharedMemorySection (
    VOID
    )
{
    HANDLE section;
    MEM_EXTENDED_PARAMETER parameter = { 0 };

    //
    // Create an AWE section.  Later we deposit pages into it and/or
    // return them.
    //

    parameter.Type = MemSectionExtendedParameterUserPhysicalFlags;
    parameter.ULong = 0;

    section = CreateFileMapping2 (INVALID_HANDLE_VALUE,
                                  NULL,
                                  SECTION_MAP_READ | SECTION_MAP_WRITE,
                                  PAGE_READWRITE,
                                  SEC_RESERVE,
                                  0,
                                  NULL,
                                  &parameter,
                                  1);

    return section;
}

#endif


typedef struct {

    LIST_ENTRY entry;
    PTE *PTE;
    ULONG64 frameNumber;

} PFN, *PPFN;

LIST_ENTRY headFreeList;
LIST_ENTRY headActiveList;
PULONG_PTR vaStart;
PPTE pageTable;
PPTE currentPTE;
PVOID transferVA;
ULONG_PTR virtual_address_size;

ULONG64 diskPageIndex;
boolean* diskPages;
PVOID disk;

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

PPTE va_to_pte(PULONG_PTR address) {
    ULONG64 index = ((ULONG64) address - (ULONG64) vaStart) / PAGE_SIZE;
    PPTE pte = pageTable + index;
    return pte;
}

PULONG_PTR pte_to_va(PPTE pte) {

    ULONG64 index = (pte - pageTable);
    return (PULONG_PTR)((index * PAGE_SIZE) + (ULONG_PTR) vaStart);

}


void initialize_lists (PULONG_PTR physical_page_number, PPFN pfnarray, ULONG_PTR physical_page_count) {
    InitializeListHead(&headFreeList);
    InitializeListHead(&headActiveList);
    PPFN pfn;
    PLIST_ENTRY entry = &headFreeList;
    for (int i = 0; i < physical_page_count; i++) {
        pfn = &pfnarray[i];
        pfn->frameNumber = physical_page_number[i];
        add_entry(entry, pfn);
    }
}

VOID diskWrite(PPFN pfn) {
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
        return;
    }
}

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
    diskPages[diskIndex] = 0;
}


VOID pageTrim() {
    //Choose a victim to remove from active list
    PPFN victim = listRemove(&headActiveList);
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
    //Update the PTE to be invalid
    victim->PTE->invalidFormat.mustBeZero = 0;
    //Stamp the PTE with the disk index related to the page
    victim->PTE->invalidFormat.diskIndex = diskPageIndex;
    //Add victim back to free list
    add_entry(&headFreeList, victim);
}


VOID
full_virtual_memory_test (
    VOID
    )
{
    unsigned i;
    PULONG_PTR arbitrary_va;
    unsigned random_number;
    BOOL allocated;
    BOOL page_faulted;
    BOOL privilege;
    BOOL obtained_pages;
    ULONG_PTR physical_page_count;
    PULONG_PTR physical_page_numbers;
    HANDLE physical_page_handle;
    ULONG_PTR virtual_address_size_in_unsigned_chunks;

    //
    // Allocate the physical pages that we will be managing.
    //
    // First acquire privilege to do this since physical page control
    // is typically something the operating system reserves the sole
    // right to do.
    //

    privilege = GetPrivilege ();

    if (privilege == FALSE) {
        printf ("full_virtual_memory_test : could not get privilege\n");
        return;
    }

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE

    physical_page_handle = CreateSharedMemorySection ();

    if (physical_page_handle == NULL) {
        printf ("CreateFileMapping2 failed, error %#x\n", GetLastError ());
        return;
    }

#else

    physical_page_handle = GetCurrentProcess ();

#endif

    physical_page_count = NUMBER_OF_PHYSICAL_PAGES;

    physical_page_numbers = malloc (physical_page_count * sizeof (ULONG_PTR));

    if (physical_page_numbers == NULL) {
        printf ("full_virtual_memory_test : could not allocate array to hold physical page numbers\n");
        return;
    }

    allocated = AllocateUserPhysicalPages (physical_page_handle,
                                           &physical_page_count,
                                           physical_page_numbers);

    if (allocated == FALSE) {
        printf ("full_virtual_memory_test : could not allocate physical pages\n");
        return;
    }

    if (physical_page_count != NUMBER_OF_PHYSICAL_PAGES) {

        printf ("full_virtual_memory_test : allocated only %llu pages out of %u pages requested\n",
                physical_page_count,
                NUMBER_OF_PHYSICAL_PAGES);
    }

    //f(va) = pte
    //f(pte) = va
    //try to randomly access a page
    //malloc for pte space = same num of virtual pages
    //insert tail into the active list from the free list

    //
    // Reserve a user address space region using the Windows kernel
    // AWE (address windowing extensions) APIs.
    //
    // This will let us connect physical pages of our choosing to
    // any given virtual address within our allocated region.
    //
    // We deliberately make this much larger than physical memory
    // to illustrate how we can manage the illusion.
    //

    virtual_address_size = 64 * physical_page_count * PAGE_SIZE;

    //
    // Round down to a PAGE_SIZE boundary.
    //

    virtual_address_size &= ~PAGE_SIZE;

    virtual_address_size_in_unsigned_chunks =
                        virtual_address_size / sizeof (ULONG_PTR);

#if SUPPORT_MULTIPLE_VA_TO_SAME_PAGE

    MEM_EXTENDED_PARAMETER parameter = { 0 };

    //
    // Allocate a MEM_PHYSICAL region that is "connected" to the AWE section
    // created above.
    //

    parameter.Type = MemExtendedParameterUserPhysicalHandle;
    parameter.Handle = physical_page_handle;

    vaStart = VirtualAlloc2 (NULL,
                       NULL,
                       virtual_address_size,
                       MEM_RESERVE | MEM_PHYSICAL,
                       PAGE_READWRITE,
                       &parameter,
                       1);

#else

    vaStart = VirtualAlloc (NULL,
                      virtual_address_size,
                      MEM_RESERVE | MEM_PHYSICAL,
                      PAGE_READWRITE);

#endif

    if (vaStart == NULL) {

        printf ("full_virtual_memory_test : could not reserve memory %x\n",
                GetLastError ());

        return;
    }
    transferVA = VirtualAlloc(NULL, PAGE_SIZE,
                                MEM_RESERVE | MEM_PHYSICAL,
                                PAGE_READWRITE);
    ULONG64 largestFN = 0;
    for (int i = 0; i < physical_page_count; i++) {
        if (largestFN < physical_page_numbers[i] + 1) {
            largestFN = physical_page_numbers[i] + 1;
        }
    }

    PPFN pfnarr = malloc(largestFN * sizeof(PFN));
    // Error check to see if pfnarray has been allocated
    if (pfnarr == NULL) {
        printf ("full_virtual_memory_test : could not allocate pfnarray\n");
        return;
    }
    //
    memset(pfnarr, 0, largestFN * sizeof(PFN));
    initialize_lists (physical_page_numbers, pfnarr, physical_page_count);
    // Sets up an array of PTE's called the page table
    pageTable = malloc(VIRTUAL_ADDRESS_SIZE / PAGE_SIZE * sizeof(PTE));
    if (pageTable == NULL) {
        printf ("full_virtual_memory_test : could not allocate pageTable\n");
        return;
    }
    ULONG64 numOfPTEs = virtual_address_size / PAGE_SIZE;
    memset(pageTable,0,numOfPTEs * sizeof(PTE));

    for (int i = 0; i < numOfPTEs; i++) {
        pageTable[i].invalidFormat.diskIndex = 0;
        pageTable[i].invalidFormat.mustBeZero = 0;
    }
    initializeDisk();
    //
    // Now perform random accesses.
    //

    for (i = 0; i < NUM_OF_ITERATIONS; i += 1) {
        //
        // Randomly access different portions of the virtual address
        // space we obtained above.
        //
        // If we have never accessed the surrounding page size (4K)
        // portion, the operating system will receive a page fault
        // from the CPU and proceed to obtain a physical page and
        // install a PTE to map it - thus connecting the end-to-end
        // virtual address translation.  Then the operating system
        // will tell the CPU to repeat the instruction that accessed
        // the virtual address and this time, the CPU will see the
        // valid PTE and proceed to obtain the physical contents
        // (without faulting to the operating system again).
        //

        random_number = rand () * rand () * rand ();

        random_number %= virtual_address_size_in_unsigned_chunks;

        //
        // Write the virtual address into each page.  If we need to
        // debug anything, we'll be able to see these in the pages.
        //

        page_faulted = FALSE;

        //
        // Ensure the write to the arbitrary virtual address doesn't
        // straddle a PAGE_SIZE boundary just to keep things simple for
        // now.
        //

        random_number &= ~0x7;

        arbitrary_va = vaStart + random_number;

        __try {

            // This is where we stamp the page
            *arbitrary_va = (ULONG_PTR) arbitrary_va;

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            page_faulted = TRUE;
        }
        // PAGE FAULT HANDLER
        if (page_faulted) {

            //
            // Connect the virtual address now - if that succeeds then
            // we'll be able to access it from now on.
            //
            // THIS IS JUST REUSING THE SAME PHYSICAL PAGE OVER AND OVER !
            //
            // IT NEEDS TO BE REPLACED WITH A TRUE MEMORY MANAGEMENT
            // STATE MACHINE !
            //

            PPFN freePage;
            PPTE pte = va_to_pte(arbitrary_va);
            PLIST_ENTRY head = &headFreeList;
            //Checks if free list is empty, if it is then we page trim
            if (IsListEmpty(&headFreeList)) {
                pageTrim();
            }
            freePage = listRemove(head);
            NULL_CHECK(freePage, "Page fault handler : free page is null");
            ULONG64 frameNumber = freePage->frameNumber;
            //checks if the pte has a saved disk index connected to it
            //If so then we read the contents from disk
            if (pte->invalidFormat.diskIndex != 0) {
                diskRead(pte->invalidFormat.diskIndex, freePage);
            }
            //Here is where we map the VA to the physical page
            if (MapUserPhysicalPages(arbitrary_va, 1, &frameNumber) == FALSE) {
                DebugBreak();
                printf ("full_virtual_memory_test : could not map VA to page");
                return;
            }
            freePage->PTE = pte;
            //This function sets the pte to be valid
            set_pte_valid(frameNumber, pte);
            //Add the page into the active list
            head = &headActiveList;
            add_entry(head, freePage);

            //
            // No exception handler needed now since we have connected
            // the virtual address above to one of our physical pages
            // so no subsequent fault can occur.
            //

            *arbitrary_va = (ULONG_PTR) arbitrary_va;

            //
            // Unmap the virtual address translation we installed above
            // now that we're done writing our value into it.
            //

        }
    }

    printf ("full_virtual_memory_test : finished accessing %u random virtual addresses\n", i);

    //
    // Now that we're done with our memory we can be a good
    // citizen and free it.
    //

    VirtualFree (vaStart, 0, MEM_RELEASE);

    return;
}

//


VOID
main (
    int argc,
    char** argv
    )
{
    //
    // Test a simple malloc implementation - we call the operating
    // system to pay the up front cost to reserve and commit everything.
    //
    // Page faults will occur but the operating system will silently
    // handle them under the covers invisibly to us.
    //

    //malloc_test ();

    //
    // Test a slightly more complicated implementation - where we reserve
    // a big virtual address range up front, and only commit virtual
    // addresses as they get accessed.  This saves us from paying
    // commit costs for any portions we don't actually access.  But
    // the downside is what if we cannot commit it at the time of the
    // fault !
    //

    //commit_at_fault_time_test ();

    //
    // Test our very complicated usermode virtual implementation.
    //
    // We will control the virtual and physical address space management
    // ourselves with the only two exceptions being that we will :
    //
    // 1. Ask the operating system for the physical pages we'll use to
    //    form our pool.
    //
    // 2. Ask the operating system to connect one of our virtual addresses
    //    to one of our physical pages (from our pool).
    //
    // We would do both of those operations ourselves but the operating
    // system (for security reasons) does not allow us to.
    //
    // But we will do all the heavy lifting of maintaining translation
    // tables, PFN data structures, management of physical pages,
    // virtual memory operations like handling page faults, materializing
    // mappings, freeing them, trimming them, writing them out to backing
    // store, bringing them back from backing store, protecting them, etc.
    //
    // This is where we can be as creative as we like, the sky's the limit !
    //

    full_virtual_memory_test ();

    return;
}