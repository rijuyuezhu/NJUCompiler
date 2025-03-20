#include "symbol.h"

/* SymbolEntry */

void MTD(SymbolEntry, drop, /) {
    if (self->kind == SymbolKindFun) {
        DROPOBJ(VecUSize, self->as_fun.type_idxs);
    }
}

SymbolEntry NSMTD(SymbolEntry, make_var, /, struct SymbolTable *table,
                  String *name, usize type_idx) {
    return (SymbolEntry){
        .kind = SymbolKindVar,
        .table = table,
        .name = name,
        .as_var = {.type_idx = type_idx},
    };
}

SymbolEntry NSMTD(SymbolEntry, make_fun, /, struct SymbolTable *table,
                  String *name) {
    return (SymbolEntry){
        .kind = SymbolKindFun,
        .table = table,
        .name = name,
        .as_fun = {.type_idxs = CREOBJ(VecUSize, /)},
    };
}

void MTD(SymbolEntry, fun_add, /, usize type_idx) {
    ASSERT(self->kind == SymbolKindFun);
    CALL(VecUSize, self->as_fun.type_idxs, push_back, /, type_idx);
}

SymbolEntry NSMTD(SymbolEntry, make_struct, /, struct SymbolTable *table,
                  String *name, usize type_idx) {
    return (SymbolEntry){
        .kind = SymbolKindStruct,
        .table = table,
        .name = name,
        .as_struct = {.type_idx = type_idx},
    };
}

/* SymbolTable */

void MTD(SymbolTable, init, /, usize parent_idx) {
    self->parent_idx = parent_idx;
    CALL(MapSymbolTable, self->mapping, init, /);
}

void MTD(SymbolTable, drop, /) { DROPOBJ(MapSymbolTable, self->mapping); }

MapSymbolTableInsertResult MTD(SymbolTable, insert, /, HString name,
                               SymbolEntry entry) {
    MapSymbolTableInsertResult result =
        CALL(MapSymbolTable, self->mapping, insert, /, name, entry);
    if (result.inserted) {
        HString *key = &result.node->key;
        SymbolEntry *value = &result.node->value;
        value->table = self;
        value->name = &key->s;
    }
    return result;
}

MapSymbolTableIterator MTD(SymbolTable, find, /, HString *name) {
    return CALL(MapSymbolTable, self->mapping, find, /, name);
}

MapSymbolTableIterator MTD(SymbolTable, find_recursive, /, HString *name,
                           SymbolManager *manager) {
    while (true) {
        MapSymbolTableIterator it = CALL(SymbolTable, *self, find, /, name);
        if (it != NULL || self->parent_idx == SYMBOL_TABLE_NO_PARENT) {
            return it;
        }
        self = &manager->tables.data[self->parent_idx];
    }
}

/* SymbolManager */

void MTD(SymbolManager, init, /) {
    CALL(VecSymbolTable, self->tables, init, /);
    self->root_idx =
        CALL(SymbolManager, *self, add_table, /, SYMBOL_TABLE_NO_PARENT);
}

void MTD(SymbolManager, drop, /) { DROPOBJ(VecSymbolTable, self->tables); }

usize MTD(SymbolManager, add_table, /, usize parent_idx) {
    SymbolTable table = CREOBJ(SymbolTable, /, parent_idx);
    CALL(VecSymbolTable, self->tables, push_back, /, table);
    return self->tables.size - 1;
}

DEFINE_MAPPING(MapSymbolTable, HString, SymbolEntry, FUNC_EXTERN);
DEFINE_CLASS_VEC(VecSymbolTable, SymbolTable, FUNC_EXTERN);
