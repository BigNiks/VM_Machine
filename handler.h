//
// Created by nikop on 7/22/2025.
//
#include "dataStructure.h"
#include "pfn.h"
#include "pte.h"
#include "list.h"
#include "util.h"
#include "debug.h"
#include "reader.h"

#ifndef HANDLER_H
#define HANDLER_H

void set_pte_valid(PPFN freePage, PPTE pte, PULONG_PTR faultedVA);
void handleFault(PULONG_PTR arbitraryVA);

#endif //HANDLER_H
