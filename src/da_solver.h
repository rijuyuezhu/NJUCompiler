#pragma once

#include "utils.h"

struct DataflowAnalysisBase;
struct IRFunction;

void NSCALL(DAWorkListSolver, solve, /, struct DataflowAnalysisBase *analysis,
            struct IRFunction *func);
