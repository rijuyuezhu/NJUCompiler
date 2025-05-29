#include "da_dominator.h"
#include "loop_opt.h"

bool MTD(LoopOpt, is_dom_bb, /, IRBasicBlock *a, IRBasicBlock *b) {
    DomFact *b_fact = CALL(DominatorDA, self->dom_da, get_out_fact, /, b);
    return CALL(DomFact, *b_fact, get, /, a);
}
IRBasicBlock *MTD(LoopOpt, get_bb, /, IRStmtBase *stmt) {
    MapStmtToBBInfoIterator it =
        CALL(MapStmtToBBInfo, self->stmt_to_bb_info, find_owned, /, stmt);
    ASSERT(it);
    return it->value.bb;
}

bool MTD(LoopOpt, is_dom_stmt, /, IRStmtBase *a, IRStmtBase *b) {
    MapStmtToBBInfoIterator a_info_it =
        CALL(MapStmtToBBInfo, self->stmt_to_bb_info, find_owned, /, a);
    ASSERT(a_info_it);
    BBInfo a_info = a_info_it->value;
    MapStmtToBBInfoIterator b_info_it =
        CALL(MapStmtToBBInfo, self->stmt_to_bb_info, find_owned, /, b);
    ASSERT(b_info_it);
    BBInfo b_info = b_info_it->value;
    if (a_info.bb == b_info.bb) {
        return a_info.idx <= b_info.idx;
    } else {
        return CALL(LoopOpt, *self, is_dom_bb, /, a_info.bb, b_info.bb);
    }
}
