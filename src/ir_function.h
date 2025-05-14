#pragma once
#include "general_container.h"
#include "ir_basic_block.h"
#include "str.h"

typedef struct DecInfo {
    usize addr;
    usize size;
} DecInfo;

DECLARE_MAPPING(MapVarToDecInfo, usize, DecInfo, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

// IRFunction
typedef struct IRFunction {
    String func_name;
    VecUSize params;
    MapVarToDecInfo var_to_dec_info;
    ListBasicBlock basic_blocks;
    IRBasicBlock *entry;
    IRBasicBlock *exit;
    MapLabelBB label_to_block;
    MapBBToListBB block_pred;
    MapBBToListBB block_succ;
} IRFunction;

void MTD(IRFunction, init, /, String func_name);
void MTD(IRFunction, drop, /);
void MTD(IRFunction, establish, /);
DELETED_CLONER(IRFunction, FUNC_STATIC)

DECLARE_CLASS_VEC(VecIRFunction, IRFunction, FUNC_EXTERN);
