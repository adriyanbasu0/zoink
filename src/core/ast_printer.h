#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "ast.h"
#include <stdio.h> // For FILE*

// Prints the Program AST to the given output stream (e.g., stdout).
void ast_print_program(Program *program, FILE *stream);

// Individual statement printers (can be static in .c if not needed externally)
void ast_print_stmt(Stmt *stmt, FILE *stream, int indent_level);
void ast_print_expr(Expr *expr, FILE *stream); // Simplified for now

#endif // AST_PRINTER_H
