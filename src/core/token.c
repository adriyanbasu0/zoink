#include "token.h"
#include <string.h> // For strdup if we decide to copy messages for error tokens
#include <stdio.h>  // For printf in debugging, remove later

// For now, error messages are not dynamically allocated with the token.
// The lexer will pass a pointer to a literal string or a buffer.
// If dynamic error messages are needed, they should be handled carefully.
// static char error_message_buffer[256]; // A simple static buffer for error messages

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_DATA: return "DATA";
        case TOKEN_LET: return "LET";
        case TOKEN_MUT: return "MUT";
        case TOKEN_MATCH: return "MATCH";
        case TOKEN_FN: return "FN";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_ASTERISK: return "ASTERISK";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_AMPERSAND: return "AMPERSAND";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_COLON: return "COLON";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_PIPE: return "PIPE";
        default: return "UNKNOWN_TOKEN";
    }
}

Token token_create(TokenType type, const char* lexeme, size_t length, int line, int col) {
    Token token;
    token.type = type;
    token.lexeme = lexeme; // Assumes lexeme is stable (points to source or long-lived buffer)
    token.length = length;
    token.line = line;
    token.col = col;
    return token;
}

// For error tokens, the 'lexeme' field will point to the error message.
// The 'length' will be the length of this message.
Token token_error_create(const char* message, int line, int col) {
    Token token;
    token.type = TOKEN_ERROR;
    // It's better if `message` is a string literal or comes from a buffer
    // that outlives the token's usage, or is copied. For now, direct assignment.
    // If `message` needs to be dynamic and unique per token, it must be allocated
    // and freed appropriately. For now, assume `message` is static or managed elsewhere.
    token.lexeme = message;
    token.length = strlen(message); // Calculate length of the message
    token.line = line;
    token.col = col;
    return token;
}

// Example of how tokens might be printed (for debugging)
void token_print(Token token) {
    // For string tokens, print the actual string content if lexeme includes quotes
    if (token.type == TOKEN_STRING && token.length > 1) {
         printf("Token { type: %s, lexeme: \"%.*s\" (len %zu), line: %d, col: %d }\n",
               token_type_to_string(token.type),
               (int)token.length - 2, // length excluding quotes
               token.lexeme + 1,     // lexeme starting after the first quote
               token.length, token.line, token.col);
    } else {
        printf("Token { type: %s, lexeme: \"%.*s\" (len %zu), line: %d, col: %d }\n",
               token_type_to_string(token.type),
               (int)token.length, token.lexeme,
               token.length, token.line, token.col);
    }
     if (token.type == TOKEN_ERROR) {
        printf("  ERROR MESSAGE: %.*s\n", (int)token.length, token.lexeme);
    }
}
