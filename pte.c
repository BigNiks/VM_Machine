//
// Created by niko.madriz on 7/10/2025.
//

#include "pte.h"

VOID set_pte_valid(ULONG64 frameNumber, PPTE pte) {
    pte->validFormat.frameNumber = frameNumber;
    pte->validFormat.valid = 1;
}