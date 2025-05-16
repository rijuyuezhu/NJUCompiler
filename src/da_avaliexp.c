#include "da_avaliexp.h"
#include "general_container.h"

int NSMTD(AEExp, compare, /, const AEExp *a, const AEExp *b) {
    if (a->op != b->op) {
        return NORMALCMP(a->op, b->op);
    }
    int cmpleft = NSCALL(IRValue, compare, /, &a->left, &b->left);
    if (cmpleft != 0) {
        return cmpleft;
    }
    return NSCALL(IRValue, compare, /, &a->right, &b->right);
}

void MTD(AEFact, init, /) {
    CALL(SetUSize, self->avaliset, init, /);
    self->is_univesal = false;
}
void MTD(AEFact, drop, /) { DROPOBJ(SetUSize, self->avaliset); }
bool MTD(AEFact, get, /, usize key) {
    if (self->is_univesal) {
        return true;
    }
    SetUSizeIterator it = CALL(SetUSize, self->avaliset, find_owned, /, key);
    return it != NULL;
}
bool MTD(AEFact, set, /, usize key, bool value) {
    if (self->is_univesal) {
        if (value) {
            return false;
        } else {
            PANIC("Do not set false to universal fact");
        }
    }
    SetUSizeIterator it = CALL(SetUSize, self->avaliset, find_owned, /, key);
    if (value) {
        if (it) {
            return false;
        } else {
            CALL(SetUSize, self->avaliset, insert, /, key, ZERO_SIZE);
            return true;
        }
    } else {
        if (it) {
            CALL(SetUSize, self->avaliset, erase, /, it);
            return true;
        } else {
            return false;
        }
    }
}
void MTD(AEFact, debug_print, /) {
    if (self->is_univesal) {
        printf("[universal set]");
        return;
    }
    for (SetUSizeIterator it = CALL(SetUSize, self->avaliset, begin, /); it;
         it = CALL(SetUSize, self->avaliset, next, /, it)) {
        printf("v%zu ", it->key);
    }
}
void MTD(AvaliExpDA, init, /) {
    CALL(AvaliExpDA, *self, base_init, /);
    CALL(MapAEExpToVar, self->exp_to_var, init, /);
    CALL(MapBBToAEFact, self->in_facts, init, /);
    CALL(MapBBToAEFact, self->out_facts, init, /);
    CALL(MapUSizeToVecUSize, self->var_to_kills, init, /);
}

void MTD(AvaliExpDA, drop, /) {
    DROPOBJ(MapUSizeToVecUSize, self->var_to_kills);
    DROPOBJ(MapBBToAEFact, self->out_facts);
    DROPOBJ(MapBBToAEFact, self->in_facts);
    DROPOBJ(MapAEExpToVar, self->exp_to_var);
}

bool MTD(AvaliExpDA, is_forward, /) { return true; }

Any MTD(AvaliExpDA, new_boundary_fact, /, ATTR_UNUSED IRFunction *func) {
    return CREOBJHEAP(AEFact, /);
}
Any MTD(AvaliExpDA, new_initial_fact, /) {
    AEFact *fact = CREOBJHEAP(AEFact, /);
    fact->is_univesal = true;
    return fact;
}
void MTD(AvaliExpDA, drop_fact, /, Any fact) { DROPOBJHEAP(AEFact, fact); }
void MTD(AvaliExpDA, set_in_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToAEFact, self->in_facts, insert_or_assign, /, bb, fact);
}
void MTD(AvaliExpDA, set_out_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToAEFact, self->out_facts, insert_or_assign, /, bb, fact);
}
Any MTD(AvaliExpDA, get_in_fact, /, IRBasicBlock *bb) {
    MapBBToAEFactIterator it =
        CALL(MapBBToAEFact, self->in_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}
Any MTD(AvaliExpDA, get_out_fact, /, IRBasicBlock *bb) {
    MapBBToAEFactIterator it =
        CALL(MapBBToAEFact, self->out_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}

bool MTD(AvaliExpDA, meet_into, /, Any fact, Any target) {
    // target = target sect fact
    AEFact *from = fact;
    AEFact *to = target;
    if (from->is_univesal) {
        return false;
    }
    if (to->is_univesal) {
        to->is_univesal = false;
        // set to = from
        ASSERT(to->avaliset.size == 0);
        for (SetUSizeIterator it = CALL(SetUSize, from->avaliset, begin, /); it;
             it = CALL(SetUSize, from->avaliset, next, /, it)) {
            usize var = it->key;
            CALL(AEFact, *to, set, /, var, true);
        }
        return true;
    } else {
        bool updated = false;
        for (SetUSizeIterator it = CALL(SetUSize, to->avaliset, begin, /),
                              nxt_it = NULL;
             it; it = nxt_it) {
            nxt_it = CALL(SetUSize, to->avaliset, next, /, it);
            usize var = it->key;
            bool value = CALL(AEFact, *from, get, /, var);
            if (!value) {
                CALL(SetUSize, to->avaliset, erase, /, it);
                updated = true;
            }
        }
        return updated;
    }
}

void MTD(AvaliExpDA, transfer_stmt, /, IRStmtBase *stmt, Any fact) {
    AEFact *ae_fact = fact;
    if (ae_fact->is_univesal) {
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
    MapUSizeToVecUSizeIterator it =
        CALL(MapUSizeToVecUSize, self->var_to_kills, find_owned, /, def);
    if (it == NULL) {
        goto FINISH_KILL;
    }
    VecUSize *kills = &it->value;
    for (usize i = 0; i < kills->size; i++) {
        usize kill = kills->data[i];
        CALL(AEFact, *ae_fact, set, /, kill, false);
    }

FINISH_KILL:;
    if (stmt->kind == IRStmtKindArith) {
        IRStmtArith *arith = (IRStmtArith *)stmt;
        CALL(AEFact, *ae_fact, set, /, arith->dst, true);
    }
}

void MTD(AvaliExpDA, debug_print, /, IRFunction *func) {
    const char *func_name = STRING_C_STR(func->func_name);
    printf("[AvaliExpDA result for function `%s`]\n", func_name);
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        printf(COLOR_GREEN "==>" COLOR_NORMAL " BB%s [%p]:\n",
               bb == func->entry  ? " (entry)"
               : bb == func->exit ? " (exit)"
                                  : "",
               bb);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " In facts: ");
        AEFact *in_facts = CALL(AvaliExpDA, *self, get_in_fact, /, bb);
        CALL(AEFact, *in_facts, debug_print, /);
        printf("\n");
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " IRs:\n");
        CALL(IRBasicBlock, *bb, debug_print, /);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " Out facts: ");
        AEFact *out_facts = CALL(AvaliExpDA, *self, get_out_fact, /, bb);
        CALL(AEFact, *out_facts, debug_print, /);
        printf("\n");
    }
}

static void maybe_add_kill(AvaliExpDA *analysis, usize exp_var, IRValue value) {
    if (value.is_const) {
        return;
    }
    usize var = value.var;
    MapUSizeToVecUSizeIterator it =
        CALL(MapUSizeToVecUSize, analysis->var_to_kills, find_owned, /, var);
    if (it == NULL) {
        VecUSize kills = CREOBJ(VecUSize, /);
        MapUSizeToVecUSizeInsertResult res = CALL(
            MapUSizeToVecUSize, analysis->var_to_kills, insert, /, var, kills);
        it = res.node;
    }
    VecUSize *kills = &it->value;
    CALL(VecUSize, *kills, push_back, /, exp_var);
}

static bool MTD(IRFunction, avaliexp_prepare_callback, /, IRBasicBlock *bb,
                ListDynIRStmtNode *stmt_it, void *extra_args) {
    IRStmtBase *stmt = stmt_it->data;
    AvaliExpDA *analysis = extra_args;
    if (stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith = (IRStmtArith *)stmt;
    AEExp exp = {
        .op = arith->aop,
        .left = arith->src1,
        .right = arith->src2,
    };

    // normalize commutative operations
    if (arith->aop == ArithopAdd || arith->aop == ArithopMul) {
        int cmp = NSCALL(IRValue, compare, /, &exp.left, &exp.right);
        if (cmp > 0) {
            IRValue tmp = exp.left;
            exp.left = exp.right;
            exp.right = tmp;
        }
    }

    MapAEExpToVarIterator it =
        CALL(MapAEExpToVar, analysis->exp_to_var, find, /, &exp);
    usize exp_var;
    if (it) {
        exp_var = it->value;
    } else {
        exp_var =
            CALL(IdxAllocator, self->program->var_idx_allocator, allocate, /);
        CALL(MapAEExpToVar, analysis->exp_to_var, insert, /, exp, exp_var);
        maybe_add_kill(analysis, exp_var, arith->src1);
        maybe_add_kill(analysis, exp_var, arith->src2);
    }
    IRValue exp_var_value = NSCALL(IRValue, from_var, /, exp_var);
    IRStmtAssign *assign =
        CREOBJHEAP(IRStmtAssign, /, arith->dst, exp_var_value);
    CALL(ListDynIRStmt, bb->stmts, insert_after, /, stmt_it,
         (IRStmtBase *)assign);
    arith->dst = exp_var;
    return true;
}

void MTD(AvaliExpDA, prepare, /, IRFunction *func) {
    CALL(IRFunction, *func, iter_stmt, /,
         MTDNAME(IRFunction, avaliexp_prepare_callback), self);
}
void VMTD(AvaliExpDA, clean_redundant_exp_callback, /, ListDynIRStmtNode *iter,
          Any fact) {
    IRStmtBase *stmt = iter->data;
    AEFact *aefact = fact;
    if (stmt->kind != IRStmtKindArith) {
        return;
    }
    IRStmtArith *arith = (IRStmtArith *)stmt;
    usize exp_var = arith->dst;
    bool is_avali = CALL(AEFact, *aefact, get, /, exp_var);
    if (is_avali) {
        stmt->is_dead = true;
    }
}

void MTD(AvaliExpDA, clean_redundant_exp, /, IRFunction *func) {
    CALL(DataflowAnalysisBase, *TOBASE(self), iter_func, /, func,
         MTDNAME(AvaliExpDA, clean_redundant_exp_callback));
}

DEFINE_MAPPING(MapAEExpToVar, AEExp, usize, FUNC_EXTERN);
DEFINE_MAPPING(MapBBToAEFact, IRBasicBlock *, AEFact *, FUNC_EXTERN);
