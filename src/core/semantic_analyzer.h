#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"
#include "types.h"
#include <stdbool.h>

// Semantic Analyzer structure
// It will hold the state needed for semantic analysis, primarily the symbol table.
typedef struct {
    SymbolTable* sym_table;
    // We might add a reference to a list of predefined types (e.g., i32, String, bool)
    // Scope* predefined_types_scope;
    bool had_error;
    // DynamicArray* errors; // To store detailed error messages
} SemanticAnalyzer;

// Creates a new semantic analyzer.
SemanticAnalyzer* semantic_analyzer_create();

// Frees the semantic analyzer and its components (like the symbol table).
void semantic_analyzer_destroy(SemanticAnalyzer* analyzer);

// Analyzes the given AST program.
// Populates the symbol table, resolves types (initial pass), and performs checks.
// Returns true if analysis is successful (no errors), false otherwise.
// The AST nodes might be annotated with type information or symbol table references during this phase.
bool semantic_analyzer_analyze(SemanticAnalyzer* analyzer, Program* program);

// Helper function to get error status
bool semantic_analyzer_had_error(const SemanticAnalyzer* analyzer);

#endif // SEMANTIC_ANALYZER_H
