#include "ir_basic_block.h"
#include "ir_function.h"
#include "loop_opt.h"

static void MTD(LoopOpt, fix_preheader_bb, /, ListBoxBBNode *bb_it,
                LoopInfo *loop_info) {
    if (!loop_info->preheader) {
        return;
    }
    IRFunction *func = self->func;
    IRBasicBlock *header = loop_info->header;
    IRBasicBlock *preheader = loop_info->preheader; // owned
    loop_info->preheader = NULL;
    usize header_label = header->label;
    usize preheader_label = preheader->label;
    ASSERT(header_label != (usize)-1);
    ASSERT(preheader_label != (usize)-1);

    ListPtr *pred = CALL(IRFunction, *func, get_pred, /, header);

    for (ListPtrNode *pred_it = pred->head; pred_it; pred_it = pred_it->next) {
        IRBasicBlock *pred_bb = pred_it->data;
        SetPtrIterator find_pred_in_loop_it =
            CALL(SetPtr, loop_info->nodes, find_owned, /, pred_bb);
        if (find_pred_in_loop_it) {
            // in loop
            if (!bb_it->prev || bb_it->prev->data != pred_bb) {
                continue;
            }
            // then pred_bb falls through to the header
            bool shall_add_goto;
            if (pred_bb->stmts.size == 0) {
                shall_add_goto = true;
            } else {
                IRStmtBase *last_stmt = pred_bb->stmts.tail->data;
                if (last_stmt->kind == IRStmtKindGoto) {
                    shall_add_goto = false;
                } else if (last_stmt->kind == IRStmtKindIf) {
                    shall_add_goto = false;
                    IRStmtIf *if_stmt = (IRStmtIf *)last_stmt;
                    if (if_stmt->false_label == (usize)-1) {
                        if_stmt->false_label = header_label;
                    }
                } else {
                    shall_add_goto = true;
                }
            }
            if (shall_add_goto) {
                IRStmtBase *goto_stmt =
                    (IRStmtBase *)CREOBJHEAP(IRStmtGoto, /, preheader_label);
                CALL(IRBasicBlock, *pred_bb, add_stmt, /, goto_stmt);
            }
        } else {
            // out of loop
            if (pred_bb->stmts.size == 0) {
                // fall through to the preheader automatically
                continue;
            }
            IRStmtBase *last_stmt = pred_bb->stmts.tail->data;
            if (last_stmt->kind == IRStmtKindGoto) {
                IRStmtGoto *goto_stmt = (IRStmtGoto *)last_stmt;
                if (goto_stmt->label == header_label) {
                    goto_stmt->label = preheader_label;
                }
            } else if (last_stmt->kind == IRStmtKindIf) {
                IRStmtIf *if_stmt = (IRStmtIf *)last_stmt;
                if (if_stmt->true_label == header_label) {
                    if_stmt->true_label = preheader_label;
                }
                if (if_stmt->false_label == header_label) {
                    if_stmt->false_label = preheader_label;
                }
            }
        }
    }

    // insert preheader before bb_it
    CALL(ListBoxBB, func->basic_blocks, insert_after, /, bb_it->prev,
         preheader);
}

void MTD(LoopOpt, fix_preheader, /) {
    for (ListBoxBBNode *it = self->func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        MapHeaderToLoopInfoIterator li_it =
            CALL(MapHeaderToLoopInfo, self->loop_infos, find_owned, /, bb);
        if (li_it) {
            CALL(LoopOpt, *self, fix_preheader_bb, /, it, &li_it->value);
        }
    }
    CALL(IRFunction, *self->func, reestablish, /);
}
