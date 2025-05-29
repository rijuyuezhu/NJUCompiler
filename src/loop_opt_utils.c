#include "loop_opt.h"

bool MTD(LoopOpt, is_dom_bb, /, IRBasicBlock *a, IRBasicBlock *b) {
    DomFact *b_fact = CALL(DominatorDA, self->dom_da, get_out_fact, /, b);
    return CALL(DomFact, *b_fact, get, /, a);
}
