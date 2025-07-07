#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h> // For bool
#include "../util/dynamic_array.h" // For managing scopes or symbol lists
#include "token.h"   // For symbol names (Tokens)
#include "types.h"   // For Type* and ADTDefinition*

// Forward declaration for recursive type (Scope contains Symbols, Symbol might point to Scope for functions/modules)
struct Scope;

// Kind of symbol (variable, function, type, etc.)
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_TYPE_ALIAS,   // For `type MyInt = i32;` (future)
    SYMBOL_ADT,          // For `data Option<T> { ... }`
    SYMBOL_FUNCTION,     // For `fn my_func() { ... }` (future)
    SYMBOL_PARAMETER,    // For function parameters (future)
    SYMBOL_GENERIC_PARAM,// For type parameters like T in Option<T> (defined in Type system, but a symbol binds it)
    // Add more kinds as needed (e.g., MODULE, TRAIT/TYPECLASS)
} SymbolKind;

// Symbol structure
typedef struct Symbol {
    SymbolKind kind;
    Token name_token; // The token that defines this symbol's name
    Type* type;       // Resolved type of the symbol (e.g., type of a variable, or the ADT's own type)
    // struct Scope* defined_in_scope; // Pointer to the scope where this symbol is defined

    union {
        // For SYMBOL_VARIABLE, SYMBOL_PARAMETER:
        struct {
            // bool is_mutable;
            // bool is_used;
            // For ownership:
            // OwnershipState ownership_state; // e.g., OWNED, BORROWED_IMM, BORROWED_MUT
            // Lifetime lifetime_info;
        } var_info;

        // For SYMBOL_ADT:
        ADTDefinition* adt_def; // Contains type_params, variants, etc.

        // For SYMBOL_FUNCTION (future):
        // struct {
        //     DynamicArray* parameters; // List of Symbol* for parameters
        //     Type* return_type;
        //     struct Scope* function_scope; // Scope for the function body
        // } func_info;

        // For SYMBOL_TYPE_ALIAS (future):
        // Type* aliased_type;

        // For SYMBOL_GENERIC_PARAM (already part of TypeGenericParam, Type* above would point to it)
    } data;

    // Add other relevant info: declaration AST node, flags (is_public, is_used), etc.
    // struct AstNode* declaration_node; // Pointer to the AST node where this symbol was declared
} Symbol;


// Scope structure (for lexical scoping)
// A simple approach for now: each scope has a list of symbols and a pointer to its parent.
// A hash map would be more efficient for symbol lookup.
typedef struct Scope {
    struct Scope* parent;
    DynamicArray* symbols; // DynamicArray of Symbol*
    // We might use a hash map here for faster lookups: HashMap* symbol_map; (string name -> Symbol*)
    int depth; // Scope depth (0 for global, 1 for first level, etc.)
} Scope;


// --- Symbol Table API ---

// Symbol functions
Symbol* symbol_create(SymbolKind kind, Token name_token, Type* type);
void symbol_destroy(Symbol* symbol); // Must handle freeing data union contents appropriately

// Scope functions
Scope* scope_create(Scope* parent);
void scope_destroy(Scope* scope); // Frees symbols within this scope and the scope itself.

// Adds a symbol to the given scope.
// Returns true on success, false if symbol already exists in this scope (or other error).
bool scope_define(Scope* scope, Symbol* symbol);

// Looks up a symbol by name in the current scope and its parent scopes.
// Returns NULL if not found.
Symbol* scope_lookup(Scope* scope, Token name_token);

// Looks up a symbol only in the current scope.
// Returns NULL if not found.
Symbol* scope_lookup_current(Scope* scope, Token name_token);


// SymbolTable structure (manages all scopes, could be part of Analyzer state)
typedef struct {
    Scope* global_scope;
    Scope* current_scope;
    // Potentially a list of all allocated types for easier cleanup, or type interning table.
    // DynamicArray* all_types;
} SymbolTable;

SymbolTable* symbol_table_create();
void symbol_table_destroy(SymbolTable* table);

void symbol_table_enter_scope(SymbolTable* table);
void symbol_table_exit_scope(SymbolTable* table);

// Defines a symbol in the current scope of the table.
bool symbol_table_define(SymbolTable* table, Symbol* symbol);

// Looks up a symbol in the current scope chain of the table.
Symbol* symbol_table_lookup(SymbolTable* table, Token name_token);

// Looks up a symbol only in the immediate current scope of the table.
Symbol* symbol_table_lookup_current(SymbolTable* table, Token name_token);


#endif // SYMBOL_TABLE_H
