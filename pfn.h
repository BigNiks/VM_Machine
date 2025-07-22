//
// Created by nikop on 7/21/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>
#include "pte.h"


#ifndef PFN_H
#define PFN_H

#define PFN_FREE 0x0
#define PFN_ACTIVE 0x1
#define PFN_MODIFIED 0x2
#define PFN_STANDBY 0x3

typedef struct {

    LIST_ENTRY entry;
    PTE *PTE;
    ULONG64 frameNumber;
    ULONG64 status: 2;

} PFN, *PPFN;

#endif //PFN_H
