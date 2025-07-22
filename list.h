//
// Created by nikop on 7/21/2025.
//

#ifndef LIST_H
#define LIST_H
#include "pfn.h"
#include "macros.h"
#include "pte.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>

#include "pageTrim.h"
LIST_ENTRY headFreeList;
LIST_ENTRY headActiveList;
LIST_ENTRY headModifiedList;
LIST_ENTRY headStandbyList;

PPFN listRemove(PLIST_ENTRY head);
void add_entry (PLIST_ENTRY head, PFN* newpfn);
PPFN getFreePage();
PPFN getVictim(PLIST_ENTRY head);
void removeFromList(PLIST_ENTRY head);


#endif //LIST_H
