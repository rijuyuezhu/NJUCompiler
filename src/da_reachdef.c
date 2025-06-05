#include "da_reachdef.h"

void MTD(RDFact, init, /) { CALL(MapUSizeToSetPtr, self->reach_defs, init, /); }
void MTD(RDFact, drop, /) { DROPOBJ(MapUSizeToSetPtr, self->reach_defs); }
SetPtr *MTD(RDFact, get, /, usize var) {
    MapUSizeToSetPtrIterator it =
        CALL(MapUSizeToSetPtr, self->reach_defs, find_owned, /, var);
    if (it == NULL) {
        SetPtr new_set = CREOBJ(SetPtr, /);
        MapUSizeToSetPtrInsertResult res =
            CALL(MapUSizeToSetPtr, self->reach_defs, insert, /, var, new_set);
        ASSERT(res.inserted);
        it = res.node;
    }
    return &it->value;
}
void MTD(RDFact, redef, /, usize var, IRStmtBase *bb) {
    SetPtr *set = CALL(RDFact, *self, get, /, var);
    CALL(SetPtr, *set, clear, /);
    CALL(SetPtr, *set, insert, /, bb, ZERO_SIZE);
}

void MTD(RDFact, debug_print, /) {
    for (MapUSizeToSetPtrIterator it =
             CALL(MapUSizeToSetPtr, self->reach_defs, begin, /);
         it; it = CALL(MapUSizeToSetPtr, self->reach_defs, next, /, it)) {
        SetPtr *set = &it->value;
        if (set->size == 0) {
            continue;
        }
        printf("{v%zu -> ", it->key);
        bool is_first = true;
        for (SetPtrIterator set_it = CALL(SetPtr, *set, begin, /); set_it;
             set_it = CALL(SetPtr, *set, next, /, set_it)) {
            IRStmtBase *stmt = set_it->key;
            if (!is_first) {
                printf(" | ");
            } else {
                is_first = false;
            }

            String builder = CREOBJ(String, /);
            VCALL(IRStmtBase, *stmt, build_str, /, &builder);
            usize not_null_pos = (usize)-1;
            bool can_update = true;
            for (usize i = 0; i < builder.size && builder.data[i]; i++) {
                if (builder.data[i] == '\n') {
                    builder.data[i] = '\0';
                    can_update = true;
                } else {
                    if (can_update) {
                        not_null_pos = i;
                        can_update = false;
                    }
                }
            }
            if (not_null_pos != (usize)-1) {
                printf("%s", STRING_C_STR(builder) + not_null_pos);
            }
            DROPOBJ(String, builder);
        }
        printf("} ");
    }
}

void MTD(ReachDefDA, init, /, MapUSizeToDynIRStmt *param_to_stmt) {
    self->param_to_stmt = param_to_stmt;
    CALL(ReachDefDA, *self, base_init, /);
    CALL(MapBBToRDFact, self->in_facts, init, /);
    CALL(MapBBToRDFact, self->out_facts, init, /);
}
void MTD(ReachDefDA, drop, /) {
    DROPOBJ(MapBBToRDFact, self->out_facts);
    DROPOBJ(MapBBToRDFact, self->in_facts);
}
bool MTD(ReachDefDA, is_forward, /) { return true; }
Any MTD(ReachDefDA, new_boundary_fact, /, IRFunction *func) {
    RDFact *fact = CREOBJHEAP(RDFact, /);
    VecUSize *params = &func->params;
    for (usize i = 0; i < params->size; i++) {
        usize param = params->data[i];
        MapUSizeToDynIRStmtIterator it = CALL(
            MapUSizeToDynIRStmt, *self->param_to_stmt, find_owned, /, param);
        ASSERT(it);
        CALL(RDFact, *fact, redef, /, param, it->value);
    }
    return fact;
}
Any MTD(ReachDefDA, new_initial_fact, /) { return CREOBJHEAP(RDFact, /); }
void MTD(ReachDefDA, drop_fact, /, Any fact) {
    RDFact *f = fact;
    DROPOBJHEAP(RDFact, f);
}
void MTD(ReachDefDA, set_in_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToRDFact, self->in_facts, insert_or_assign, /, bb, fact);
}
void MTD(ReachDefDA, set_out_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToRDFact, self->out_facts, insert_or_assign, /, bb, fact);
}
Any MTD(ReachDefDA, get_in_fact, /, IRBasicBlock *bb) {
    MapBBToRDFactIterator it =
        CALL(MapBBToRDFact, self->in_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}
Any MTD(ReachDefDA, get_out_fact, /, IRBasicBlock *bb) {
    MapBBToRDFactIterator it =
        CALL(MapBBToRDFact, self->out_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}

bool MTD(ReachDefDA, meet_into, /, Any fact, Any target) {
    RDFact *from = fact;
    RDFact *to = target;
    // to = from union to
    bool updated = false;
    for (MapUSizeToSetPtrIterator it =
             CALL(MapUSizeToSetPtr, from->reach_defs, begin, /);
         it; it = CALL(MapUSizeToSetPtr, from->reach_defs, next, /, it)) {
        usize key = it->key;
        SetPtr *from_set = &it->value;
        SetPtr *to_set = CALL(RDFact, *to, get, /, key);
        for (SetPtrIterator set_it = CALL(SetPtr, *from_set, begin, /); set_it;
             set_it = CALL(SetPtr, *from_set, next, /, set_it)) {
            IRStmtBase *stmt = set_it->key;
            if (CALL(SetPtr, *to_set, insert, /, stmt, ZERO_SIZE).inserted) {
                updated = true;
            }
        }
    }
    return updated;
}

void MTD(ReachDefDA, transfer_stmt, /, IRStmtBase *stmt, Any fact) {
    RDFact *rd_fact = fact;
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    if (def != (usize)-1) {
        CALL(RDFact, *rd_fact, redef, /, def, stmt);
    }
}

void MTD(ReachDefDA, debug_print, /, IRFunction *func) {
    const char *func_name = STRING_C_STR(func->func_name);
    printf("[ReachDefDA result for function `%s`]\n", func_name);
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        printf(COLOR_GREEN "==>" COLOR_NORMAL " BB%s [%p]:\n",
               bb == func->entry  ? " (entry)"
               : bb == func->exit ? " (exit)"
                                  : "",
               bb);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " In facts: ");
        RDFact *in_facts = CALL(ReachDefDA, *self, get_in_fact, /, bb);
        CALL(RDFact, *in_facts, debug_print, /);
        printf("\n");
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " IRs:\n");
        CALL(IRBasicBlock, *bb, debug_print, /);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " Out facts: ");
        RDFact *out_facts = CALL(ReachDefDA, *self, get_out_fact, /, bb);
        CALL(RDFact, *out_facts, debug_print, /);
        printf("\n");
    }
}

DEFINE_MAPPING(MapBBToRDFact, IRBasicBlock *, RDFact *, FUNC_EXTERN);
