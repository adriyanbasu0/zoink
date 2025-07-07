#ifndef PARSER_H
#define PARSER_H

#include "token.h"    // For Token
#include "ast.h"      // For Program, Stmt, Expr etc.
#include "../util/dynamic_array.h" // For Lexer's token list

// Parser structure
typedef struct {
    DynamicArray *tokens; // List of tokens from the lexer (not owned by parser)
    int current;          // Index of the current token being processed
    bool had_error;       // Flag to indicate if any parsing errors occurred
    // We can add a DynamicArray here to store error messages if needed.
    // DynamicArray* errors;
} Parser;

// Initializes a new parser with a list of tokens.
// The parser does not take ownership of the tokens array.
Parser* parser_create(DynamicArray *tokens);

// Frees the parser structure.
// Does not free the tokens array itself, as it's owned by the lexer or caller.
void parser_destroy(Parser *parser);

// Parses the tokens and returns the root AST node (e.g., a Program node).
// Returns NULL if parsing fails and sets parser->had_error.
// The caller is responsible for freeing the returned AST using ast_program_destroy or similar.
Program* parser_parse(Parser *parser);

// Returns true if any parsing errors were encountered.
bool parser_had_error(const Parser *parser);

#endif // PARSER_H
