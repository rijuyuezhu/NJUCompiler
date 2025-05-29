#include "da_dominator.h"
#include "ir_function.h"
#include "utils.h"

void MTD(DomFact, init, /) {
    CALL(SetPtr, self->doms, init, /);
    self->is_universal = false;
}
void MTD(DomFact, drop, /) { DROPOBJ(SetPtr, self->doms); }
bool MTD(DomFact, get, /, IRBasicBlock *key) {
    if (self->is_universal) {
        return true;
    }
    SetPtrIterator it = CALL(SetPtr, self->doms, find_owned, /, key);
    return it != NULL;
}
bool MTD(DomFact, set, /, IRBasicBlock *key, bool value) {
    if (self->is_universal) {
        if (value) {
            return false;
        } else {
            PANIC("Do not set false to universal fact");
        }
    }
    SetPtrIterator it = CALL(SetPtr, self->doms, find_owned, /, key);
    if (value) {
        if (it) {
            return false;
        } else {
            CALL(SetPtr, self->doms, insert, /, key, ZERO_SIZE);
            return true;
        }
    } else {
        if (it) {
            CALL(SetPtr, self->doms, erase, /, it);
            return true;
        } else {
            return false;
        }
    }
}
void MTD(DomFact, debug_print, /) {
    if (self->is_universal) {
        printf("[universal set]");
        return;
    }
    for (SetPtrIterator it = CALL(SetPtr, self->doms, begin, /); it;
         it = CALL(SetPtr, self->doms, next, /, it)) {
        printf("[%p] ", it->key);
    }
}

void MTD(DominatorDA, init, /, MapStmtToBBInfo *stmt_to_bb_info) {
    CALL(DominatorDA, *self, base_init, /);
    self->stmt_to_bb_info = stmt_to_bb_info;
    CALL(MapBBToDomFact, self->in_facts, init, /);
    CALL(MapBBToDomFact, self->out_facts, init, /);
}

void MTD(DominatorDA, drop, /) {
    DROPOBJ(MapBBToDomFact, self->out_facts);
    DROPOBJ(MapBBToDomFact, self->in_facts);
}

bool MTD(DominatorDA, is_forward, /) { return true; }

Any MTD(DominatorDA, new_boundary_fact, /, IRFunction *func) {
    DomFact *fact = CREOBJHEAP(DomFact, /);
    CALL(SetPtr, fact->doms, insert, /, func->entry, ZERO_SIZE);
    return fact;
}
Any MTD(DominatorDA, new_initial_fact, /) {
    DomFact *fact = CREOBJHEAP(DomFact, /);
    fact->is_universal = true;
    return fact;
}
void MTD(DominatorDA, drop_fact, /, Any fact) { DROPOBJHEAP(DomFact, fact); }
void MTD(DominatorDA, set_in_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToDomFact, self->in_facts, insert_or_assign, /, bb, fact);
}
void MTD(DominatorDA, set_out_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToDomFact, self->out_facts, insert_or_assign, /, bb, fact);
}
Any MTD(DominatorDA, get_in_fact, /, IRBasicBlock *bb) {
    MapBBToDomFactIterator it =
        CALL(MapBBToDomFact, self->in_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}
Any MTD(DominatorDA, get_out_fact, /, IRBasicBlock *bb) {
    MapBBToDomFactIterator it =
        CALL(MapBBToDomFact, self->out_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}

bool MTD(DominatorDA, meet_into, /, Any fact, Any target) {
    // target = target sect fact
    DomFact *from = fact;
    DomFact *to = target;
    if (from->is_universal) {
        return false;
    }
    if (to->is_universal) {
        to->is_universal = false;
        // set to = from
        CALL(SetPtr, to->doms, clone_from, /, &from->doms);
        return true;
    } else {
        bool updated = false;
        for (SetPtrIterator it = CALL(SetPtr, to->doms, begin, /),
                            nxt_it = NULL;
             it; it = nxt_it) {
            nxt_it = CALL(SetPtr, to->doms, next, /, it);
            IRBasicBlock *var = it->key;
            bool value = CALL(DomFact, *from, get, /, var);
            if (!value) {
                CALL(SetPtr, to->doms, erase, /, it);
                updated = true;
            }
        }
        return updated;
    }
}

void MTD(DominatorDA, transfer_stmt, /, IRStmtBase *stmt, Any fact) {
    DomFact *dom_fact = fact;
    if (dom_fact->is_universal) {
        // universal fact happens on the initialization;
        // it is not convenient to delete elements from it;
        // if the code is reachable, it will be not universal one day;
        // so we wait until then
        return;
    }
    MapStmtToBBInfoIterator it =
        CALL(MapStmtToBBInfo, *self->stmt_to_bb_info, find_owned, /, stmt);
    ASSERT(it);
    IRBasicBlock *bb = it->value.bb;
    ASSERT(bb->stmts.head);
    if (bb->stmts.head->data == stmt) {
        CALL(SetPtr, dom_fact->doms, insert, /, bb, ZERO_SIZE);
    }
}

void MTD(DominatorDA, debug_print, /, IRFunction *func) {
    const char *func_name = STRING_C_STR(func->func_name);
    printf("[DominatorDA result for function `%s`]\n", func_name);
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        printf(COLOR_GREEN "==>" COLOR_NORMAL " BB%s [%p]:\n",
               bb == func->entry  ? " (entry)"
               : bb == func->exit ? " (exit)"
                                  : "",
               bb);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " In facts: ");
        DomFact *in_facts = CALL(DominatorDA, *self, get_in_fact, /, bb);
        CALL(DomFact, *in_facts, debug_print, /);
        printf("\n");
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " IRs:\n");
        CALL(IRBasicBlock, *bb, debug_print, /);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " Out facts: ");
        DomFact *out_facts = CALL(DominatorDA, *self, get_out_fact, /, bb);
        CALL(DomFact, *out_facts, debug_print, /);
        printf("\n");
    }
}

DEFINE_MAPPING(MapBBToDomFact, IRBasicBlock *, DomFact *, FUNC_EXTERN);
DEFINE_MAPPING(MapStmtToBBInfo, IRStmtBase *, BBInfo, FUNC_EXTERN);
