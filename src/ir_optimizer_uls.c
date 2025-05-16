#include "ir_basic_block.h"
#include "ir_function.h"
#include "ir_optimizer.h"
#include "ir_stmt.h"

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
    // give relabel to all BBs
    for (ListBoxBBNode *it = func->basic_blocks.tail; it; it = it->prev) {
        IRBasicBlock *bb = it->data;
        bb->tag = bb->label;
        if (bb->stmts.size != 0) {
            continue;
        }
        ListBoxBBNode *nxt = it->next;
        if (nxt) {
            IRBasicBlock *nxt_bb = nxt->data;
            bb->tag = nxt_bb->tag;
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
        if (bb->label == (usize)-1) {
            continue;
        }
        if (CALL(SetUSize, used_labels, find_owned, /, bb->label) == NULL) {
            MapLabelBBIterator it2 = CALL(MapLabelBB, func->label_to_block,
                                          find_owned, /, bb->label);
            ASSERT(it2);
            CALL(MapLabelBB, func->label_to_block, erase, /, it2);
            bb->label = (usize)-1;
        }
    }
    DROPOBJ(SetUSize, used_labels);
}

static void strip_gotos(IRFunction *func) {
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (bb->label == (usize)-1 || !it->prev) {
            continue;
        }
        IRBasicBlock *prev_bb = it->prev->data;
        NSCALL(IRFunction, try_strip_gotos, /, prev_bb, bb->label);
    }
}

bool MTD(IROptimizer, optimize_func_useless_label_strip, /, IRFunction *func) {
    relabel_func(func);
    strip_unused_label(func);
    strip_gotos(func);
    CALL(IRFunction, *func, reestablish, /);
    return true;
}
