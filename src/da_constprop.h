#pragma once

#include "dataflow_analysis.h"
#include "ir_function.h"
#include "tem_map.h"
#include "utils.h"

typedef enum CPValueKind {
    CPValueKindUndef,
    CPValueKindNAC,
    CPValueKindConst,
} CPValueKind;

typedef struct CPValue {
    CPValueKind kind;
    int const_val;
} CPValue;

FUNC_STATIC CPValue NSMTD(CPValue, get_undef, /) {
    return (CPValue){
        .kind = CPValueKindUndef,
        .const_val = 0,
    };
}

FUNC_STATIC CPValue NSMTD(CPValue, get_nac, /) {
    return (CPValue){
        .kind = CPValueKindNAC,
        .const_val = 0,
    };
}

FUNC_STATIC CPValue NSMTD(CPValue, get_const, /, int const_val) {
    return (CPValue){
        .kind = CPValueKindConst,
        .const_val = const_val,
    };
}

FUNC_STATIC bool NSMTD(CPValue, eq, /, CPValue a, CPValue b) {
    return a.kind == b.kind &&
           !(a.kind == CPValueKindConst && a.const_val != b.const_val);
}

DECLARE_MAPPING(MapVarToCPValue, usize, CPValue, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

typedef struct CPFact {
    MapVarToCPValue mapping;
} CPFact;

void MTD(CPFact, init, /);
void MTD(CPFact, drop, /);
CPValue MTD(CPFact, get, /, usize key);
bool MTD(CPFact, update, /, usize key, CPValue value);
void MTD(CPFact, debug_print, /);

DECLARE_MAPPING(MapBBToCPFact, IRBasicBlock *, CPFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
FUNC_STATIC void NSMTD(MapBBToCPFact, drop_value, /, CPFact **value) {
    DROPOBJHEAP(CPFact, *value);
}
FUNC_STATIC CPFact *NSMTD(MapBBToCPFact, clone_value, /,
                          ATTR_UNUSED CPFact *const *other) {
    PANIC("Disable clone");
}

typedef struct ConstPropDA {
    DataflowAnalysisBase base;

    MapBBToCPFact in_facts;
    MapBBToCPFact out_facts;
} ConstPropDA;
void MTD(ConstPropDA, init, /);
void MTD(ConstPropDA, const_fold, /, IRFunction *func);

DEFINE_DATAFLOW_ANALYSIS_STRUCT(ConstPropDA);
