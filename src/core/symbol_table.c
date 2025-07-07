#include "symbol_table.h"
#include "types.h" // For ADTDefinition, Type destruction
#include <stdlib.h>
#include <string.h> // For strncmp
#include <stdbool.h> // For bool, true, false

// --- Symbol Functions ---

Symbol* symbol_create(SymbolKind kind, Token name_token, Type* type) {
    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    if (!symbol) return NULL;
    symbol->kind = kind;
    symbol->name_token = name_token; // Token struct copied
    symbol->type = type; // Assumes ownership of type if this symbol is its primary definition point
                         // Or type might be shared (e.g. primitive types). Careful with type_destroy.
                         // For SYMBOL_ADT, type would be the ADT's self-referential type, adt_def holds structure.

    // Initialize union data based on kind if necessary (e.g., set pointers to NULL)
    if (kind == SYMBOL_ADT) {
        symbol->data.adt_def = NULL; // To be filled later
    }
    // Other kinds might need similar initialization for their specific data.
    return symbol;
}

void symbol_destroy(Symbol* symbol) {
    if (!symbol) return;

    // Important: Decide on Type ownership.
    // If a symbol uniquely "owns" its Type definition (e.g. a complex ADT type constructed for it),
    // then type_destroy(symbol->type) should be called here.
    // If types are interned or shared (like primitives), then don't free here.
    // For now, let's assume types associated with symbols are owned if complex, or managed globally if simple.
    // A common pattern: type_destroy is called by whatever allocated the Type.
    // If symbol_create also creates a new Type for the symbol, it should destroy it.
    // If type is passed in and potentially shared, don't destroy here.
    // Let's assume for now `symbol->type` is owned by the symbol, UNLESS it's a predefined type.

    // Access predefined types (needs to be visible here, e.g. via extern or passed in)
    // For now, this is a simplification. A better way would be to have a type_is_singleton() check.
    // This requires types.c to expose these or a helper function.
    // Hacky direct comparison for now, assuming these are accessible (they are not directly from here).
    // If a symbol's type is one of the global predefined types, we should not destroy it here,
    // as their lifecycle is managed by types_init_predefined and types_cleanup_predefined.
    if (!type_is_predefined(symbol->type)) {
        type_destroy(symbol->type);
    }
    // else: it's a predefined type, types_cleanup_predefined() will handle it.


    if (symbol->kind == SYMBOL_ADT) {
        adt_definition_destroy(symbol->data.adt_def); // This will free type_params and variants.
    }
    // Add cleanup for other symbol kinds as they are developed (e.g., func_info)

    free(symbol);
}

// --- Scope Functions ---

Scope* scope_create(Scope* parent) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    if (!scope) return NULL;
    scope->parent = parent;
    scope->symbols = da_create(8, sizeof(Symbol*)); // Stores Symbol*
    if (!scope->symbols) {
        free(scope);
        return NULL;
    }
    scope->depth = parent ? parent->depth + 1 : 0;
    return scope;
}

void scope_destroy(Scope* scope) {
    if (!scope) return;
    if (scope->symbols) {
        for (size_t i = 0; i < da_count(scope->symbols); ++i) {
            symbol_destroy((Symbol*)da_get(scope->symbols, i));
        }
        da_destroy(scope->symbols);
    }
    free(scope);
}

bool scope_define(Scope* scope, Symbol* symbol) {
    if (!scope || !symbol) return false;
    // Check for redefinition in the current scope only
    for (size_t i = 0; i < da_count(scope->symbols); ++i) {
        Symbol* existing = (Symbol*)da_get(scope->symbols, i);
        if (existing->name_token.length == symbol->name_token.length &&
            strncmp(existing->name_token.lexeme, symbol->name_token.lexeme, symbol->name_token.length) == 0) {
            // Symbol already defined in this scope
            return false;
        }
    }
    return da_push(scope->symbols, symbol) == 0;
}

Symbol* scope_lookup(Scope* scope, Token name_token) {
    Scope* current = scope;
    while (current) {
        Symbol* symbol = scope_lookup_current(current, name_token);
        if (symbol) return symbol;
        current = current->parent;
    }
    return NULL;
}

Symbol* scope_lookup_current(Scope* scope, Token name_token) {
    if (!scope) return NULL;
    for (size_t i = 0; i < da_count(scope->symbols); ++i) {
        Symbol* symbol = (Symbol*)da_get(scope->symbols, i);
        if (symbol->name_token.length == name_token.length &&
            strncmp(symbol->name_token.lexeme, name_token.lexeme, name_token.length) == 0) {
            return symbol;
        }
    }
    return NULL;
}


// --- SymbolTable Functions ---

SymbolTable* symbol_table_create() {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!table) return NULL;
    table->global_scope = scope_create(NULL); // Global scope has no parent
    if (!table->global_scope) {
        free(table);
        return NULL;
    }
    table->current_scope = table->global_scope;
    return table;
}

void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;
    // Destroying global_scope will recursively destroy child scopes if that logic is added,
    // or we need to manage the scope stack carefully.
    // For now, SymbolTable only directly owns global_scope.
    // If scopes are pushed/popped, current_scope should point to a scope that will be
    // part of the global_scope's parent chain or global_scope itself.
    scope_destroy(table->global_scope); // This will free all symbols defined within.
    free(table);
}

void symbol_table_enter_scope(SymbolTable* table) {
    if (!table) return;
    Scope* new_scope = scope_create(table->current_scope);
    if (new_scope) { // Handle allocation failure if scope_create can fail
        table->current_scope = new_scope;
    }
    // Else: error, couldn't create new scope. Should probably signal this.
}

void symbol_table_exit_scope(SymbolTable* table) {
    if (!table || !table->current_scope) return;
    if (table->current_scope->parent) { // Cannot exit global scope this way
        Scope* old_scope = table->current_scope;
        table->current_scope = table->current_scope->parent;
        scope_destroy(old_scope); // Destroy the scope we are exiting
    }
    // Else: error or trying to exit global scope, handle as appropriate.
}

bool symbol_table_define(SymbolTable* table, Symbol* symbol) {
    if (!table || !table->current_scope) return false;
    return scope_define(table->current_scope, symbol);
}

Symbol* symbol_table_lookup(SymbolTable* table, Token name_token) {
    if (!table || !table->current_scope) return NULL;
    return scope_lookup(table->current_scope, name_token);
}

Symbol* symbol_table_lookup_current(SymbolTable* table, Token name_token) {
    if (!table || !table->current_scope) return NULL;
    return scope_lookup_current(table->current_scope, name_token);
}
