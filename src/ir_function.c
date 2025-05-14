#include "ir_function.h"

void MTD(IRFunction, init, /, String func_name) {
    self->func_name = func_name;
    CALL(VecUSize, self->params, init, /);
    CALL(MapVarToDecInfo, self->var_to_dec_info, init, /);
    CALL(ListBasicBlock, self->basic_blocks, init, /);
    self->entry = NULL;
    self->exit = NULL;
    CALL(MapLabelBB, self->label_to_block, init, /);
    CALL(MapBBToListBB, self->block_pred, init, /);
    CALL(MapBBToListBB, self->block_succ, init, /);
}

void MTD(IRFunction, drop, /) {
    DROPOBJ(MapBBToListBB, self->block_succ);
    DROPOBJ(MapBBToListBB, self->block_pred);
    DROPOBJ(MapLabelBB, self->label_to_block);
    DROPOBJ(ListBasicBlock, self->basic_blocks);
    DROPOBJ(MapVarToDecInfo, self->var_to_dec_info);
    DROPOBJ(VecUSize, self->params);
    DROPOBJ(String, self->func_name);
}

void MTD(IRFunction, establish, /) {

}

DEFINE_MAPPING(MapVarToDecInfo, usize, DecInfo, FUNC_EXTERN);
DEFINE_CLASS_VEC(VecIRFunction, IRFunction, FUNC_EXTERN);
