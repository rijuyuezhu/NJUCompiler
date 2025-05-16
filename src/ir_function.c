#include "ir_function.h"
#include "ir_basic_block.h"
#include "ir_program.h"
#include "ir_stmt.h"
#include "task_engine.h"
#include "utils.h"

void MTD(IRFunction, init, /, String func_name, IRProgram *program) {
    self->func_name = func_name;
    CALL(VecUSize, self->params, init, /);
    CALL(MapVarToDecInfo, self->var_to_dec_info, init, /);
    CALL(ListBoxBB, self->basic_blocks, init, /);
    self->entry = NULL;
    self->exit = NULL;
    CALL(MapLabelBB, self->label_to_block, init, /);
    CALL(MapBBToListBB, self->block_pred, init, /);
    CALL(MapBBToListBB, self->block_succ, init, /);
    self->program = program;

    // add the first block
    IRBasicBlock *first_bb = CREOBJHEAP(IRBasicBlock, /, (usize)-1);
    CALL(ListBoxBB, self->basic_blocks, push_back, /, first_bb);
}

void MTD(IRFunction, drop, /) {
    DROPOBJ(MapBBToListBB, self->block_succ);
    DROPOBJ(MapBBToListBB, self->block_pred);
    DROPOBJ(MapLabelBB, self->label_to_block);
    DROPOBJ(ListBoxBB, self->basic_blocks);
    DROPOBJ(MapVarToDecInfo, self->var_to_dec_info);
    DROPOBJ(VecUSize, self->params);
    DROPOBJ(String, self->func_name);
}

void MTD(IRFunction, add_stmt, /, IRStmtBase *stmt) {
    ASSERT(self->basic_blocks.tail);
    IRBasicBlock *last_bb = self->basic_blocks.tail->data;
    if (last_bb->stmts.size != 0) {
        IRStmtBase *last_stmt = last_bb->stmts.tail->data;
        if (last_stmt->kind == IRStmtKindGoto ||
            last_stmt->kind == IRStmtKindIf ||
            last_stmt->kind == IRStmtKindReturn) {
            // the last statement is jump.

            if (stmt->kind == IRStmtKindGoto &&
                last_stmt->kind == IRStmtKindIf) {
                // try to merge the two statements
                IRStmtGoto *goto_stmt = (IRStmtGoto *)stmt;
                IRStmtIf *last_if_stmt = (IRStmtIf *)last_stmt;
                if (last_if_stmt->false_label == (usize)-1) {
                    last_if_stmt->false_label = goto_stmt->label;
                    VDROPOBJHEAP(IRStmtBase, stmt);
                    return;
                }
            }
            // create a new block
            last_bb = CREOBJHEAP(IRBasicBlock, /, (usize)-1);
            CALL(ListBoxBB, self->basic_blocks, push_back, /, last_bb);
        }
    }
    // add the statement to the last block
    CALL(ListDynIRStmt, last_bb->stmts, push_back, /, stmt);
}

void NSMTD(IRFunction, try_strip_gotos, /, IRBasicBlock *bb, usize next_label) {
    while (bb->stmts.tail) {
        IRStmtBase *stmt = bb->stmts.tail->data;
        if (stmt->kind == IRStmtKindGoto) {
            IRStmtGoto *goto_stmt = (IRStmtGoto *)stmt;
            if (goto_stmt->label == next_label) {
                // remove the goto statement
                CALL(ListDynIRStmt, bb->stmts, remove_back, /);
            } else {
                break;
            }
        } else if (stmt->kind == IRStmtKindIf) {
            IRStmtIf *if_stmt = (IRStmtIf *)stmt;
            if (if_stmt->true_label == next_label) {
                if (if_stmt->false_label == (usize)-1 ||
                    if_stmt->false_label == next_label) {
                    // remove the entire if statement
                    CALL(ListDynIRStmt, bb->stmts, remove_back, /);
                } else {
                    CALL(IRStmtIf, *if_stmt, flip, /);
                    ASSERT(if_stmt->false_label == next_label);
                    if_stmt->false_label = (usize)-1;
                }
            } else if (if_stmt->false_label == next_label) {
                if_stmt->false_label = (usize)-1;
            }
            break;
        } else {
            break;
        }
    }
}

void MTD(IRFunction, add_label, /, usize label) {
    ASSERT(self->basic_blocks.tail);
    IRBasicBlock *last_bb = self->basic_blocks.tail->data;
    NSCALL(IRFunction, try_strip_gotos, /, last_bb, label);
    IRBasicBlock *new_bb = CREOBJHEAP(IRBasicBlock, /, label);
    CALL(ListBoxBB, self->basic_blocks, push_back, /, new_bb);
}

static void MTD(IRFunction, add_edge, /, IRBasicBlock *fr, IRBasicBlock *to) {
    ListPtr *fr_succ = CALL(IRFunction, *self, get_succ, /, fr);
    CALL(ListPtr, *fr_succ, push_back, /, to);
    ListPtr *to_pred = CALL(IRFunction, *self, get_pred, /, to);
    CALL(ListPtr, *to_pred, push_back, /, fr);
}

// engine is nullable to ignore error
static void MTD(IRFunction, build_graph, /, TaskEngine *engine) {

    // insert all labels into the label_to_block map
    for (ListBoxBBNode *it = self->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (bb->label != (usize)-1) {
            MapLabelBBInsertResult res = CALL(MapLabelBB, self->label_to_block,
                                              insert, /, bb->label, bb);
            if (!res.inserted && engine) {
                // already exists
                engine->parse_err = true;
                printf("Semantic error: Redefinition of label %zu\n",
                       bb->label);
            }
        }
    }

    // build the graph
    for (ListBoxBBNode *it = self->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (bb == self->exit) {
            continue;
        }
        ASSERT(it->next);
        IRBasicBlock *nxt_bb = it->next->data;
        if (bb->stmts.size == 0) {
            CALL(IRFunction, *self, add_edge, /, bb, nxt_bb);
            continue;
        }

        IRStmtBase *last_stmt = bb->stmts.tail->data;
        if (last_stmt->kind == IRStmtKindGoto) {
            IRStmtGoto *goto_stmt = (IRStmtGoto *)last_stmt;
            IRBasicBlock *target =
                CALL(IRFunction, *self, label_to_bb, /, goto_stmt->label);
            if (target) {
                CALL(IRFunction, *self, add_edge, /, bb, target);
            }
        } else if (last_stmt->kind == IRStmtKindIf) {
            IRStmtIf *if_stmt = (IRStmtIf *)last_stmt;
            IRBasicBlock *true_target =
                CALL(IRFunction, *self, label_to_bb, /, if_stmt->true_label);
            if (true_target) {
                CALL(IRFunction, *self, add_edge, /, bb, true_target);
            }
            IRBasicBlock *false_target =
                CALL(IRFunction, *self, label_to_bb, /, if_stmt->false_label);
            if (!false_target) {
                // if the false label is not defined, go to the next
                // block
                false_target = nxt_bb;
            }
            CALL(IRFunction, *self, add_edge, /, bb, false_target);
        } else if (last_stmt->kind == IRStmtKindReturn) {
            CALL(IRFunction, *self, add_edge, /, bb, self->exit);
        } else {
            CALL(IRFunction, *self, add_edge, /, bb, nxt_bb);
        }
    }
}

void MTD(IRFunction, establish, /, TaskEngine *engine) {
    ASSERT(self->basic_blocks.tail);
    self->entry = CREOBJHEAP(IRBasicBlock, /, (usize)-1);
    CALL(ListBoxBB, self->basic_blocks, push_front, /, self->entry);
    self->exit = CREOBJHEAP(IRBasicBlock, /, (usize)-1);
    CALL(ListBoxBB, self->basic_blocks, push_back, /, self->exit);
    CALL(IRFunction, *self, build_graph, /, engine);
}

void MTD(IRFunction, reestablish, /) {
    CALL(MapBBToListBB, self->block_pred, clear, /);
    CALL(MapBBToListBB, self->block_succ, clear, /);
    CALL(IRFunction, *self, build_graph, /, NULL);
}

IRBasicBlock *MTD(IRFunction, label_to_bb, /, usize label) {
    if (label == (usize)-1) {
        return NULL;
    }
    MapLabelBBIterator it =
        CALL(MapLabelBB, self->label_to_block, find_owned, /, label);
    if (!it) {
        return NULL;
    }
    return it->value;
}

ListPtr *MTD(IRFunction, get_pred, /, IRBasicBlock *bb) {
    MapBBToListBBIterator it =
        CALL(MapBBToListBB, self->block_pred, find_owned, /, bb);
    if (!it) {
        ListPtr new_list = CREOBJ(ListPtr, /);
        MapBBToListBBInsertResult res =
            CALL(MapBBToListBB, self->block_pred, insert, /, bb, new_list);
        ASSERT(res.inserted);
        it = res.node;
    }
    return &it->value;
}

ListPtr *MTD(IRFunction, get_succ, /, IRBasicBlock *bb) {
    MapBBToListBBIterator it =
        CALL(MapBBToListBB, self->block_succ, find_owned, /, bb);
    if (!it) {
        ListPtr new_list = CREOBJ(ListPtr, /);
        MapBBToListBBInsertResult res =
            CALL(MapBBToListBB, self->block_succ, insert, /, bb, new_list);
        ASSERT(res.inserted);
        it = res.node;
    }
    return &it->value;
}

void MTD(IRFunction, build_str, /, String *builder) {
    const char *func_name = STRING_C_STR(self->func_name);
    CALL(String, *builder, pushf, /, "FUNCTION %s :\n", func_name);
    for (usize i = 0; i < self->params.size; i++) {
        CALL(String, *builder, pushf, /, "PARAM v%zu\n", self->params.data[i]);
    }
    for (MapVarToDecInfoIterator it =
             CALL(MapVarToDecInfo, self->var_to_dec_info, begin, /);
         it; it = CALL(MapVarToDecInfo, self->var_to_dec_info, next, /, it)) {
        CALL(String, *builder, pushf, /, "DEC v%zu %d\n", it->key,
             it->value.size);
        CALL(String, *builder, pushf, /, "v%zu := &v%zu\n", it->value.addr,
             it->key);
    }
    for (ListBoxBBNode *it = self->basic_blocks.head; it; it = it->next) {
        CALL(IRBasicBlock, *it->data, build_str, /, builder);
    }
}

void MTD(IRFunction, iter_stmt, /, IterStmtCallback callback,
         void *extra_args) {
    for (ListBoxBBNode *bb_it = self->basic_blocks.head; bb_it;
         bb_it = bb_it->next) {
        IRBasicBlock *bb = bb_it->data;
        for (ListDynIRStmtNode *stmt_it = bb->stmts.head, *nxt = NULL; stmt_it;
             stmt_it = nxt) {
            nxt = stmt_it->next;
            bool has_inserted = callback(self, bb, stmt_it, extra_args);
            if (has_inserted) {
                nxt = stmt_it->next;
            }
        }
    }
}

void MTD(IRFunction, iter_bb, /, IterBBCallback callback, void *extra_args) {
    for (ListBoxBBNode *bb_it = self->basic_blocks.head, *nxt = NULL; bb_it;
         bb_it = nxt) {
        nxt = bb_it->next;
        bool has_inserted = callback(self, bb_it, extra_args);
        if (has_inserted) {
            nxt = bb_it->next;
        }
    }
}

static bool MTD(IRFunction, remove_dead_bb_link_callback, /,
                ListBoxBBNode *bb_it, ATTR_UNUSED void *extra_args) {
    IRBasicBlock *bb = bb_it->data;
    if (!bb->is_dead) {
        ListPtr *pred = CALL(IRFunction, *self, get_pred, /, bb);
        for (ListPtrNode *it = pred->head, *nxt = NULL; it; it = nxt) {
            nxt = it->next;
            IRBasicBlock *pred_bb = it->data;
            if (pred_bb->is_dead) {
                CALL(ListPtr, *pred, remove, /, it);
            }
        }
        ListPtr *succ = CALL(IRFunction, *self, get_succ, /, bb);
        for (ListPtrNode *it = succ->head, *nxt = NULL; it; it = nxt) {
            nxt = it->next;
            IRBasicBlock *succ_bb = it->data;
            if (succ_bb->is_dead) {
                CALL(ListPtr, *succ, remove, /, it);
            }
        }
    }
    return false;
}

static bool MTD(IRFunction, remove_dead_bb_itself_callback, /,
                ListBoxBBNode *bb_it, void *extra_args) {
    bool *updated = extra_args;
    if (bb_it->data->is_dead) {
        CALL(ListBoxBB, self->basic_blocks, remove, /, bb_it);
        *updated = true;
    }
    return false;
}

bool MTD(IRFunction, remove_dead_bb, /) {
    bool updated = false;
    CALL(IRFunction, *self, iter_bb, /,
         MTDNAME(IRFunction, remove_dead_bb_link_callback), NULL);
    CALL(IRFunction, *self, iter_bb, /,
         MTDNAME(IRFunction, remove_dead_bb_itself_callback), &updated);
    return updated;
}

static bool MTD(IRFunction, remove_dead_stmt_callback, /, IRBasicBlock *bb,
                ListDynIRStmtNode *stmt_it, void *extra_args) {
    bool *updated = extra_args;
    if (stmt_it->data->is_dead) {
        CALL(ListDynIRStmt, bb->stmts, remove, /, stmt_it);
        *updated = true;
    }
    return false;
}

bool MTD(IRFunction, remove_dead_stmt, /) {
    bool updated = false;
    CALL(IRFunction, *self, iter_stmt, /,
         MTDNAME(IRFunction, remove_dead_stmt_callback), &updated);
    return updated;
}

DEFINE_MAPPING(MapVarToDecInfo, usize, DecInfo, FUNC_EXTERN);
DEFINE_CLASS_VEC(VecIRFunction, IRFunction, FUNC_EXTERN);
