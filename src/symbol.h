#pragma once
#include "general_vec.h"
#include "str.h"
#include "tem_map.h"
#include "tem_vec.h"
#include "utils.h"

/* SymbolEntry */

typedef enum SymbolKind {
    SymbolKindInvalid,
    SymbolKindVar,
    SymbolKindFun,
    SymbolKindStruct,
} SymbolKind;

typedef struct SymbolEntry {
    SymbolKind kind;
    struct SymbolTable *table; // points to the table that contains this entry
    String *name;              // points to the key in the symbol table
    union {
        struct {
            usize type_idx;
        } as_var;
        struct {
            VecUSize type_idxs;
            // the 0-th is the return type; 1 + #param == type_ids.size
        } as_fun;
        struct {
            usize type_idx;
        } as_struct;
    };
} SymbolEntry;

void MTD(SymbolEntry, drop, /);
DELETED_CLONER(SymbolEntry, FUNC_STATIC);

SymbolEntry NSMTD(SymbolEntry, make_var, /, struct SymbolTable *table,
                  String *name, usize type_idx);
SymbolEntry NSMTD(SymbolEntry, make_fun, /, struct SymbolTable *table,
                  String *name);
void MTD(SymbolEntry, fun_add, /, usize type_idx);
SymbolEntry NSMTD(SymbolEntry, make_struct, /, struct SymbolTable *table,
                  String *name, usize type_idx);

/* SymbolTable */

DECLARE_MAPPING(MapSymbolTable, HString, SymbolEntry, FUNC_EXTERN,
                GENERATOR_CLASS_KEY, GENERATOR_CLASS_VALUE,
                GENERATOR_CLASS_COMPARATOR);

#define SYMBOL_TABLE_NO_PARENT ((usize) - 1)
typedef struct SymbolTable {
    usize parent_idx;
    MapSymbolTable mapping;
} SymbolTable;

void MTD(SymbolTable, init, /, usize parent_idx);
void MTD(SymbolTable, drop, /);
DELETED_CLONER(SymbolTable, FUNC_STATIC);

MapSymbolTableInsertResult MTD(SymbolTable, insert, /, HString name,
                               SymbolEntry entry);
MapSymbolTableIterator MTD(SymbolTable, find, /, HString *name);
struct SymbolManager;
MapSymbolTableIterator MTD(SymbolTable, find_recursive, /, HString *name,
                           struct SymbolManager *manager);

/* SymbolManager */

DECLARE_CLASS_VEC(VecSymbolTable, SymbolTable, FUNC_EXTERN);

typedef struct SymbolManager {
    VecSymbolTable tables;
    usize root_idx;
} SymbolManager;

void MTD(SymbolManager, init, /);
void MTD(SymbolManager, drop, /);
usize MTD(SymbolManager, add_table, /, usize parent_idx);
