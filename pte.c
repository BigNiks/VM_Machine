//
// Created by niko.madriz on 7/10/2025.
//

#include "pte.h"

#include "dataStructure.h"
#include "list.h"

PPTE va_to_pte(PULONG_PTR address) {
    ULONG64 index = ((ULONG64) address - (ULONG64) vaStart) / PAGE_SIZE;
    PPTE pte = pageTable + index;
    return pte;
}

PULONG_PTR pte_to_va(PPTE pte) {

    ULONG64 index = (pte - pageTable);
    return (PULONG_PTR)((index * PAGE_SIZE) + (ULONG_PTR) vaStart);

}