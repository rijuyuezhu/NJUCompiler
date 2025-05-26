#pragma once
#include "ir_function.h"
#include "ir_program.h"
#include "ir_value.h"
#include "task_engine.h"
#include "utils.h"

static usize get_label(TaskEngine *engine, String label_name) {
    return CALL(IdxAllocator, engine->ir_program.label_idx_allocator, get, /,
                label_name);
}
static usize get_var(TaskEngine *engine, String var_name) {
    return CALL(IdxAllocator, engine->ir_program.var_idx_allocator, get, /,
                var_name);
}
static IRFunction *get_func(TaskEngine *engine) {

    return engine->parse_helper.now_func;
}
static void add_label(TaskEngine *engine, usize label) {
    IRFunction *now_func = get_func(engine);
    CALL(IRFunction, *now_func, add_label, /, label);
}
static void add_stmt(TaskEngine *engine, IRStmtBase *stmt) {
    IRFunction *now_func = get_func(engine);
    CALL(IRFunction, *now_func, add_stmt, /, stmt);
}
static IRValue get_ir_value_from_deref_value(TaskEngine *engine,
                                             IRValue deref_value) {
    usize temp =
        CALL(IdxAllocator, engine->ir_program.var_idx_allocator, allocate, /);
    IRStmtBase *stmt =
        (IRStmtBase *)CREOBJHEAP(IRStmtLoad, /, temp, deref_value);
    add_stmt(engine, stmt);
    return NSCALL(IRValue, from_var, /, temp);
}
static IRValue get_ir_value_from_var(ATTR_UNUSED TaskEngine *engine,
                                     usize var) {
    return NSCALL(IRValue, from_var, /, var);
}
static IRValue get_ir_value_from_const(ATTR_UNUSED TaskEngine *engine,
                                       int val) {
    return NSCALL(IRValue, from_const, /, val);
}
static IRValue get_ir_value_from_addr(TaskEngine *engine, usize var) {
    IRFunction *now_func = get_func(engine);
    MapVarToDecInfo *var_to_dec_info = &now_func->var_to_dec_info;
    MapVarToDecInfoIterator it =
        CALL(MapVarToDecInfo, *var_to_dec_info, find_owned, /, var);
    if (!it) {
        // already exists
        engine->parse_err = true;
        printf("Semantic error: do not support get the address of normal "
               "variable %zu\n",
               var);
        return NSCALL(IRValue, from_const, /, 0);
    }
    return NSCALL(IRValue, from_var, /, it->value.addr);
}
static void add_arg(TaskEngine *engine, IRValue arg) {
    CALL(VecIRValue, engine->parse_helper.arglist, push_back, /, arg);
}

static VecIRValue take_arglist(TaskEngine *engine) {
    VecIRValue arglist = engine->parse_helper.arglist;
    engine->parse_helper.arglist = CREOBJ(VecIRValue, /);
    return arglist;
}

static void add_dec(TaskEngine *engine, usize var, int size) {
    MapVarToDecInfo *var_to_dec_info =
        &engine->parse_helper.now_func->var_to_dec_info;
    usize addr =
        CALL(IdxAllocator, engine->ir_program.var_idx_allocator, allocate, /);
    MapVarToDecInfoInsertResult res =
        CALL(MapVarToDecInfo, *var_to_dec_info, insert, /, var,
             (DecInfo){.addr = addr, .size = size});
    if (!res.inserted) {
        // already exists
        engine->parse_err = true;
        printf("Semantic error: variable %zu already exists\n", var);
    }
}
static void add_param(TaskEngine *engine, usize param) {
    IRFunction *now_func = get_func(engine);
    CALL(VecUSize, now_func->params, push_back, /, param);
}
static void init_func(TaskEngine *engine, String func_name) {
    IRFunction func = CREOBJ(IRFunction, /, func_name, &engine->ir_program);
    VecIRFunction *functions = &engine->ir_program.functions;
    CALL(VecIRFunction, *functions, push_back, /, func);
    engine->parse_helper.now_func = CALL(VecIRFunction, *functions, back, /);
}
