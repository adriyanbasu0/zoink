#include "semantic_analyzer.h"
#include "ast.h"
#include "symbol_table.h"
#include "types.h"
#include "token.h" // For TOKEN_INTEGER, TOKEN_STRING
#include <stdio.h> // For fprintf, stderr for errors
#include <stdlib.h> // For malloc, free
#include <string.h> // For strncmp

// Forward declarations for AST traversal functions
static void analyze_stmt(SemanticAnalyzer* analyzer, Stmt* stmt);
static void analyze_expr(SemanticAnalyzer* analyzer, Expr* expr); // Basic version for now

// --- Error Reporting ---
static void semantic_error_at_token(SemanticAnalyzer* analyzer, Token token, const char* message) {
    analyzer->had_error = true;
    fprintf(stderr, "[L%d C%d at '%.*s'] Semantic Error: %s\n",
            token.line, token.col, (int)token.length, token.lexeme, message);
}

static void semantic_error_general(SemanticAnalyzer* analyzer, const char* message) {
    analyzer->had_error = true;
    fprintf(stderr, "Semantic Error: %s\n", message);
}


// --- Helper Functions ---

// Resolves a type name token to a Type object.
// Checks ADT's own generic parameters, then predefined types, then the symbol table.
// - adt_generic_params: DynamicArray of TypeGenericParam* for the ADT being defined.
// Returns a Type*. If a new type is allocated (e.g. TypeADT, TypeError), it's owned by the caller.
// If a shared type is returned (generic param, predefined), it's not.
static Type* resolve_type_for_token(SemanticAnalyzer* analyzer, Token type_name_token, DynamicArray* adt_generic_params) {
    // 1. Check ADT's own generic parameters
    if (adt_generic_params) {
        for (size_t i = 0; i < da_count(adt_generic_params); ++i) {
            TypeGenericParam* gp_type = (TypeGenericParam*)da_get(adt_generic_params, i);
            if (gp_type->name.length == type_name_token.length &&
                strncmp(gp_type->name.lexeme, type_name_token.lexeme, type_name_token.length) == 0) {
                return (Type*)gp_type; // Found generic parameter, return shared Type*
            }
        }
    }

    // 2. Check predefined types
    // TODO: Make this more robust, perhaps a helper in types.c or a map if many predefined types
    if (type_name_token.length == 3 && strncmp(type_name_token.lexeme, "i32", 3) == 0) {
        return type_i32_instance; // Shared predefined type
    }
    if (type_name_token.length == 6 && strncmp(type_name_token.lexeme, "String", 6) == 0) {
        return type_string_instance; // Shared predefined type
    }
    if (type_name_token.length == 4 && strncmp(type_name_token.lexeme, "bool", 4) == 0) {
        return type_bool_instance; // Shared predefined type
    }
    // Add other predefined types like 'void' if necessary, though 'void' fields are unusual.

    // 3. Look up in symbol table
    Symbol* found_symbol = symbol_table_lookup(analyzer->sym_table, type_name_token);
    if (found_symbol) {
        if (found_symbol->kind == SYMBOL_ADT) {
            // It's another ADT. Create a TypeADT instance for it.
            // For now, type_args is NULL (or empty DA). Handling generic ADTs as fields is more complex.
            // The created TypeADT will be owned by the caller (specifically, the ADTFieldSymbol).
            return type_adt_create(type_name_token, NULL, found_symbol);
        } else {
            // Found a symbol, but it's not a type (e.g., a variable or function used as a type)
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg),
                     "Expected a type name, but '%.*s' refers to a %s.",
                     (int)type_name_token.length, type_name_token.lexeme,
                     // TODO: Add a symbol_kind_to_string helper for better error messages
                     found_symbol->kind == SYMBOL_VARIABLE ? "variable" : "non-type symbol");
            semantic_error_at_token(analyzer, type_name_token, err_msg);
            return type_error_create(); // Caller owns this
        }
    } else {
        // Type name not found anywhere
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Unknown type name '%.*s'.", (int)type_name_token.length, type_name_token.lexeme);
        semantic_error_at_token(analyzer, type_name_token, err_msg);
        return type_error_create(); // Caller owns this
    }
}


// --- Analysis of AST Nodes ---

static void analyze_stmt_data(SemanticAnalyzer* analyzer, StmtData* stmt) {
    // 1. Check if ADT name is already defined in the current scope.
    Symbol* existing_symbol = symbol_table_lookup_current(analyzer->sym_table, stmt->name);
    if (existing_symbol) {
        semantic_error_at_token(analyzer, stmt->name, "ADT with this name already defined in the current scope.");
        // Potentially don't define this ADT to avoid further cascading errors related to it.
        return;
    }

    // 2. Create Type objects for generic parameters (if any) and store them temporarily.
    //    These are not added to the main symbol table here but are part of ADTDefinition.
    DynamicArray* generic_param_types = da_create(da_count(stmt->type_params), sizeof(Type*));
    if (stmt->type_params) {
        for (size_t i = 0; i < da_count(stmt->type_params); ++i) {
            Token* param_token = (Token*)da_get(stmt->type_params, i); // These are Token* from parser
            // TODO: Check for duplicate type parameter names within this ADT's definition.
            Type* generic_type = type_generic_param_create(*param_token);
            da_push(generic_param_types, generic_type);
        }
    }

    // 3. Create ADTDefinition structure (for symbol table)
    //    This involves converting AST ADTVariants/Fields to ADTVariantSymbol/FieldSymbol
    //    and resolving field types (placeholder for now, will be more complex).
    DynamicArray* variant_symbols = da_create(da_count(stmt->variants), sizeof(ADTVariantSymbol*));
    for (size_t i = 0; i < da_count(stmt->variants); ++i) {
        ADTVariant* ast_variant = (ADTVariant*)da_get(stmt->variants, i);
        // TODO: Check for duplicate variant names within this ADT.

        DynamicArray* field_symbols = NULL;
        if (ast_variant->fields && da_count(ast_variant->fields) > 0) {
            field_symbols = da_create(da_count(ast_variant->fields), sizeof(ADTFieldSymbol*));
            for (size_t j = 0; j < da_count(ast_variant->fields); ++j) {
                ADTVariantField* ast_field = (ADTVariantField*)da_get(ast_variant->fields, j);
                Type* field_type = resolve_type_for_token(analyzer, ast_field->type_name_token, generic_param_types);
                // resolve_type_for_token returns a type that ADTFieldSymbol will take ownership of if it's new,
                // or it's a shared type (generic param, predefined) that doesn't need deep freeing by ADTFieldSymbol.
                // If field_type is an error type, it will also be owned and correctly freed.

                ADTFieldSymbol* field_sym = adt_field_symbol_create(ast_field->name, field_type);
                da_push(field_symbols, field_sym);
            }
        }
        ADTVariantSymbol* var_sym = adt_variant_symbol_create(ast_variant->name, field_symbols);
        da_push(variant_symbols, var_sym);
    }

    ADTDefinition* adt_def = adt_definition_create(stmt->name, generic_param_types, variant_symbols);

    // 4. Create the main Type for the ADT itself (e.g., Option<T> becomes a TypeADT instance)
    //    This self-referential type might be tricky. The TypeADT would have its type_args
    //    pointing to the TypeGenericParams created in step 2.
    //    For now, let's make a simpler TypeADT that just refers to the definition by name.
    //    A more complete TypeADT would involve cloning generic_param_types for its type_args.
    Type* adt_self_type = type_adt_create(stmt->name, da_create(0, sizeof(Type*)) /* no concrete args here */, NULL);
    // The adt_symbol field in TypeADT will be set after the symbol is created.

    Symbol* adt_symbol = symbol_create(SYMBOL_ADT, stmt->name, adt_self_type);
    adt_symbol->data.adt_def = adt_def;
    ((TypeADT*)adt_self_type)->adt_symbol = adt_symbol; // Link back from type to symbol

    if (!symbol_table_define(analyzer->sym_table, adt_symbol)) {
        // This should have been caught by lookup_current, but as a safeguard:
        semantic_error_at_token(analyzer, stmt->name, "Failed to define ADT symbol (should be caught earlier).");
        symbol_destroy(adt_symbol); // adt_symbol owns its type and adt_def
    } else {
        // Successfully defined.
        // The adt_def now owns generic_param_types and variant_symbols.
        // The adt_symbol now owns adt_self_type and adt_def.
    }
}

static void analyze_stmt_let(SemanticAnalyzer* analyzer, StmtLet* stmt) {
    // 1. Check if variable name is already defined in the current scope.
    if (symbol_table_lookup_current(analyzer->sym_table, stmt->name)) {
        semantic_error_at_token(analyzer, stmt->name, "Variable with this name already defined in current scope.");
        // Don't define this variable to avoid further errors.
        // If there was an initializer, it still needs to be analyzed for its own errors/type.
        if (stmt->initializer) analyze_expr(analyzer, stmt->initializer);
        return;
    }

    Type* var_type = type_unknown_create(); // Default to unknown

    if (stmt->initializer) {
        analyze_expr(analyzer, stmt->initializer); // Analyze initializer first

        // Infer type from initializer (very basic for Phase 1)
        // In a full system, expr analysis would annotate the Expr node with its type.
        // For now, directly inspect the initializer expression type.
        if (stmt->initializer->type == EXPR_LITERAL) {
            ExprLiteral* lit_expr = (ExprLiteral*)stmt->initializer;
            if (lit_expr->literal.type == TOKEN_INTEGER) {
                type_destroy(var_type); // Destroy the old unknown
                var_type = type_i32_instance; // Use global predefined i32 type
            } else if (lit_expr->literal.type == TOKEN_STRING) {
                type_destroy(var_type);
                var_type = type_string_instance; // Use global predefined String type
            }
            // Add other literals like bool, float later
        } else if (stmt->initializer->type == EXPR_VARIABLE) {
            // ExprVariable* var_expr_init = (ExprVariable*)stmt->initializer;
            // Symbol* init_sym = symbol_table_lookup(analyzer->sym_table, var_expr_init->name);
            // if (init_sym && init_sym->type) {
            //     type_destroy(var_type);
            //     var_type = type_clone(init_sym->type); // Need type_clone if types are complex and owned
            //                                        // For shared primitives, direct assignment is fine.
            // } else {
            //     semantic_error_at_token(analyzer, var_expr_init->name, "Cannot infer type from undefined variable.");
            // }
            // For Phase 1, let's assume if it's a variable, its type is already known or this is complex.
            // We are not doing full type inference yet, so this part is simplified.
            // The type remains <unknown> if not a simple literal.
        }
        // ADT instantiation as initializer is not handled yet for type inference.
    }
    // TODO: Handle explicit type annotations on `let` if syntax allows.

    Symbol* var_symbol = symbol_create(SYMBOL_VARIABLE, stmt->name, var_type);
    // var_symbol->data.var_info.is_mutable = stmt->is_mutable;

    if (!symbol_table_define(analyzer->sym_table, var_symbol)) {
        // Should be caught by lookup_current, but safeguard.
        semantic_error_at_token(analyzer, stmt->name, "Failed to define variable symbol.");
        symbol_destroy(var_symbol); // var_symbol owns its type
    } else {
         // If var_type was a shared primitive, symbol_destroy won't double-free.
         // If it was a newly allocated <unknown>, symbol_destroy will handle it.
    }
}


static void analyze_stmt(SemanticAnalyzer* analyzer, Stmt* stmt) {
    if (!stmt) return;
    switch (stmt->type) {
        case STMT_DATA:
            analyze_stmt_data(analyzer, (StmtData*)stmt);
            break;
        case STMT_LET:
            analyze_stmt_let(analyzer, (StmtLet*)stmt);
            break;
        // Other statements like STMT_EXPRESSION, STMT_IF, etc.
        default:
            // Should not happen if parser produces valid StmtTypes
            break;
    }
}

// Basic expression analysis (placeholder for Phase 1, mostly for initializers)
static void analyze_expr(SemanticAnalyzer* analyzer, Expr* expr) {
    if (!expr) return;
    switch (expr->type) {
        case EXPR_LITERAL:
            // No specific analysis for literals in Phase 1 beyond what `analyze_stmt_let` does.
            // Later, could validate literal format or attach precise type.
            break;
        case EXPR_VARIABLE: {
            // ExprVariable* var_expr = (ExprVariable*)expr;
            // Symbol* sym = symbol_table_lookup(analyzer->sym_table, var_expr->name);
            // if (!sym) {
            //     semantic_error_at_token(analyzer, var_expr->name, "Undefined variable.");
            // } else {
            //     // Annotate var_expr with sym->type or sym itself.
            // }
            break;
        }
        // Other expressions
        default:
            break;
    }
}


// --- Public API ---

SemanticAnalyzer* semantic_analyzer_create() {
    SemanticAnalyzer* analyzer = (SemanticAnalyzer*)malloc(sizeof(SemanticAnalyzer));
    if (!analyzer) return NULL;
    analyzer->sym_table = symbol_table_create();
    if (!analyzer->sym_table) {
        free(analyzer);
        return NULL;
    }
    analyzer->had_error = false;
    types_init_predefined(); // Initialize global predefined types
    return analyzer;
}

void semantic_analyzer_destroy(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    symbol_table_destroy(analyzer->sym_table);
    types_cleanup_predefined(); // Cleanup global predefined types
    free(analyzer);
}

bool semantic_analyzer_analyze(SemanticAnalyzer* analyzer, Program* program) {
    if (!analyzer || !program) {
        if (analyzer) analyzer->had_error = true;
        return false;
    }
    analyzer->had_error = false; // Reset error state for this run

    for (size_t i = 0; i < da_count(program->statements); ++i) {
        analyze_stmt(analyzer, (Stmt*)da_get(program->statements, i));
    }

    return !analyzer->had_error;
}

bool semantic_analyzer_had_error(const SemanticAnalyzer* analyzer) {
    return analyzer ? analyzer->had_error : true;
}
