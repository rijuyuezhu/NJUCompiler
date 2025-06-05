#include "da_constprop.h"
#include "general_container.h"
#include "ir_function.h"

void MTD(CPFact, init, /) { CALL(MapVarToCPValue, self->mapping, init, /); }

void MTD(CPFact, drop, /) { DROPOBJ(MapVarToCPValue, self->mapping); }

CPValue MTD(CPFact, get, /, usize key) {
    MapVarToCPValueIterator it =
        CALL(MapVarToCPValue, self->mapping, find_owned, /, key);
    if (it == NULL) {
        return NSCALL(CPValue, get_undef, /);
    } else {
        return it->value;
    }
}

bool MTD(CPFact, update, /, usize key, CPValue value) {
    MapVarToCPValueIterator it =
        CALL(MapVarToCPValue, self->mapping, find_owned, /, key);
    if (value.kind == CPValueKindUndef) {
        if (it == NULL) {
            return false;
        } else {
            CALL(MapVarToCPValue, self->mapping, erase, /, it);
            return true;
        }
    } else {
        if (it == NULL) {
            MapVarToCPValueInsertResult res =
                CALL(MapVarToCPValue, self->mapping, insert, /, key, value);
            ASSERT(res.inserted);
            return true;
        } else {
            bool same = NSCALL(CPValue, eq, /, it->value, value);
            it->value = value;
            return !same;
        }
    }
}
void MTD(CPFact, debug_print, /) {
    for (MapVarToCPValueIterator it =
             CALL(MapVarToCPValue, self->mapping, begin, /);
         it; it = CALL(MapVarToCPValue, self->mapping, next, /, it)) {
        printf("{v%zu -> ", it->key);
        CPValue v = it->value;
        if (v.kind == CPValueKindUndef) {
            PANIC();
        } else if (v.kind == CPValueKindNAC) {
            printf("NAC");
        } else {
            printf("#%d", v.const_val);
        }
        printf("} ");
    }
}

void MTD(ConstPropDA, init, /) {
    CALL(ConstPropDA, *self, base_init, /);
    CALL(MapBBToCPFact, self->in_facts, init, /);
    CALL(MapBBToCPFact, self->out_facts, init, /);
}
void MTD(ConstPropDA, drop, /) {
    DROPOBJ(MapBBToCPFact, self->out_facts);
    DROPOBJ(MapBBToCPFact, self->in_facts);
}
bool MTD(ConstPropDA, is_forward, /) { return true; }
Any MTD(ConstPropDA, new_boundary_fact, /, IRFunction *func) {
    CPFact *fact = CREOBJHEAP(CPFact, /);
    VecUSize *params = &func->params;
    for (usize i = 0; i < params->size; i++) {
        usize param = params->data[i];
        CPValue nac = NSCALL(CPValue, get_nac, /);
        CALL(CPFact, *fact, update, /, param, nac);
    }
    return fact;
}
Any MTD(ConstPropDA, new_initial_fact, /) { return CREOBJHEAP(CPFact, /); }
void MTD(ConstPropDA, drop_fact, /, Any fact) {
    CPFact *f = fact;
    DROPOBJHEAP(CPFact, f);
}
void MTD(ConstPropDA, set_in_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToCPFact, self->in_facts, insert_or_assign, /, bb, fact);
}
void MTD(ConstPropDA, set_out_fact, /, IRBasicBlock *bb, Any fact) {
    CALL(MapBBToCPFact, self->out_facts, insert_or_assign, /, bb, fact);
}
Any MTD(ConstPropDA, get_in_fact, /, IRBasicBlock *bb) {
    MapBBToCPFactIterator it =
        CALL(MapBBToCPFact, self->in_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}
Any MTD(ConstPropDA, get_out_fact, /, IRBasicBlock *bb) {
    MapBBToCPFactIterator it =
        CALL(MapBBToCPFact, self->out_facts, find_owned, /, bb);
    ASSERT(it);
    return it->value;
}

static CPValue meet_value(CPValue v1, CPValue v2) {
    if (v1.kind == CPValueKindNAC || v2.kind == CPValueKindNAC) {
        return NSCALL(CPValue, get_nac, /);
    } else if (v1.kind == CPValueKindUndef) {
        return v2;
    } else if (v2.kind == CPValueKindUndef) {
        return v1;
    } else {
        ASSERT(v1.kind == CPValueKindConst && v2.kind == CPValueKindConst);
        if (v1.const_val == v2.const_val) {
            return v1;
        } else {
            return NSCALL(CPValue, get_nac, /);
        }
    }
}

bool MTD(ConstPropDA, meet_into, /, Any fact, Any target) {
    CPFact *from = fact;
    CPFact *to = target;
    bool updated = false;
    for (MapVarToCPValueIterator it =
             CALL(MapVarToCPValue, from->mapping, begin, /);
         it; it = CALL(MapVarToCPValue, from->mapping, next, /, it)) {
        usize var = it->key;
        CPValue value = it->value;
        CPValue orig_value = CALL(CPFact, *to, get, /, var);
        CPValue res = meet_value(value, orig_value);
        updated = CALL(CPFact, *to, update, /, var, res) || updated;
    }
    return updated;
}

void MTD(ConstPropDA, transfer_stmt, /, IRStmtBase *stmt, Any fact) {
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    if (def == (usize)-1) {
        return;
    }
    CPFact *cp_fact = fact;
    CPValue eval;
    if (stmt->kind == IRStmtKindAssign) {
        IRValue src = ((IRStmtAssign *)stmt)->src;
        if (src.is_const) {
            eval = NSCALL(CPValue, get_const, /, src.const_val);
        } else {
            eval = CALL(CPFact, *cp_fact, get, /, src.var);
        }
    } else if (stmt->kind == IRStmtKindArith) {
        IRStmtArith *arith = (IRStmtArith *)stmt;
        IRValue src1 = arith->src1;
        IRValue src2 = arith->src2;
        CPValue cpval1;
        CPValue cpval2;
        if (src1.is_const) {
            cpval1 = NSCALL(CPValue, get_const, /, src1.const_val);
        } else {
            cpval1 = CALL(CPFact, *cp_fact, get, /, src1.var);
        }
        if (src2.is_const) {
            cpval2 = NSCALL(CPValue, get_const, /, src2.const_val);
        } else {
            cpval2 = CALL(CPFact, *cp_fact, get, /, src2.var);
        }

        if (cpval1.kind == CPValueKindConst &&
            cpval2.kind == CPValueKindConst) {
            switch (arith->aop) {
            case ArithopAdd:
                eval = NSCALL(CPValue, get_const, /,
                              cpval1.const_val + cpval2.const_val);
                break;
            case ArithopSub:
                eval = NSCALL(CPValue, get_const, /,
                              cpval1.const_val - cpval2.const_val);
                break;
            case ArithopMul:
                eval = NSCALL(CPValue, get_const, /,
                              cpval1.const_val * cpval2.const_val);
                break;
            case ArithopDiv:
                if (cpval2.const_val == 0) {
                    // treat division by zero as 0; we do not use UNDEF
                    // considering the monotonicity of the analysis
                    eval = NSCALL(CPValue, get_const, /, 0);
                } else {
                    eval = NSCALL(CPValue, get_const, /,
                                  cpval1.const_val / cpval2.const_val);
                }
                break;
            default:
                PANIC("Unsupported arith op %d", arith->aop);
            }
        } else if (cpval1.kind == CPValueKindNAC ||
                   cpval2.kind == CPValueKindNAC) {
            eval = NSCALL(CPValue, get_nac, /);
        } else {
            eval = NSCALL(CPValue, get_undef, /);
        }
    } else {
        eval = NSCALL(CPValue, get_nac, /);
    }
    CALL(CPFact, *cp_fact, update, /, def, eval);
}

void MTD(ConstPropDA, debug_print, /, IRFunction *func) {
    const char *func_name = STRING_C_STR(func->func_name);
    printf("[ConstPropDA result for function `%s`]\n", func_name);
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        printf(COLOR_GREEN "==>" COLOR_NORMAL " BB%s [%p]:\n",
               bb == func->entry  ? " (entry)"
               : bb == func->exit ? " (exit)"
                                  : "",
               bb);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " In facts: ");
        CPFact *in_facts = CALL(ConstPropDA, *self, get_in_fact, /, bb);
        CALL(CPFact, *in_facts, debug_print, /);
        printf("\n");
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " IRs:\n");
        CALL(IRBasicBlock, *bb, debug_print, /);
        printf("  " COLOR_BLUE "->" COLOR_NORMAL " Out facts: ");
        CPFact *out_facts = CALL(ConstPropDA, *self, get_out_fact, /, bb);
        CALL(CPFact, *out_facts, debug_print, /);
        printf("\n");
    }
}

static void VMTD(ConstPropDA, const_fold_callback, /, ListDynIRStmtNode *iter,
                 Any fact, void *extra_args) {
    IRStmtBase *stmt = iter->data;
    CPFact *cp_fact = fact;
    bool *updated = extra_args;
    SliceIRValue uses = VCALL(IRStmtBase, *stmt, get_use, /);
    for (usize i = 0; i < uses.size; i++) {
        IRValue use = uses.data[i];
        if (use.is_const) {
            continue;
        }
        usize var = use.var;
        CPValue cp_value = CALL(CPFact, *cp_fact, get, /, var);
        if (cp_value.kind == CPValueKindConst) {
            uses.data[i] = NSCALL(IRValue, from_const, /, cp_value.const_val);
            *updated = true;
        }
    }
}

bool MTD(ConstPropDA, const_fold, /, IRFunction *func) {
    bool updated = false;
    CALL(DataflowAnalysisBase, *TOBASE(self), iter_func, /, func,
         MTDNAME(ConstPropDA, const_fold_callback), &updated);
    return updated;
}

DEFINE_MAPPING(MapVarToCPValue, usize, CPValue, FUNC_EXTERN);
DEFINE_MAPPING(MapBBToCPFact, IRBasicBlock *, CPFact *, FUNC_EXTERN);
