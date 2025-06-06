#include "ir_function.h"
#include "ir_optimizer.h"

static void strip_gotos(IRFunction *func) {
    IRBasicBlock *last_logical_bb = NULL;
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (bb->label != (usize)-1 && last_logical_bb) {
            NSCALL(IRFunction, try_strip_gotos, /, last_logical_bb, bb->label);
        }
        if (bb->stmts.size != 0) {
            last_logical_bb = bb;
        }
    }
}

static usize get_relabel(IRFunction *func, usize label) {
    if (label == (usize)-1) {
        return (usize)-1;
    }
    IRBasicBlock *bb = CALL(IRFunction, *func, label_to_bb, /, label);
    if (!bb) {
        return label;
    }
    return bb->tag;
}

static void relabel_func(IRFunction *func) {

    // give relabel to all BBs (inherit from the next BB, backward)
    for (ListBoxBBNode *it = func->basic_blocks.tail; it; it = it->prev) {
        IRBasicBlock *bb = it->data;
        bb->tag = bb->label;
        if (bb->stmts.size != 0) {
            continue;
        }
        ListBoxBBNode *nxt = it->next;
        if (nxt) {
            IRBasicBlock *nxt_bb = nxt->data;
            if (nxt_bb->tag != (usize)-1) {
                bb->tag = nxt_bb->tag;
            }
        }
    }

    // forward the labels of empty BBs
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (bb->stmts.size != 0 || bb->tag == (usize)-1 ||
            bb->label != bb->tag) {
            continue;
        }
        ListBoxBBNode *nxt = it->next;
        if (nxt) {
            IRBasicBlock *nxt_bb = nxt->data;
            ASSERT(nxt_bb->tag == (usize)-1);
            nxt_bb->tag = bb->tag;
            nxt_bb->label = bb->label;
            bb->label = (usize)-1;
        }
    }

    // apply relabel
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (!bb->stmts.tail) {
            continue;
        }
        IRStmtBase *last_stmt = bb->stmts.tail->data;
        if (last_stmt->kind == IRStmtKindGoto) {
            IRStmtGoto *goto_stmt = (IRStmtGoto *)last_stmt;
            goto_stmt->label = get_relabel(func, goto_stmt->label);
        } else if (last_stmt->kind == IRStmtKindIf) {
            IRStmtIf *if_stmt = (IRStmtIf *)last_stmt;
            if_stmt->true_label = get_relabel(func, if_stmt->true_label);
            if_stmt->false_label = get_relabel(func, if_stmt->false_label);
        }
    }
}

static void strip_unused_label(IRFunction *func) {
    SetUSize used_labels = CREOBJ(SetUSize, /);
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (!bb->stmts.tail) {
            continue;
        }
        IRStmtBase *last_stmt = bb->stmts.tail->data;
        if (last_stmt->kind == IRStmtKindGoto) {
            IRStmtGoto *goto_stmt = (IRStmtGoto *)last_stmt;
            if (goto_stmt->label != (usize)-1) {
                CALL(SetUSize, used_labels, insert, /, goto_stmt->label,
                     ZERO_SIZE);
            }

        } else if (last_stmt->kind == IRStmtKindIf) {
            IRStmtIf *if_stmt = (IRStmtIf *)last_stmt;
            if (if_stmt->true_label != (usize)-1) {
                CALL(SetUSize, used_labels, insert, /, if_stmt->true_label,
                     ZERO_SIZE);
            }
            if (if_stmt->false_label != (usize)-1) {
                CALL(SetUSize, used_labels, insert, /, if_stmt->false_label,
                     ZERO_SIZE);
            }
        }
    }

    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;

        // if the label is not used, set it to -1
        if (bb->label != (usize)-1 &&
            !CALL(SetUSize, used_labels, find_owned, /, bb->label)) {
            bb->label = (usize)-1;
        }

        // delete empty BBs that are not entry or exit
        if (bb->stmts.size == 0 && bb != func->entry && bb != func->exit) {
            ASSERT(bb->label == (usize)-1);
            bb->is_dead = true;
        }
    }
    DROPOBJ(SetUSize, used_labels);
}

bool MTD(IROptimizer, optimize_func_useless_label_strip, /, IRFunction *func) {
    strip_gotos(func);
    relabel_func(func);
    strip_unused_label(func);
    CALL(IRFunction, *func, remove_dead_bb, /);
    CALL(IRFunction, *func, reestablish, /);
    return true;
}
