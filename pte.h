//
// Created by niko.madriz on 7/10/2025.
//
#include "Windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>

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


PULONG_PTR pte_to_va(PPTE pte);
PPTE va_to_pte(PULONG_PTR address);

#endif //PTE_H
