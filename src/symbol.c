#include "symbol.h"

/* SymbolTable */

void MTD(SymbolTable, init, /, usize parent_idx) {
    self->parent_idx = parent_idx;
    CALL(MapSymtab, self->mapping, init, /);
}

void MTD(SymbolTable, drop, /) { DROPOBJ(MapSymtab, self->mapping); }

bool MTD(SymbolTable, is_root, /) {
    return self->parent_idx == SYMTAB_NO_PARENT;
}

MapSymtabInsertResult MTD(SymbolTable, insert, /, HString name,
                          SymbolEntryKind kind, usize type_idx) {
    SymbolEntry entry = CREOBJ(SymbolEntry, /, kind, type_idx);
    MapSymtabInsertResult result =
        CALL(MapSymtab, self->mapping, insert, /, name, entry);
    if (result.inserted) {
        HString *key = &result.node->key;
        SymbolEntry *value = &result.node->value;
        value->table = self;
        value->name = &key->s;
    }
    return result;
}

MapSymtabIterator MTD(SymbolTable, find, /, HString *name) {
    return CALL(MapSymtab, self->mapping, find, /, name);
}

MapSymtabIterator MTD(SymbolTable, find_recursive, /, HString *name,
                      SymbolManager *manager) {
    while (true) {
        MapSymtabIterator it = CALL(SymbolTable, *self, find, /, name);
        if (it != NULL || CALL(SymbolTable, *self, is_root, /)) {
            return it;
        }
        self = &manager->tables.data[self->parent_idx];
    }
}

/* SymbolManager */

void MTD(SymbolManager, init, /) {
    CALL(VecSymbolTable, self->tables, init, /);
    self->root_idx = CALL(SymbolManager, *self, add_table, /, SYMTAB_NO_PARENT);
    self->temp_cnt = 0;
}

void MTD(SymbolManager, drop, /) { DROPOBJ(VecSymbolTable, self->tables); }

usize MTD(SymbolManager, add_table, /, usize parent_idx) {
    ASSERT(parent_idx < self->tables.size || parent_idx == SYMTAB_NO_PARENT);
    SymbolTable table = CREOBJ(SymbolTable, /, parent_idx);
    CALL(VecSymbolTable, self->tables, push_back, /, table);
    return self->tables.size - 1;
}

SymbolTable *MTD(SymbolManager, get_table, /, usize idx) {
    ASSERT(idx < self->tables.size);
    return &self->tables.data[idx];
}

HString MTD(SymbolManager, make_temp, /) {
    String s = NSCALL(String, from_f, /, "@temp_%zu", self->temp_cnt);
    self->temp_cnt++;
    return NSCALL(HString, from_inner, /, s);
}

DEFINE_MAPPING(MapSymtab, HString, SymbolEntry, FUNC_EXTERN);
DEFINE_CLASS_VEC(VecSymbolTable, SymbolTable, FUNC_EXTERN);
