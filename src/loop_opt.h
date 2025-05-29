#pragma once
#include "da_dominator.h"
#include "da_reachdef.h"
#include "general_container.h"
#include "ir_basic_block.h"
#include "ir_function.h"
#include "tem_map.h"
#include "utils.h"

typedef struct LoopInfo {
    IRFunction *func;
    IRBasicBlock *header;
    VecPtr backedge_starts;
    VecPtr nodes;
    VecPtr exits;
} LoopInfo;

void MTD(LoopInfo, init, /, IRFunction *func, IRBasicBlock *header);
void MTD(LoopInfo, drop, /);
DELETED_CLONER(LoopInfo, FUNC_STATIC);

DECLARE_MAPPING(MapHeaderToLoopInfo, IRBasicBlock *, LoopInfo, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CLASS_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

typedef struct LoopOpt {
    IRFunction *func;

    // some data structures
    MapPtrPtr stmt_to_bb;
    MapUSizeToDynIRStmt param_to_stmt;

    // analysis
    DominatorDA dom_da;
    ReachDefDA reach_def_da;

    // loop infos
    MapHeaderToLoopInfo loop_infos;
} LoopOpt;

void MTD(LoopOpt, init, /, IRFunction *func);
void MTD(LoopOpt, drop, /);

// utils
bool MTD(LoopOpt, is_dom_bb, /, IRBasicBlock *a, IRBasicBlock *b);

// prepare
void MTD(LoopOpt, prepare, /);

// optimize
bool MTD(LoopOpt, invariant_compute_motion, /);
bool MTD(LoopOpt, induction_var_optimize, /);
