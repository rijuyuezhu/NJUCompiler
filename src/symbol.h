#pragma once
#include "str.h"
#include "tem_map.h"
#include "tem_vec.h"
#include "utils.h"

struct SymbolManager;

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
    usize offset;

    // The following members shall be manually set

    struct SymbolTable *table; // points to the table that contains this entry
    String *name;              // points to the key in the symbol table
    union {
        struct {
            bool is_defined;
            int first_decl_line_no;
        } as_fun;
        struct {
            usize ir_var_idx;
        } as_var;
    };
} SymbolEntry;

FUNC_STATIC void MTD(SymbolEntry, init, /, SymbolEntryKind kind, usize type_idx,
                     usize offset) {
    self->kind = kind;
    self->type_idx = type_idx;
    self->offset = offset;
}

FUNC_STATIC DEFAULT_DROPER(SymbolEntry);

DELETED_CLONER(SymbolEntry, FUNC_STATIC);

/* SymbolTable */

DECLARE_MAPPING(MapSymtab, HString, SymbolEntry, FUNC_EXTERN,
                GENERATOR_CLASS_KEY, GENERATOR_CLASS_VALUE,
                GENERATOR_CLASS_COMPARATOR);

#define SYMTAB_NO_PARENT ((usize)(-1))

typedef struct SymbolTable {
    usize parent_idx;
    MapSymtab mapping;
    usize offset_acc;
} SymbolTable;

void MTD(SymbolTable, init, /, usize parent_idx);
void MTD(SymbolTable, drop, /);
DELETED_CLONER(SymbolTable, FUNC_STATIC);

bool MTD(SymbolTable, is_root, /);
MapSymtabInsertResult MTD(SymbolTable, insert, /, HString name,
                          SymbolEntryKind kind, usize type_idx, usize width);

MapSymtabIterator MTD(SymbolTable, find, /, HString *name);

MapSymtabIterator MTD(SymbolTable, find_recursive, /, HString *name,
                      struct SymbolManager *manager);

/* SymbolManager */

DECLARE_CLASS_VEC(VecSymbolTable, SymbolTable, FUNC_EXTERN);

typedef struct SymbolManager {
    VecSymbolTable tables;
    usize root_idx;
    usize temp_cnt;

    SymbolEntry *read_fun;
    SymbolEntry *write_fun;
} SymbolManager;

void MTD(SymbolManager, init, /);
void MTD(SymbolManager, drop, /);
usize MTD(SymbolManager, add_table, /, usize parent_idx);
SymbolTable *MTD(SymbolManager, get_table, /, usize idx);
