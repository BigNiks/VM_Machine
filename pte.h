//
// Created by niko.madriz on 7/10/2025.
//
#include "Windows.h"

#ifndef PTE_H
#define PTE_H
typedef struct {
    ULONG64 valid: 1;
    ULONG64 frameNumber: 40;
} validPte;

typedef struct {
    // otherwise would be other format
    ULONG64 mustBeZero: 1;
    ULONG64 diskIndex: 40;
    ULONG64 reserved: 23;

} invalidPte;

typedef struct {
    // overlays
    union {
        validPte validFormat;
        invalidPte invalidFormat;
        ULONG64 entireFormat;
    };
} PTE, *PPTE;


VOID set_pte_valid(ULONG64 frameNumber, PPTE pte);

#endif //PTE_H
