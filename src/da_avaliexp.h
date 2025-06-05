#pragma once

#include "dataflow_analysis.h"
#include "general_container.h"
#include "ir_value.h"
#include "op.h"

struct IRFunction;

typedef struct AEExp {
    ArithopKind op;
    IRValue left, right;
} AEExp;
int NSMTD(AEExp, compare, /, const AEExp *a, const AEExp *b);

DECLARE_MAPPING(MapAEExpToVar, AEExp, usize, FUNC_EXTERN, GENERATOR_PLAIN_KEY,
                GENERATOR_PLAIN_VALUE, GENERATOR_CLASS_COMPARATOR);

typedef struct AEFact {
    bool is_universal;
    SetUSize avaliset;
} AEFact;

void MTD(AEFact, init, /);
void MTD(AEFact, drop, /);
bool MTD(AEFact, get, /, usize key);
bool MTD(AEFact, set, /, usize key, bool value);
void MTD(AEFact, debug_print, /);

DECLARE_MAPPING(MapBBToAEFact, struct IRBasicBlock *, AEFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
FUNC_STATIC void NSMTD(MapBBToAEFact, drop_value, /, AEFact **value) {
    DROPOBJHEAP(AEFact, *value);
}
FUNC_STATIC AEFact *NSMTD(MapBBToAEFact, clone_value, /,
                          ATTR_UNUSED AEFact *const *other) {
    PANIC("Disable clone");
}

typedef struct AvaliExpDA {
    DataflowAnalysisBase base;

    MapAEExpToVar exp_to_var;
    MapBBToAEFact in_facts;
    MapBBToAEFact out_facts;
    MapUSizeToVecUSize var_to_kills;
} AvaliExpDA;
void MTD(AvaliExpDA, init, /);
void MTD(AvaliExpDA, prepare, /, struct IRFunction *func);
void MTD(AvaliExpDA, clean_redundant_exp, /, struct IRFunction *func);
DEFINE_DATAFLOW_ANALYSIS_STRUCT(AvaliExpDA);
