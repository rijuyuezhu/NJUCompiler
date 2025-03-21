#pragma once
#include "general_vec.h"
#include "str.h"
#include "tem_map.h"
#include "tem_vec.h"
#include "utils.h"

/* SymbolEntry */

typedef enum SymbolEntryKind {
    SymbolEntryInvalid,
    SymbolEntryVar,
    SymbolEntryFun,
    SymbolEntryStruct,
} SymbolEntryKind;

typedef struct SymbolEntry {
    SymbolEntryKind kind;
    usize type_idx;

    // The following things shall be manually set
    struct SymbolTable *table; // points to the table that contains this entry
    String *name;              // points to the key in the symbol table
    union {
        struct {
            bool is_defined;
            int first_decl_line_no;
        } as_fun;
    };
} SymbolEntry;

FUNC_STATIC void MTD(SymbolEntry, init, /, SymbolEntryKind kind,
                     usize type_idx) {
    self->kind = kind;
    self->type_idx = type_idx;
}

FUNC_STATIC DEFAULT_DROPER(SymbolEntry);

DELETED_CLONER(SymbolEntry, FUNC_STATIC);

/* SymbolTable */

DECLARE_MAPPING(MapSymtab, HString, SymbolEntry, FUNC_EXTERN,
                GENERATOR_CLASS_KEY, GENERATOR_CLASS_VALUE,
                GENERATOR_CLASS_COMPARATOR);

#define SYMBOL_TABLE_NO_PARENT ((usize)(-1))

typedef struct SymbolTable {
    usize parent_idx;
    MapSymtab mapping;
} SymbolTable;

void MTD(SymbolTable, init, /, usize parent_idx);
void MTD(SymbolTable, drop, /);
DELETED_CLONER(SymbolTable, FUNC_STATIC);

bool MTD(SymbolTable, is_root, /);
MapSymtabInsertResult MTD(SymbolTable, insert, /, HString name,
                          SymbolEntryKind kind, usize type_idx);

MapSymtabIterator MTD(SymbolTable, find, /, HString *name);

struct SymbolManager;

MapSymtabIterator MTD(SymbolTable, find_recursive, /, HString *name,
                      struct SymbolManager *manager);

/* SymbolManager */

DECLARE_CLASS_VEC(VecSymbolTable, SymbolTable, FUNC_EXTERN);

typedef struct SymbolManager {
    VecSymbolTable tables;
    usize root_idx;
    usize temp_cnt;
} SymbolManager;

void MTD(SymbolManager, init, /);
void MTD(SymbolManager, drop, /);
usize MTD(SymbolManager, add_table, /, usize parent_idx);
SymbolTable *MTD(SymbolManager, get_table, /, usize idx);
HString MTD(SymbolManager, make_temp, /);
