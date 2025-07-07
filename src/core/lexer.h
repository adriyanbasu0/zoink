#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h> // For bool type
#include "token.h"
#include "../util/dynamic_array.h" // To store tokens

// Lexer structure
typedef struct {
    const char *source; // Pointer to the source code string
    const char *current; // Pointer to the current character being scanned
    int line;           // Current line number
    int col;            // Current column number (approximate, start of token)
    DynamicArray *tokens; // Dynamic array to store scanned tokens
    // Potentially add a filename field for error reporting
    // const char* filename;
} Lexer;

// Initializes a new lexer with the given source code.
// The lexer does NOT take ownership of the source string; it must remain valid
// for the lifetime of the lexer or until scanning is complete.
Lexer* lexer_create(const char *source);

// Frees the lexer and its associated resources (like the tokens array).
// Does NOT free the source string.
void lexer_destroy(Lexer *lexer);

// Scans all tokens from the source code and stores them in the lexer's token list.
// Returns true on success, false if any lexical errors were encountered.
// After this call, tokens can be retrieved from lexer->tokens.
bool lexer_scan_tokens(Lexer *lexer);

// Helper function to get all scanned tokens.
// The returned DynamicArray is owned by the lexer.
DynamicArray* lexer_get_tokens(Lexer *lexer);

// (Optional) A function to scan and return the next token individually,
// if a streaming lexer approach is desired later.
// Token lexer_next_token(Lexer *lexer);

#endif // LEXER_H
