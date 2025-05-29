#pragma once
#include "da_dominator.h"
#include "da_reachdef.h"
#include "general_container.h"
#include "ir_function.h"

typedef struct LoopOpt {
    IRFunction *func;

    // some data structures
    MapPtrPtr stmt_to_bb;
    MapUSizeToDynIRStmt param_to_stmt;

    // analysis
    DominatorDA dom_da;
    ReachDefDA reach_def_da;
} LoopOpt;

void MTD(LoopOpt, init, /, IRFunction *func);
void MTD(LoopOpt, drop, /);
void MTD(LoopOpt, prepare, /);
bool MTD(LoopOpt, invariant_compute_motion, /);
bool MTD(LoopOpt, induction_var_optimize, /);
