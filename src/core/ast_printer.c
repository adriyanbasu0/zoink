#include "ast_printer.h"
#include "ast.h"
#include "token.h" // For token_type_to_string and accessing token data
#include <stdio.h>

static void print_indent(FILE *stream, int indent_level) {
    for (int i = 0; i < indent_level; ++i) {
        fprintf(stream, "  "); // Two spaces per indent level
    }
}

// Forward declare to handle mutual recursion if Expr can contain Stmt (e.g. block expressions)
// For now, Stmt contains Expr.
// void ast_print_expr(Expr *expr, FILE *stream); // Already in header

void ast_print_expr_internal(Expr *expr, FILE *stream, bool in_call) {
    (void)in_call; // Mark as unused for now to silence warning
    if (!expr) {
        fprintf(stream, "<null_expr>");
        return;
    }
    switch (expr->type) {
        case EXPR_LITERAL: {
            ExprLiteral *lit = (ExprLiteral*)expr;
            fprintf(stream, "%.*s", (int)lit->literal.length, lit->literal.lexeme);
            break;
        }
        case EXPR_VARIABLE: {
            ExprVariable *var = (ExprVariable*)expr;
            fprintf(stream, "%.*s", (int)var->name.length, var->name.lexeme);
            break;
        }
        case EXPR_CALL: {
            ExprCall* call = (ExprCall*)expr;
            ast_print_expr_internal(call->callee, stream, true);
            fprintf(stream, "(");
            for (size_t i = 0; i < da_count(call->arguments); ++i) {
                ast_print_expr_internal((Expr*)da_get(call->arguments, i), stream, false);
                if (i < da_count(call->arguments) - 1) {
                    fprintf(stream, ", ");
                }
            }
            fprintf(stream, ")");
            break;
        }
        // Add other expression types here
        default:
            fprintf(stream, "<unknown_expr_type:%d>", expr->type);
            break;
    }
}

void ast_print_expr(Expr *expr, FILE *stream) {
    ast_print_expr_internal(expr, stream, false);
}


void ast_print_stmt(Stmt *stmt, FILE *stream, int indent_level) {
    if (!stmt) {
        print_indent(stream, indent_level);
        fprintf(stream, "<null_stmt>\n");
        return;
    }

    print_indent(stream, indent_level);

    switch (stmt->type) {
        case STMT_LET: {
            StmtLet *let_stmt = (StmtLet*)stmt;
            fprintf(stream, "LET %s %.*s",
                    let_stmt->is_mutable ? "MUT" : "",
                    (int)let_stmt->name.length, let_stmt->name.lexeme);
            if (let_stmt->initializer) {
                fprintf(stream, " = ");
                ast_print_expr(let_stmt->initializer, stream);
            }
            fprintf(stream, ";\n");
            break;
        }
        case STMT_DATA: {
            StmtData *data_stmt = (StmtData*)stmt;
            fprintf(stream, "DATA %.*s", (int)data_stmt->name.length, data_stmt->name.lexeme);
            if (data_stmt->type_params && da_count(data_stmt->type_params) > 0) {
                fprintf(stream, "<");
                for (size_t i = 0; i < da_count(data_stmt->type_params); ++i) {
                    Token* param_token = (Token*)da_get(data_stmt->type_params, i);
                    fprintf(stream, "%.*s", (int)param_token->length, param_token->lexeme);
                    if (i < da_count(data_stmt->type_params) - 1) {
                        fprintf(stream, ", ");
                    }
                }
                fprintf(stream, ">");
            }
            fprintf(stream, " {\n");
            for (size_t i = 0; i < da_count(data_stmt->variants); ++i) {
                ADTVariant *variant = (ADTVariant*)da_get(data_stmt->variants, i);
                print_indent(stream, indent_level + 1);
                fprintf(stream, "%.*s", (int)variant->name.length, variant->name.lexeme);
                if (variant->fields && da_count(variant->fields) > 0) {
                    fprintf(stream, "(");
                    for (size_t j = 0; j < da_count(variant->fields); ++j) {
                        ADTVariantField *field = (ADTVariantField*)da_get(variant->fields, j);
                        // If field->name is present (struct-like variant), print it. Not for Phase 1.
                        fprintf(stream, "%.*s", (int)field->type_name_token.length, field->type_name_token.lexeme);
                        if (j < da_count(variant->fields) - 1) {
                            fprintf(stream, ", ");
                        }
                    }
                    fprintf(stream, ")");
                }
                if (i < da_count(data_stmt->variants) - 1) {
                     fprintf(stream, ",\n");
                } else {
                    fprintf(stream, "\n");
                }
            }
            print_indent(stream, indent_level);
            fprintf(stream, "}\n");
            break;
        }
        case STMT_EXPRESSION: {
            // Assuming StmtExpression struct has an 'expression' field of type Expr*
            // StmtExpression* expr_stmt = (StmtExpression*)stmt;
            // ast_print_expr(expr_stmt->expression, stream);
            // fprintf(stream, ";\n");
            fprintf(stream, "<expr_stmt_placeholder>;\n"); // Placeholder for now
            break;
        }
        default:
            fprintf(stream, "<unknown_stmt_type:%d>\n", stmt->type);
            break;
    }
}

void ast_print_program(Program *program, FILE *stream) {
    if (!program) {
        fprintf(stream, "<null_program>\n");
        return;
    }
    fprintf(stream, "PROGRAM:\n");
    for (size_t i = 0; i < da_count(program->statements); ++i) {
        ast_print_stmt((Stmt*)da_get(program->statements, i), stream, 1);
    }
}
