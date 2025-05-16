#include "da_copyprop.h"
#include "general_container.h"

void MTD(CYPFact, init, /) {
    self->is_universal = false;
    CALL(MapUSizeUSize, self->dst_to_src, init, /);
    CALL(MapUSizeToSetUSize, self->src_to_dst, init, /);
}
void MTD(CYPFact, drop, /) {
    DROPOBJ(MapUSizeToSetUSize, self->src_to_dst);
    DROPOBJ(MapUSizeUSize, self->dst_to_src);
}
bool MTD(CYPFact, kill_dst, /, usize dst) {
    if (self->is_universal) {
        PANIC("Do not call kill_dst on a universe fact");
    }
    MapUSizeUSizeIterator it =
        CALL(MapUSizeUSize, self->dst_to_src, find_owned, /, dst);
    if (it) {
        usize src = it->value;
        CALL(MapUSizeUSize, self->dst_to_src, erase, /, it);
        MapUSizeToSetUSizeIterator it2 =
            CALL(MapUSizeToSetUSize, self->src_to_dst, find_owned, /, src);
        ASSERT(it2);
        SetUSizeIterator it3 = CALL(SetUSize, it2->value, find_owned, /, dst);
        ASSERT(it3);
        CALL(SetUSize, it2->value, erase, /, it3);
        return true;
    } else {
        return false;
    }
}
bool MTD(CYPFact, kill_src, /, usize src) {
    if (self->is_universal) {
        PANIC("Do not call kill_src on a universe fact");
    }
    MapUSizeToSetUSizeIterator it =
        CALL(MapUSizeToSetUSize, self->src_to_dst, find_owned, /, src);
    if (it) {
        SetUSize *dsts = &it->value;
        for (SetUSizeIterator it2 = CALL(SetUSize, *dsts, begin, /); it2;
             it2 = CALL(SetUSize, *dsts, next, /, it2)) {
            usize dst = it2->key;
            MapUSizeUSizeIterator it3 =
                CALL(MapUSizeUSize, self->dst_to_src, find_owned, /, dst);
            ASSERT(it3);
            CALL(MapUSizeUSize, self->dst_to_src, erase, /, it3);
        }
        bool updated = (dsts->size != 0);
        CALL(MapUSizeToSetUSize, self->src_to_dst, erase, /, it);
        return updated;
    } else {
        return false;
    }
}

void MTD(CYPFact, add_assign, /, usize dst, usize src) {
    if (self->is_universal) {
        PANIC("Do not call add_assign on a universe fact");
    }
    MapUSizeUSizeInsertResult res =
        CALL(MapUSizeUSize, self->dst_to_src, insert, /, dst, src);
    ASSERT(res.inserted);
    MapUSizeToSetUSizeIterator it2 =
        CALL(MapUSizeToSetUSize, self->src_to_dst, find_owned, /, src);
    if (!it2) {
        SetUSize empty = CREOBJ(SetUSize, /);
        MapUSizeToSetUSizeInsertResult res2 =
            CALL(MapUSizeToSetUSize, self->src_to_dst, insert, /, src, empty);
        ASSERT(res2.inserted);
        it2 = res2.node;
    }
    SetUSize *dsts = &it2->value;
    SetUSizeInsertResult res3 =
        CALL(SetUSize, *dsts, insert, /, dst, ZERO_SIZE);
    ASSERT(res3.inserted);
}

usize MTD(CYPFact, get_src, /, usize dst) {
    if (self->is_universal) {
        PANIC("Do not call get_src on a universe fact");
    }
    MapUSizeUSizeIterator it =
        CALL(MapUSizeUSize, self->dst_to_src, find_owned, /, dst);
    if (it) {
        return it->value;
    } else {
        return (usize)-1;
    }
}
void MTD(CYPFact, debug_print, /) {
    if (self->is_universal) {
        printf("[universal map]");
        return;
    }
    for (MapUSizeUSizeIterator it =
             CALL(MapUSizeUSize, self->dst_to_src, begin, /);
         it; it = CALL(MapUSizeUSize, self->dst_to_src, next, /, it)) {
        usize dst = it->key;
        usize src = it->value;
        printf("{v%zu := v%zu} ", dst, src);
    }
}

void MTD(CopyPropDA, init, /) {
    CALL(CopyPropDA, *self, base_init, /);
    CALL(MapBBToCYPFact, self->in_facts, init, /);
    CALL(MapBBToCYPFact, self->out_facts, init, /);
}

void MTD(CopyPropDA, drop, /) {
    DROPOBJ(MapBBToCYPFact, self->out_facts);
    DROPOBJ(MapBBToCYPFact, self->in_facts);
}

bool MTD(CopyPropDA, is_forward, /) { return true; }

Any MTD(CopyPropDA, new_boundary_fact, /, ATTR_UNUSED IRFunction *func) {
    return CREOBJHEAP(CYPFact, /);
}
Any MTD(CopyPropDA, new_initial_fact, /) {
    CYPFact *fact = CREOBJHEAP(CYPFact, /);
    fact->is_universal = true;
    return fact;
}
void MTD(CopyPropDA, drop_fact, /, Any fact) { DROPOBJHEAP(CYPFact, fact); }
void MTD(CopyPropDA, set_in_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToCYPFact, self->in_facts, insert_or_assign, /, bb, fact);
}
void MTD(CopyPropDA, set_out_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToCYPFact, self->out_facts, insert_or_assign, /, bb, fact);
}
Any MTD(CopyPropDA, get_in_fact, /, IRBasicBlock *bb) {
    MapBBToCYPFactIterator it =
        CALL(MapBBToCYPFact, self->in_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}
Any MTD(CopyPropDA, get_out_fact, /, IRBasicBlock *bb) {
    MapBBToCYPFactIterator it =
        CALL(MapBBToCYPFact, self->out_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}

bool MTD(CopyPropDA, meet_into, /, Any fact, Any target) {
    CYPFact *from = fact;
    CYPFact *to = target;
    // to = to sect from
    if (from->is_universal) {
        return false;
    }
    if (to->is_universal) {
        to->is_universal = false;
        // set to = from
        CALL(MapUSizeUSize, to->dst_to_src, clone_from, /, &from->dst_to_src);
        CALL(MapUSizeToSetUSize, to->src_to_dst, clone_from, /,
             &from->src_to_dst);
        return true;
    } else {
        bool updated = false;
        for (MapUSizeUSizeIterator
                 it = CALL(MapUSizeUSize, to->dst_to_src, begin, /),
                 nxt_it = NULL;
             it; it = nxt_it) {
            nxt_it = CALL(MapUSizeUSize, to->dst_to_src, next, /, it);
            usize dst = it->key;
            usize src = it->value;
            // check whether or not dst := src is in from; if not, kill it
            usize src_in_from = CALL(CYPFact, *from, get_src, /, dst);
            if (src_in_from == (usize)-1 || src_in_from != src) {
                bool res = CALL(CYPFact, *to, kill_dst, /, dst);
                ASSERT(res);
                updated = true;
            }
        }
        return updated;
    }
}

void MTD(CopyPropDA, transfer_stmt, /, IRStmtBase *stmt, Any fact) {
    CYPFact *cyp_fact = fact;
    if (cyp_fact->is_universal) {
        // universal fact happens on the initialization;
        // it is not convenient to delete elements from it;
        // it the code is reachable, it will be not universal oneday;
        // so we wait until then
        return;
    }
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    if (def == (usize)-1) {
        goto FINISH_KILL;
    }
    CALL(CYPFact, *cyp_fact, kill_dst, /, def);
    CALL(CYPFact, *cyp_fact, kill_src, /, def);

FINISH_KILL:;
    if (stmt->kind == IRStmtKindAssign) {
        IRStmtAssign *assign = (IRStmtAssign *)stmt;
        if (!assign->src.is_const) {
            usize dst = assign->dst;
            usize src = assign->src.var;
            if (dst != src) {
                // here dst must have been killed above
                CALL(CYPFact, *cyp_fact, add_assign, /, dst, src);
            }
        }
    }
}

void MTD(CopyPropDA, debug_print, /, IRFunction *func) {
    const char *func_name = STRING_C_STR(func->func_name);
    printf("[CopyPropDA result for function `%s`]\n", func_name);
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        printf(COLOR_GREEN "==>" COLOR_NORMAL " BB%s [%p]:\n",
               bb == func->entry  ? " (entry)"
               : bb == func->exit ? " (exit)"
                                  : "",
               bb);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " In facts: ");
        CYPFact *in_facts = CALL(CopyPropDA, *self, get_in_fact, /, bb);
        CALL(CYPFact, *in_facts, debug_print, /);
        printf("\n");
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " IRs:\n");
        CALL(IRBasicBlock, *bb, debug_print, /);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " Out facts: ");
        CYPFact *out_facts = CALL(CopyPropDA, *self, get_out_fact, /, bb);
        CALL(CYPFact, *out_facts, debug_print, /);
        printf("\n");
    }
}

static void VMTD(CopyPropDA, copy_propagate_callback, /,
                 ListDynIRStmtNode *iter, Any fact, void *extra_args) {
    IRStmtBase *stmt = iter->data;
    CYPFact *cyp_fact = fact;
    bool *updated = extra_args;
    SliceIRValue uses = VCALL(IRStmtBase, *stmt, get_use, /);
    for (usize i = 0; i < uses.size; i++) {
        IRValue *use = &uses.data[i];
        if (use->is_const) {
            continue;
        }
        usize updated_var = use->var;
        while (true) {
            usize src = CALL(CYPFact, *cyp_fact, get_src, /, updated_var);
            if (src == (usize)-1) {
                break;
            }
            updated_var = src;
        }
        if (use->var != updated_var) {
            use->var = updated_var;
            *updated = true;
        }
    }
}

bool MTD(CopyPropDA, copy_propagate, /, IRFunction *func) {
    bool updated = false;
    CALL(DataflowAnalysisBase, *TOBASE(self), iter_func, /, func,
         MTDNAME(CopyPropDA, copy_propagate_callback), &updated);
    return updated;
}

DEFINE_MAPPING(MapBBToCYPFact, IRBasicBlock *, CYPFact *, FUNC_EXTERN);
