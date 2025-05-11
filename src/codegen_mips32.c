#include "codegen_mips32.h"
#include "general_vec.h"
#include "ir.h"
#include "str.h"
#include "tem_map.h"
#include "utils.h"

// clang-format off
enum REGS {
    REG_ZERO, REG_AT, REG_V0, REG_V1, REG_A0, REG_A1, REG_A2, REG_A3,
    REG_T0, REG_T1, REG_T2, REG_T3, REG_T4, REG_T5, REG_T6, REG_T7,
    REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5, REG_S6, REG_S7,
    REG_T8, REG_T9, REG_K0, REG_K1, REG_GP, REG_SP, REG_FP, REG_RA,
    NUM_REGS,
};

static const char *const REG_TO_NAME[NUM_REGS] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8",   "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
};

static const usize USABLE_REGS[] = {
    // --- callee saved ---
    REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5, REG_S6, REG_S7,
    // --- caller saved ---
    REG_V1,
    REG_T0, REG_T1, REG_T2, REG_T3, REG_T4, REG_T5, REG_T6, REG_T7,
    REG_T8, REG_T9,
    REG_A0, REG_A1, REG_A2, REG_A3,
};
static const usize CALLER_SAVED_REGS[] = {
    REG_V1,
    REG_T0, REG_T1, REG_T2, REG_T3, REG_T4, REG_T5, REG_T6, REG_T7,
    REG_T8, REG_T9,
    REG_A0, REG_A1, REG_A2, REG_A3,
};
static const usize CALLEE_SAVED_REGS[] = {
    REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5, REG_S6, REG_S7,
};
// clang-format on

typedef struct AddrDesc {
    isize offset; // the virtual address of the variable: offset($fp)
    usize corr_reg_idx;
} AddrDesc;

typedef struct RegDesc {
    AddrDesc *corr_addr;
    bool dirty;
    bool was_used;
} RegDesc;

DECLARE_MAPPING(MapVarAddr, usize, AddrDesc, FUNC_STATIC, GENERATOR_PLAIN_KEY,
                GENERATOR_PLAIN_VALUE, GENERATOR_PLAIN_COMPARATOR);
DEFINE_MAPPING(MapVarAddr, usize, AddrDesc, FUNC_STATIC);

typedef struct FuncInfo {
    usize arity;
    String *func_name;
    String body_asm;
    MapVarAddr var_to_addr;
    RegDesc regs[NUM_REGS];
    isize offset_occupied;
    usize candidate_evict_reg;
    VecPtr arglist; // of type IREntity*
    usize num_parsed_arg;
    bool has_func_call;
} FuncInfo;

FUNC_STATIC void MTD(FuncInfo, init, /) {
    self->arity = 0;
    self->func_name = NULL;
    CALL(String, self->body_asm, init, /);
    CALL(MapVarAddr, self->var_to_addr, init, /);
    for (usize i = 0; i < NUM_REGS; i++) {
        self->regs[i].corr_addr = NULL;
        self->regs[i].was_used = false;
        self->regs[i].dirty = false;
    }
    self->offset_occupied = -4 * (2 + LENGTH(CALLEE_SAVED_REGS));
    // -4; ra, fp, callee saved
    self->candidate_evict_reg = 0;
    CALL(VecPtr, self->arglist, init, /);
    self->num_parsed_arg = 0;
    self->has_func_call = false;
}

FUNC_STATIC void MTD(FuncInfo, drop, /) {
    DROPOBJ(VecPtr, self->arglist);
    DROPOBJ(MapVarAddr, self->var_to_addr);
    DROPOBJ(String, self->body_asm);
}

void MTD(CodegenMips32, init, /, struct IRManager *ir_manager) {
    CALL(String, self->result, init, /);
    self->ir_manager = ir_manager;
}

void MTD(CodegenMips32, drop, /) { DROPOBJ(String, self->result); }

static AddrDesc *get_addr(FuncInfo *info, usize var_idx) {
    MapVarAddrIterator iter =
        CALL(MapVarAddr, info->var_to_addr, find_owned, /, var_idx);
    if (iter == NULL) {
        info->offset_occupied -= 4;
        isize now_offset = info->offset_occupied;
        MapVarAddrInsertResult res =
            CALL(MapVarAddr, info->var_to_addr, insert, /, var_idx,
                 (AddrDesc){.offset = now_offset, .corr_reg_idx = (usize)-1});
        ASSERT(res.inserted);
        iter = res.node;
    }
    return &iter->value;
}

#define MAX_REG_IN_IR 3

static void writeback_reg(FuncInfo *info, usize reg) {
    RegDesc *reg_desc = &info->regs[reg];
    AddrDesc *addr_desc = reg_desc->corr_addr;
    if (addr_desc && reg_desc->dirty) {
        // write back dirty reg
        CALL(String, info->body_asm, pushf, /, "  sw %s, %zd($fp)\n",
             REG_TO_NAME[reg], addr_desc->offset);
    }
    if (addr_desc) {
        addr_desc->corr_reg_idx = (usize)-1;
    }
    // clear reg
    reg_desc->corr_addr = NULL;
    reg_desc->dirty = false;
}

static void writeback_regs(FuncInfo *info) {
    for (usize i = 0; i < LENGTH(USABLE_REGS); i++) {
        usize reg = USABLE_REGS[i];
        writeback_reg(info, reg);
    }
    info->candidate_evict_reg = 0;
}

static void get_reg(FuncInfo *info, usize num, usize var_idxs[],
                    usize reg_idxs[], bool has_ret) {
    ASSERT(num <= MAX_REG_IN_IR);
    AddrDesc *addr_descs[MAX_REG_IN_IR] = {NULL};
    for (usize i = 0; i < num; i++) {
        reg_idxs[i] = (usize)-1;
    }

    // for those addr that are already in reg
    for (usize i = 0; i < num; i++) {
        usize var_idx = var_idxs[i];
        if (var_idx == (usize)-1) {
            // temp
            continue;
        }
        AddrDesc *addr_desc = get_addr(info, var_idx);
        addr_descs[i] = addr_desc;
        if (addr_desc->corr_reg_idx != (usize)-1) {
            reg_idxs[i] = addr_desc->corr_reg_idx;
        }
    }

    // for those addr that are not in reg
    for (usize i = 0; i < num; i++) {
        if (reg_idxs[i] != (usize)-1) {
            continue;
        }

        // find a proper reg
        usize candidate;
        while (true) {
            candidate = USABLE_REGS[info->candidate_evict_reg];
            info->candidate_evict_reg += 1;
            if (info->candidate_evict_reg == LENGTH(USABLE_REGS)) {
                info->candidate_evict_reg = 0;
            }

            if (!info->regs[candidate].corr_addr) {
                break;
            }

            bool can_use = true;
            for (usize j = 0; j < num; j++) {
                if (candidate == reg_idxs[j]) {
                    can_use = false;
                    break;
                }
            }
            if (can_use) {
                break;
            }
        }
        reg_idxs[i] = candidate;
        writeback_reg(info, candidate);

        RegDesc *reg_desc = &info->regs[candidate];
        reg_desc->was_used = true;

        // assign reg if not temp
        AddrDesc *addr_desc = addr_descs[i];
        if (addr_desc) {
            reg_desc->corr_addr = addr_desc;
            addr_desc->corr_reg_idx = candidate;
            if (!(i == 0 && has_ret)) {
                // the ret var need not to be loaded
                CALL(String, info->body_asm, pushf, /, "  lw %s, %zd($fp)\n",
                     REG_TO_NAME[candidate], addr_desc->offset);
            }
        }
    }
}

void MTD(CodegenMips32, generate, /) {
    CALL(CodegenMips32, *self, add_predefined_snippets, /);

    VecPtr *irs = &self->ir_manager->irs;
    usize last_func_start = (usize)-1;
    for (usize i = 0; i < irs->size; i++) {
        IR *ir = (IR *)irs->data[i];
        if (ir->kind == IRFunction) {
            if (last_func_start != (usize)-1) {
                CALL(CodegenMips32, *self, generate_func, /, last_func_start,
                     i);
            }
            last_func_start = i;
        }
    }
    if (last_func_start != (usize)-1) {
        CALL(CodegenMips32, *self, generate_func, /, last_func_start,
             irs->size);
    }
}

void MTD(CodegenMips32, add_predefined_snippets, /) {
    CALL(String, self->result, push_str, /,
         ".data\n"
         "_prompt: .asciiz \"Enter an integer:\"\n"
         "_ret: .asciiz \"\\n\"\n"
         ".globl main\n"
         ".text\n"
         "_Fread:\n"
         "  li $v0, 4\n"
         "  la $a0, _prompt\n"
         "  syscall\n"
         "  li $v0, 5\n"
         "  syscall\n"
         "  jr $ra\n"
         "\n"
         "_Fwrite:\n"
         "  li $v0, 1\n"
         "  syscall\n"
         "  li $v0, 4\n"
         "  la $a0, _ret\n"
         "  syscall\n"
         "  move $v0, $0\n"
         "  jr $ra\n");
}

#define CODE_GENERATOR(irkind)                                                 \
    FUNC_STATIC void MTD(CodegenMips32, CONCATENATE(code_generator_, irkind),  \
                         /, ATTR_UNUSED IR * ir, ATTR_UNUSED FuncInfo * info)

#define CODE_GENERATOR_DECLARE_AID(ir_kind) CODE_GENERATOR(ir_kind);
APPLY_IR_KIND(CODE_GENERATOR_DECLARE_AID);

#define CODE_GENERATOR_DISPATCH_BY_IRKIND_AID(ir_kind)                         \
    [CONCATENATE(IR, ir_kind)] =                                               \
        MTDNAME(CodegenMips32, CONCATENATE(code_generator_, ir_kind)),

typedef void (*CodeGeneratorFunc)(CodegenMips32 *, IR *, FuncInfo *);
static const CodeGeneratorFunc CODE_GENERATOR_DISPATCH_BY_IRKIND[] = {
    [IRInvalid] = NULL, APPLY_IR_KIND(CODE_GENERATOR_DISPATCH_BY_IRKIND_AID)

};

static void get_func_info_from_dec(FuncInfo *info, IR *ir) {
    ASSERT(ir->kind == IRFunction);
    IREntity *ent = &ir->e1;
    ASSERT(ent->kind == IREntityFun);
    info->arity = ent->as_fun.arity;
    info->func_name = ent->as_fun.name;
}

void MTD(CodegenMips32, generate_func, /, usize start, usize end) {
    VecPtr *irs = &self->ir_manager->irs;

    FuncInfo *info = CREOBJHEAP(FuncInfo, /);
    get_func_info_from_dec(info, irs->data[start]);

    // handle the function body
    for (usize i = start + 1; i < end; i++) {
        IR *ir = irs->data[i];
        ASSERT(ir->kind != IRInvalid);

        // comment the ir code in the asm code
        CALL(String, info->body_asm, push_str, /, "  # ");
        CALL(IR, *ir, build_str, /, &info->body_asm);

        CODE_GENERATOR_DISPATCH_BY_IRKIND[ir->kind](self, ir, info);

        // writeback
        // TODO: only writeback at the end of the basic block
        writeback_regs(info);
    }

    // start to generate the complete code

    const char *func_name = STRING_C_STR(*info->func_name);

    // generate the function prologue
    if (strcmp(func_name, "main") == 0) {
        CALL(String, self->result, push_str, /, "\nmain:\n");
    } else {
        CALL(String, self->result, pushf, /, "\n_F%s:\n", func_name);
    }

    usize frame_size = -info->offset_occupied;
    CALL(String, self->result, pushf, /, "  subu $sp, $sp, %zu\n", frame_size);

    if (info->has_func_call) {

        CALL(String, self->result, pushf, /, "  sw $ra, %zu($sp)\n",
             frame_size - 4);
    }

    CALL(String, self->result, pushf, /,
         "  sw $fp, %zu($sp)\n"
         "  addi $fp, $sp, %zu\n",
         frame_size - 8, frame_size);

    for (usize i = 0; i < LENGTH(CALLEE_SAVED_REGS); i++) {
        usize reg = CALLEE_SAVED_REGS[i];
        if (info->regs[reg].was_used) {
            CALL(String, self->result, pushf, /, "  sw %s, %zi($fp)\n",
                 REG_TO_NAME[reg], -4 * (3 + i));
        }
    }

    // the body
    const char *body_codegen = STRING_C_STR(info->body_asm);
    CALL(String, self->result, push_str, /, body_codegen);

    // generate the function epilogue
    CALL(String, self->result, pushf, /, "_R%s:\n", func_name);

    for (usize i = 0; i < LENGTH(CALLEE_SAVED_REGS); i++) {
        usize reg = CALLEE_SAVED_REGS[i];
        if (info->regs[reg].was_used) {
            CALL(String, self->result, pushf, /, "  lw %s, %zi($fp)\n",
                 REG_TO_NAME[reg], -4 * (3 + i));
        }
    }
    if (info->has_func_call) {
        CALL(String, self->result, pushf, /, "  lw $ra, %zu($sp)\n",
             frame_size - 4);
    }
    CALL(String, self->result, pushf, /,
         "  lw $fp, %zu($sp)\n"
         "  addi $sp, $sp, %zu\n"
         "  jr $ra\n",
         frame_size - 8, frame_size);

    // done
    DROPOBJHEAP(FuncInfo, info);
}

CODE_GENERATOR(Label) {
    CALL(String, info->body_asm, pushf, /, "_L%zu:\n", ir->e1.label_idx);
}
CODE_GENERATOR(Function) { PANIC("Should not be called"); }
CODE_GENERATOR(Assign) {
    IREntity *e1 = &ir->e1;
    IREntity *ret = &ir->ret;
    if (e1->kind == IREntityImmInt || e1->kind == IREntityAddr) {
        ASSERT(ret->kind == IREntityVar);
        usize var_idxs[1] = {ret->var_idx};
        usize reg_idxs[1];
        get_reg(info, 1, var_idxs, reg_idxs, true);
        usize ret_reg = reg_idxs[0];
        if (e1->kind == IREntityImmInt) {
            // <ret:var> = <e1:imm>
            CALL(String, info->body_asm, pushf, /, "  li %s, %d\n",
                 REG_TO_NAME[ret_reg], e1->imm_int);
        } else {
            // <ret:var> = <e1:addr>
            AddrDesc *addr_desc = get_addr(info, e1->var_idx);
            CALL(String, info->body_asm, pushf, /, "  addi %s, $fp, %zd\n",
                 REG_TO_NAME[ret_reg], addr_desc->offset);
        }
        info->regs[ret_reg].dirty = true;
    } else {
        usize var_idxs[2] = {ret->var_idx, e1->var_idx};
        usize reg_idxs[2];
        // NOTE: for <ret:deref> = <e1:var>, we need to get the actual value;
        // thus has_ret shall be false
        bool has_ret = ret->kind == IREntityVar;
        get_reg(info, 2, var_idxs, reg_idxs, has_ret);
        usize ret_reg = reg_idxs[0];
        usize e1_reg = reg_idxs[1];
        if (ret->kind == IREntityVar) {
            if (e1->kind == IREntityVar) {
                // <ret:var> = <e1:var>
                CALL(String, info->body_asm, pushf, /, "  move %s, %s\n",
                     REG_TO_NAME[ret_reg], REG_TO_NAME[e1_reg]);
            } else if (e1->kind == IREntityDeref) {
                // <ret:var> = <e1:deref>
                CALL(String, info->body_asm, pushf, /, "  lw %s, 0(%s)\n",
                     REG_TO_NAME[ret_reg], REG_TO_NAME[e1_reg]);
            } else {
                PANIC("Should not be called");
            }
            info->regs[ret_reg].dirty = true;
        } else if (ret->kind == IREntityDeref) {
            // <ret:deref> = <e1:var>
            ASSERT(e1->kind == IREntityVar, "e1 should be var");
            CALL(String, info->body_asm, pushf, /, "  sw %s, 0(%s)\n",
                 REG_TO_NAME[e1_reg], REG_TO_NAME[ret_reg]);
        } else {
            PANIC("Should not be called");
        }
    }
}
CODE_GENERATOR(ArithAssign) {
    IREntity *e1 = &ir->e1;
    IREntity *e2 = &ir->e2;
    IREntity *ret = &ir->ret;
    ArithopKind aop = ir->arithop_val;

    usize var_idxs[3] = {ret->var_idx,
                         e1->kind == IREntityVar ? e1->var_idx : (usize)-1,
                         e2->kind == IREntityVar ? e2->var_idx : (usize)-1};
    usize reg_idxs[3];
    get_reg(info, 3, var_idxs, reg_idxs, true);
    usize ret_reg = reg_idxs[0];
    usize e1_reg = reg_idxs[1];
    usize e2_reg = reg_idxs[2];
    if (e1->kind == IREntityImmInt) {
        CALL(String, info->body_asm, pushf, /, "  li %s, %d\n",
             REG_TO_NAME[e1_reg], e1->imm_int);
    }
    if (e2->kind == IREntityImmInt) {
        CALL(String, info->body_asm, pushf, /, "  li %s, %d\n",
             REG_TO_NAME[e2_reg], e2->imm_int);
    }

    if (aop == ArithopAdd) {
        CALL(String, info->body_asm, pushf, /, "  add %s, %s, %s\n",
             REG_TO_NAME[ret_reg], REG_TO_NAME[e1_reg], REG_TO_NAME[e2_reg]);
    } else if (aop == ArithopSub) {
        CALL(String, info->body_asm, pushf, /, "  sub %s, %s, %s\n",
             REG_TO_NAME[ret_reg], REG_TO_NAME[e1_reg], REG_TO_NAME[e2_reg]);
    } else if (aop == ArithopMul) {
        CALL(String, info->body_asm, pushf, /, "  mul %s, %s, %s\n",
             REG_TO_NAME[ret_reg], REG_TO_NAME[e1_reg], REG_TO_NAME[e2_reg]);
    } else if (aop == ArithopDiv) {
        CALL(String, info->body_asm, pushf, /, "  div %s, %s\n",
             REG_TO_NAME[e1_reg], REG_TO_NAME[e2_reg]);
        CALL(String, info->body_asm, pushf, /, "  mflo %s\n",
             REG_TO_NAME[ret_reg]);
    } else {
        PANIC("Should not be reached");
    }
    info->regs[ret_reg].dirty = true;
}
CODE_GENERATOR(Goto) {
    IREntity *label = &ir->ret;
    ASSERT(label->kind == IREntityLabel);
    CALL(String, info->body_asm, pushf, /, "  j _L%zu\n", label->label_idx);
}
CODE_GENERATOR(CondGoto) {
    IREntity *e1 = &ir->e1;
    IREntity *e2 = &ir->e2;
    IREntity *label = &ir->ret;
    ASSERT(label->kind == IREntityLabel);
    usize var_idxs[2] = {e1->kind == IREntityVar ? e1->var_idx : (usize)-1,
                         e2->kind == IREntityVar ? e2->var_idx : (usize)-1};
    usize reg_idxs[2];
    get_reg(info, 2, var_idxs, reg_idxs, false);
    usize e1_reg = reg_idxs[0];
    usize e2_reg = reg_idxs[1];
    if (e1->kind == IREntityImmInt) {
        CALL(String, info->body_asm, pushf, /, "  li %s, %d\n",
             REG_TO_NAME[e1_reg], e1->imm_int);
    }
    if (e2->kind == IREntityImmInt) {
        CALL(String, info->body_asm, pushf, /, "  li %s, %d\n",
             REG_TO_NAME[e2_reg], e2->imm_int);
    }
    static const char *const RELOP_TO_STR[] = {
        [RelopGT] = "bgt", [RelopLT] = "blt", [RelopGE] = "bge",
        [RelopLE] = "ble", [RelopEQ] = "beq", [RelopNE] = "bne",
    };
    CALL(String, info->body_asm, pushf, /, "  %s %s, %s, _L%zu\n",
         RELOP_TO_STR[ir->relop_val], REG_TO_NAME[e1_reg], REG_TO_NAME[e2_reg],
         label->label_idx);
}
CODE_GENERATOR(Return) {
    IREntity *e1 = &ir->e1;
    ASSERT(e1->kind == IREntityVar);
    usize var_idxs[1] = {e1->var_idx};
    usize reg_idxs[1];
    get_reg(info, 1, var_idxs, reg_idxs, false);
    usize e1_reg = reg_idxs[0];
    CALL(String, info->body_asm, pushf, /, "  move $v0, %s\n",
         REG_TO_NAME[e1_reg]);
    const char *func_name = STRING_C_STR(*info->func_name);
    CALL(String, info->body_asm, pushf, /, "  j _R%s\n", func_name);
}
CODE_GENERATOR(Dec) {
    IREntity *dec = &ir->ret;
    IREntity *imm = &ir->e1;
    ASSERT(dec->kind == IREntityVar);
    ASSERT(imm->kind == IREntityImmInt);
    info->offset_occupied -= imm->imm_int;
    isize offset = info->offset_occupied;
    MapVarAddrInsertResult res =
        CALL(MapVarAddr, info->var_to_addr, insert, /, dec->var_idx,
             (AddrDesc){.offset = offset, .corr_reg_idx = (usize)-1});
    ASSERT(res.inserted);
}
CODE_GENERATOR(Arg) {
    IREntity *arg = &ir->e1;
    ASSERT(arg->kind == IREntityVar);
    CALL(VecPtr, info->arglist, push_back, /, arg);
}
CODE_GENERATOR(Call) {
    info->has_func_call = true;

    IREntity *ret = &ir->ret;
    ASSERT(ret->kind == IREntityVar);
    IREntity *fun = &ir->e1;
    ASSERT(fun->kind == IREntityFun);

    usize callee_arity = fun->as_fun.arity;
    const char *callee_name = STRING_C_STR(*fun->as_fun.name);

    // writeback caller saved regs
    for (usize i = 0; i < LENGTH(CALLER_SAVED_REGS); i++) {
        usize reg = CALLER_SAVED_REGS[i];
        writeback_reg(info, reg);
    }

    // prepare parameter
    ASSERT(info->arglist.size >= callee_arity);
    usize num_pass_by_reg = Min((usize)4, callee_arity);
    usize num_pass_by_stack = callee_arity - num_pass_by_reg;
    if (num_pass_by_stack != 0) {
        CALL(String, info->body_asm, pushf, /, "  subu $sp, $sp, %zu\n",
             4 * num_pass_by_stack);
    }

    for (usize i = 0; i < callee_arity; i++) {
        IREntity *arg = *CALL(VecPtr, info->arglist, back, /);
        CALL(VecPtr, info->arglist, pop_back, /);

        AddrDesc *addr_desc = get_addr(info, arg->var_idx);
        if (i < num_pass_by_reg) {
            if (addr_desc->corr_reg_idx != (usize)-1) {
                CALL(String, info->body_asm, pushf, /, "  move %s, %s\n",
                     REG_TO_NAME[REG_A0 + i],
                     REG_TO_NAME[addr_desc->corr_reg_idx]);
            } else {
                CALL(String, info->body_asm, pushf, /, "  lw %s, %zd($fp)\n",
                     REG_TO_NAME[REG_A0 + i], addr_desc->offset);
            }
        } else {
            if (addr_desc->corr_reg_idx != (usize)-1) {
                CALL(String, info->body_asm, pushf, /, "  sw %s, %zu($sp)\n",
                     REG_TO_NAME[addr_desc->corr_reg_idx],
                     4 * (i - num_pass_by_reg));
            } else {
                // NOTE: use $v1 to temporarily store the value
                CALL(String, info->body_asm, pushf, /, "  lw $v1, %zd($fp)\n",
                     addr_desc->offset);
                CALL(String, info->body_asm, pushf, /, "  sw $v1, %zu($sp)\n",
                     4 * (i - num_pass_by_reg));
            }
        }
    }

    // call the function
    if (strcmp(callee_name, "main") == 0) {
        CALL(String, info->body_asm, pushf, /, "  jal main\n");
    } else {
        CALL(String, info->body_asm, pushf, /, "  jal _F%s\n", callee_name);
    }

    // resume sp
    if (num_pass_by_stack != 0) {
        CALL(String, info->body_asm, pushf, /, "  addu $sp, $sp, %zu\n",
             4 * num_pass_by_stack);
    }

    // return value
    if (ret->var_idx != (usize)-1) { // for moked write, which has no ret
        usize var_idxs[1] = {ret->var_idx};
        usize reg_idxs[1];
        get_reg(info, 1, var_idxs, reg_idxs, true);
        usize ret_reg = reg_idxs[0];
        CALL(String, info->body_asm, pushf, /, "  move %s, $v0\n",
             REG_TO_NAME[ret_reg]);
        info->regs[ret_reg].dirty = true;
    }
}
CODE_GENERATOR(Param) {
    IREntity *param = &ir->e1;
    ASSERT(param->kind == IREntityVar);
    usize param_var_idx = param->var_idx;

    usize now_parsed_param = info->num_parsed_arg;
    info->num_parsed_arg += 1;
    if (now_parsed_param < 4) {
        AddrDesc *addr_desc = get_addr(info, param_var_idx);
        usize target_reg = REG_A0 + now_parsed_param;
        RegDesc *reg_desc = &info->regs[target_reg];
        ASSERT(addr_desc->corr_reg_idx == (usize)-1);
        ASSERT(reg_desc->corr_addr == NULL);
        addr_desc->corr_reg_idx = target_reg;
        reg_desc->corr_addr = addr_desc;
        reg_desc->dirty = true;
    } else {
        isize offset = 4 * (now_parsed_param - 4);
        MapVarAddrInsertResult res =
            CALL(MapVarAddr, info->var_to_addr, insert, /, param_var_idx,
                 (AddrDesc){.offset = offset, .corr_reg_idx = (usize)-1});
        ASSERT(res.inserted);
    }
}
CODE_GENERATOR(Read) {
    String mocked_name = NSCALL(String, mock_raw, /, "read");
    IR moked_read = {
        .kind = IRFunction,
        .ret = ir->ret,
        .e1 =
            {
                .kind = IREntityFun,
                .as_fun =
                    {
                        .arity = 0,
                        .name = &mocked_name,
                    },
            },
    };
    CALL(CodegenMips32, *self, code_generator_Call, /, &moked_read, info);
}
CODE_GENERATOR(Write) {
    String mocked_name = NSCALL(String, mock_raw, /, "write");
    IR moked_write_arg = {
        .kind = IRParam,
        .e1 = ir->e1,
    };
    IR moked_write = {
        .kind = IRFunction,
        .ret =
            {
                .kind = IREntityVar,
                // hack in code_generator_Function: no ret var
                .var_idx = (usize)-1,

            },
        .e1 =
            {
                .kind = IREntityFun,
                .as_fun =
                    {
                        .arity = 1,
                        .name = &mocked_name,
                    },
            },
    };
    CALL(CodegenMips32, *self, code_generator_Arg, /, &moked_write_arg, info);
    CALL(CodegenMips32, *self, code_generator_Call, /, &moked_write, info);
}
