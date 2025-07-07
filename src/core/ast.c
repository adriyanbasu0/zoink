#include "ast.h"
#include <stdlib.h> // For malloc, free
#include <stdbool.h> // For bool (used in function signature)

//------------------------------------------------------------------------------
// Expression Node Constructor Functions
//------------------------------------------------------------------------------

Expr* ast_expr_literal_create(Token literal) {
    ExprLiteral* expr = (ExprLiteral*)malloc(sizeof(ExprLiteral));
    if (!expr) return NULL;
    expr->base.type = EXPR_LITERAL;
    expr->literal = literal; // Token is copied by value
    return (Expr*)expr;
}

Expr* ast_expr_variable_create(Token name) {
    ExprVariable* expr = (ExprVariable*)malloc(sizeof(ExprVariable));
    if (!expr) return NULL;
    expr->base.type = EXPR_VARIABLE;
    expr->name = name; // Token is copied by value
    return (Expr*)expr;
}

Expr* ast_expr_call_create(Expr* callee, DynamicArray* arguments, Token closing_paren) {
    ExprCall* expr = (ExprCall*)malloc(sizeof(ExprCall));
    if (!expr) return NULL;
    expr->base.type = EXPR_CALL;
    expr->callee = callee; // Ownership assumed by ExprCall
    expr->arguments = arguments; // Ownership assumed by ExprCall
    expr->closing_paren = closing_paren;
    return (Expr*)expr;
}

//------------------------------------------------------------------------------
// Statement Node Constructor Functions
//------------------------------------------------------------------------------

Stmt* ast_stmt_let_create(Token name, bool is_mutable, Expr* initializer) {
    StmtLet* stmt = (StmtLet*)malloc(sizeof(StmtLet));
    if (!stmt) return NULL;
    stmt->base.type = STMT_LET;
    stmt->name = name; // Token copied by value
    stmt->is_mutable = is_mutable;
    stmt->initializer = initializer; // Ownership assumed by StmtLet
    return (Stmt*)stmt;
}

ADTVariantField* ast_adt_variant_field_create(Token name, Token type_name_token) {
    ADTVariantField* field = (ADTVariantField*)malloc(sizeof(ADTVariantField));
    if (!field) return NULL;
    field->name = name; // Optional, token copied
    field->type_name_token = type_name_token; // Token copied
    return field;
}

ADTVariant* ast_adt_variant_create(Token name, DynamicArray* fields) {
    ADTVariant* variant = (ADTVariant*)malloc(sizeof(ADTVariant));
    if (!variant) return NULL;
    variant->name = name; // Token copied
    variant->fields = fields; // Ownership of DA and its ADTVariantField* elements assumed
    return variant;
}

Stmt* ast_stmt_data_create(Token name, DynamicArray* type_params, DynamicArray* variants) {
    StmtData* stmt = (StmtData*)malloc(sizeof(StmtData));
    if (!stmt) return NULL;
    stmt->base.type = STMT_DATA;
    stmt->name = name; // Token copied
    stmt->type_params = type_params; // Ownership of DA and its Token* elements assumed (if Tokens are heap allocated)
                                     // For now, Tokens are structs, so DA owns copies if configured, or pointers.
                                     // Let's assume DA stores Token pointers, and those Tokens are managed elsewhere or copied if needed.
                                     // For simplicity, if type_params stores Token structs directly, DA handles them.
                                     // If it stores Token*, parser needs to manage memory of those Token* if they are heap.
                                     // Given Token is a struct, DynamicArray probably stores copies or pointers to parts of source.
                                     // Let's assume type_params stores Token (copied by value in DA if DA supports it, or pointers to source tokens).
                                     // For ADTVariant*, the variants DA owns the ADTVariant pointers.
    stmt->variants = variants;       // Ownership of DA and its ADTVariant* elements assumed
    return (Stmt*)stmt;
}

Program* ast_program_create(DynamicArray* statements) {
    Program* program = (Program*)malloc(sizeof(Program));
    if (!program) return NULL;
    program->statements = statements; // Ownership of DA and its Stmt* elements assumed
    return program;
}


//------------------------------------------------------------------------------
// AST Node Destructor Functions (Recursive)
//------------------------------------------------------------------------------

// Forward declare variant/field destructors if they are complex
static void ast_adt_variant_field_destroy(ADTVariantField* field) {
    if (!field) return;
    // Tokens are not heap-allocated themselves (lexeme points to source).
    // If type_name_token's lexeme were copied, it would be freed here.
    free(field);
}

static void ast_adt_variant_destroy(ADTVariant* variant) {
    if (!variant) return;
    if (variant->fields) {
        for (size_t i = 0; i < da_count(variant->fields); ++i) {
            ast_adt_variant_field_destroy((ADTVariantField*)da_get(variant->fields, i));
        }
        da_destroy(variant->fields);
    }
    free(variant);
}


void ast_expr_destroy(Expr* expr) {
    if (!expr) return;

    switch (expr->type) {
        case EXPR_LITERAL:
            // ExprLiteral* lit_expr = (ExprLiteral*)expr;
            // If literal.lexeme or a parsed value (e.g. string_val) was heap allocated, free it here.
            // Tokens themselves are structs, lexemes point to source.
            break;
        case EXPR_VARIABLE:
            // ExprVariable* var_expr = (ExprVariable*)expr;
            // Token name is a struct.
            break;
        case EXPR_CALL: {
            ExprCall* call_expr = (ExprCall*)expr;
            ast_expr_destroy(call_expr->callee);
            if (call_expr->arguments) {
                for (size_t i = 0; i < da_count(call_expr->arguments); ++i) {
                    ast_expr_destroy((Expr*)da_get(call_expr->arguments, i));
                }
                da_destroy(call_expr->arguments);
            }
            break;
        }
        // Add other expression types
        default:
            // Should not happen if all types are handled
            break;
    }
    free(expr);
}

void ast_stmt_destroy(Stmt* stmt) {
    if (!stmt) return;

    switch (stmt->type) {
        case STMT_LET: {
            StmtLet* let_stmt = (StmtLet*)stmt;
            if (let_stmt->initializer) {
                ast_expr_destroy(let_stmt->initializer);
            }
            // Token name is a struct.
            break;
        }
        case STMT_DATA: {
            StmtData* data_stmt = (StmtData*)stmt;
            // Type parameters (Tokens) are now stored as Token* (pointers to heap-allocated Tokens)
            if (data_stmt->type_params) {
                for (size_t i = 0; i < da_count(data_stmt->type_params); ++i) {
                    free(da_get(data_stmt->type_params, i)); // Free each Token*
                }
                da_destroy(data_stmt->type_params);
            }
            if (data_stmt->variants) {
                for (size_t i = 0; i < da_count(data_stmt->variants); ++i) {
                    ast_adt_variant_destroy((ADTVariant*)da_get(data_stmt->variants, i));
                }
                da_destroy(data_stmt->variants);
            }
            break;
        }
        case STMT_EXPRESSION: {
            // If StmtExpression has an Expr field, cast and destroy it.
            // Example: StmtExpression* expr_stmt = (StmtExpression*)stmt; ast_expr_destroy(expr_stmt->expression);
            break;
        }
        // Add other statement types
        default:
            break;
    }
    free(stmt);
}

void ast_program_destroy(Program* program) {
    if (!program) return;
    if (program->statements) {
        for (size_t i = 0; i < da_count(program->statements); ++i) {
            ast_stmt_destroy((Stmt*)da_get(program->statements, i));
        }
        da_destroy(program->statements);
    }
    free(program);
}
