//
// Created by nikop on 7/22/2025.
//
#include "pfn.h"
#include "pte.h"
#include "dataStructure.h"

#ifndef UTIL_H
#define UTIL_H

void checkVa(PULONG64 va);
void zeroPage(ULONG64 frameNumber);

#endif //UTIL_H
