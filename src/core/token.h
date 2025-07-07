#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h> // For size_t

// Enum for all possible token types
typedef enum {
    // Special Tokens
    TOKEN_EOF,        // End of File/Input
    TOKEN_ERROR,      // Lexical error

    // Literals
    TOKEN_IDENTIFIER, // e.g., variable_name, function_name, TypeName
    TOKEN_INTEGER,    // e.g., 123, 0, 42
    TOKEN_FLOAT,      // e.g., 3.14, 0.5 (for future use, not in initial focus)
    TOKEN_STRING,     // e.g., "hello world"

    // Keywords (from design docs, plus common ones)
    TOKEN_DATA,       // `data`
    TOKEN_LET,        // `let`
    TOKEN_MUT,        // `mut` (for mutable bindings)
    TOKEN_MATCH,      // `match`
    TOKEN_FN,         // `fn` (for functions)
    TOKEN_RETURN,     // `return`
    TOKEN_IF,         // `if`
    TOKEN_ELSE,       // `else`
    TOKEN_TRUE,       // `true`
    TOKEN_FALSE,      // `false`
    TOKEN_TYPE,       // `type` (for type aliases, future)
    // Add more keywords as language evolves (e.g., for, while, struct, enum (if different from data), pub, etc.)

    // Operators
    TOKEN_PLUS,       // +
    TOKEN_MINUS,      // -
    TOKEN_ASTERISK,   // *
    TOKEN_SLASH,      // /
    TOKEN_PERCENT,    // %
    TOKEN_ASSIGN,     // =
    TOKEN_EQUAL,      // ==
    TOKEN_NOT_EQUAL,  // !=
    TOKEN_LESS,       // <
    TOKEN_LESS_EQUAL, // <=
    TOKEN_GREATER,    // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_AND,        // && (logical and)
    TOKEN_OR,         // || (logical or)
    TOKEN_NOT,        // !  (logical not)
    TOKEN_AMPERSAND,  // & (bitwise AND, or reference)
    // TOKEN_PIPE,    // | (bitwise OR, or ADT variant separator in type def) - handled by context?
    // TOKEN_CARET,   // ^ (bitwise XOR)
    // TOKEN_TILDE,   // ~ (bitwise NOT)
    // TOKEN_LSHIFT,  // <<
    // TOKEN_RSHIFT,  // >>

    // Punctuation / Delimiters
    TOKEN_LPAREN,     // (
    TOKEN_RPAREN,     // )
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LBRACKET,   // [ (for arrays/slices, future)
    TOKEN_RBRACKET,   // ]
    TOKEN_COMMA,      // ,
    TOKEN_DOT,        // .
    TOKEN_COLON,      // :
    TOKEN_SEMICOLON,  // ;
    TOKEN_ARROW,      // => (for match arms, lambda syntax)
    TOKEN_PIPE,       // | (used in match for OR patterns, or ADT variants in `data` block) - name carefully

    // Add other tokens as needed
} TokenType;

// Structure to represent a token
typedef struct {
    TokenType type;     // Type of the token
    const char *lexeme; // Pointer to the beginning of the lexeme in the source code
                        // For literals like strings or numbers, this points to the raw text.
                        // The actual value (e.g. int value of "123") will be parsed later.
    size_t length;      // Length of the lexeme
    int line;           // Line number where the token starts
    int col;            // Column number where the token starts (approximate)

    // For string literals, we might store the processed string if escapes are handled here.
    // For now, lexeme points to the raw string including quotes.
    // char* string_value; // Example: if TOKEN_STRING stores the actual content
    // double float_value; // Example
    // long long int_value; // Example
} Token;

// Function to convert TokenType to a string (for debugging)
const char* token_type_to_string(TokenType type);

// Function to create a token (useful in lexer)
// Lexeme is NOT copied by this function, it assumes lexeme points to source or a stable buffer.
Token token_create(TokenType type, const char* lexeme, size_t length, int line, int col);

// Function to create an error token
Token token_error_create(const char* message, int line, int col);


#endif // TOKEN_H
