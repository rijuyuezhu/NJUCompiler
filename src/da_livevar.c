#include "da_livevar.h"
#include "ir_stmt.h"

void MTD(LVFact, init, /) { CALL(SetUSize, self->live_vars, init, /); }
void MTD(LVFact, drop, /) { DROPOBJ(SetUSize, self->live_vars); }
bool MTD(LVFact, get, /, usize key) {
    SetUSizeIterator it = CALL(SetUSize, self->live_vars, find_owned, /, key);
    return it != NULL;
}
bool MTD(LVFact, set, /, usize key, bool value) {
    SetUSizeIterator it = CALL(SetUSize, self->live_vars, find_owned, /, key);
    if (value) {
        if (it) {
            return false;
        } else {
            CALL(SetUSize, self->live_vars, insert, /, key, ZERO_SIZE);
            return true;
        }
    } else {
        if (it) {
            CALL(SetUSize, self->live_vars, erase, /, it);
            return true;
        } else {
            return false;
        }
    }
}

void MTD(LVFact, debug_print, /) {
    for (SetUSizeIterator it = CALL(SetUSize, self->live_vars, begin, /); it;
         it = CALL(SetUSize, self->live_vars, next, /, it)) {
        printf("v%zu ", it->key);
    }
}
void MTD(LiveVarDA, init, /) {
    CALL(LiveVarDA, *self, base_init, /);
    CALL(MapBBToLVFact, self->in_facts, init, /);
    CALL(MapBBToLVFact, self->out_facts, init, /);
}

void MTD(LiveVarDA, drop, /) {
    DROPOBJ(MapBBToLVFact, self->out_facts);
    DROPOBJ(MapBBToLVFact, self->in_facts);
}

bool MTD(LiveVarDA, is_forward, /) { return false; }
Any MTD(LiveVarDA, new_boundary_fact, /, ATTR_UNUSED IRFunction *func) {
    return CREOBJHEAP(LVFact, /);
}
Any MTD(LiveVarDA, new_initial_fact, /) { return CREOBJHEAP(LVFact, /); }
void MTD(LiveVarDA, drop_fact, /, Any fact) { DROPOBJHEAP(LVFact, fact); }
void MTD(LiveVarDA, set_in_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToLVFact, self->in_facts, insert_or_assign, /, bb, fact);
}
void MTD(LiveVarDA, set_out_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToLVFact, self->out_facts, insert_or_assign, /, bb, fact);
}
Any MTD(LiveVarDA, get_in_fact, /, IRBasicBlock *bb) {
    MapBBToLVFactIterator it =
        CALL(MapBBToLVFact, self->in_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}
Any MTD(LiveVarDA, get_out_fact, /, IRBasicBlock *bb) {
    MapBBToLVFactIterator it =
        CALL(MapBBToLVFact, self->out_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}

bool MTD(LiveVarDA, meet_into, /, Any fact, Any target) {
    LVFact *from = fact;
    LVFact *to = target;
    // to = to union from
    bool updated = false;
    for (SetUSizeIterator it = CALL(SetUSize, from->live_vars, begin, /); it;
         it = CALL(SetUSize, from->live_vars, next, /, it)) {
        usize var = it->key;
        updated = CALL(LVFact, *to, set, /, var, true) || updated;
    }
    return updated;
}

void MTD(LiveVarDA, transfer_stmt, /, IRStmtBase *stmt, Any fact) {
    LVFact *lv_fact = fact;
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    if (def != (usize)-1) {
        // kill def
        CALL(LVFact, *lv_fact, set, /, def, false);
    }
    SliceIRValue uses = VCALL(IRStmtBase, *stmt, get_use, /);
    for (usize i = 0; i < uses.size; i++) {
        IRValue use = uses.data[i];
        if (!use.is_const) {
            usize var = use.var;
            // add use
            CALL(LVFact, *lv_fact, set, /, var, true);
        }
    }
}

void MTD(LiveVarDA, debug_print, /, IRFunction *func) {
    const char *func_name = STRING_C_STR(func->func_name);
    printf("[LiveVarDA result for function `%s`]\n", func_name);
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        printf(COLOR_GREEN "==>" COLOR_NORMAL " BB%s [%p]:\n",
               bb == func->entry  ? " (entry)"
               : bb == func->exit ? " (exit)"
                                  : "",
               bb);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " In facts: ");
        LVFact *in_facts = CALL(LiveVarDA, *self, get_in_fact, /, bb);
        CALL(LVFact, *in_facts, debug_print, /);
        printf("\n");
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " IRs:\n");
        CALL(IRBasicBlock, *bb, debug_print, /);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " Out facts: ");
        LVFact *out_facts = CALL(LiveVarDA, *self, get_out_fact, /, bb);
        CALL(LVFact, *out_facts, debug_print, /);
        printf("\n");
    }
}

static void VMTD(LiveVarDA, dead_code_eliminate_bb_callback, /,
                 ListDynIRStmtNode *iter, Any fact,
                 ATTR_UNUSED void *extra_args) {
    IRStmtBase *stmt = iter->data;
    LVFact *lv_fact = fact;
    if (stmt->kind == IRStmtKindAssign || stmt->kind == IRStmtKindLoad ||
        stmt->kind == IRStmtKindArith) {
        usize def = VCALL(IRStmtBase, *stmt, get_def, /);
        if (def == (usize)-1) {
            return;
        }
        if (!CALL(LVFact, *lv_fact, get, /, def)) {
            stmt->is_dead = true;
        }
    }
}

void MTD(LiveVarDA, dead_code_eliminate_bb, /, IRBasicBlock *bb) {
    CALL(DataflowAnalysisBase, *TOBASE(self), iter_bb, /, bb,
         MTDNAME(LiveVarDA, dead_code_eliminate_bb_callback), NULL);
}

void MTD(LiveVarDA, dead_code_eliminate_func_meta, /, IRFunction *func) {
    LVFact *lv_fact = CALL(LiveVarDA, *self, get_in_fact, /, func->entry);

    for (MapVarToDecInfoIterator
             it = CALL(MapVarToDecInfo, func->var_to_dec_info, begin, /),
             nxt_it = NULL;
         it; it = nxt_it) {
        nxt_it = CALL(MapVarToDecInfo, func->var_to_dec_info, next, /, it);
        usize addr_var = it->value.addr;
        if (!CALL(LVFact, *lv_fact, get, /, addr_var)) {
            // remove var from function
            CALL(MapVarToDecInfo, func->var_to_dec_info, erase, /, it);
        }
    }
}

DEFINE_MAPPING(MapBBToLVFact, IRBasicBlock *, LVFact *, FUNC_EXTERN);
