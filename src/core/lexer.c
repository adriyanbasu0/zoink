#include "lexer.h"
#include "token.h" // For token_create, token_error_create, TokenType
#include <string.h> // For strncmp, strlen, memcmp
#include <ctype.h>  // For isalpha, isdigit, isalnum
#include <stdlib.h> // For NULL, malloc, free, realloc
#include <stdbool.h>
#include <stdio.h>  // For snprintf


// Helper to check if a character is at the end of the source
static bool is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

// Helper to advance the current character and return it
static char advance(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    lexer->current++;
    lexer->col++; // Basic column tracking
    return lexer->current[-1];
}

// Helper to peek at the current character without consuming it
static char peek(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return *lexer->current;
}

// Helper to peek at the next character
static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer) || lexer->current[1] == '\0') return '\0';
    return lexer->current[1];
}

// Helper to match a character and consume it if it matches
static bool match_char(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    lexer->col++;
    return true;
}

// Helper to add a token to the lexer's token list
static void add_token(Lexer *lexer, TokenType type, const char *lexeme, size_t length) {
    Token token = token_create(type, lexeme, length, lexer->line, lexer->col - (int)length);
    // The Token struct currently assumes lexeme points into the original source.
    // If lexemes need to be copied (e.g. for processed string literals), that logic would go here
    // or in token_create. For now, it's a view into the source.
    da_push(lexer->tokens, (void*)memcpy(malloc(sizeof(Token)), &token, sizeof(Token))); // Store a copy
}

static void add_error_token(Lexer *lexer, const char *message) {
    // Error token's lexeme is the message itself.
    Token error_token = token_error_create(message, lexer->line, lexer->col);
     da_push(lexer->tokens, (void*)memcpy(malloc(sizeof(Token)), &error_token, sizeof(Token)));
}


// Skips whitespace and comments
static void skip_whitespace_and_comments(Lexer *lexer) {
    while (true) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                advance(lexer);
                lexer->line++;
                lexer->col = 1; // Reset column at newline
                break;
            case '/':
                if (peek_next(lexer) == '/') { // Line comment
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') { // Block comment
                    advance(lexer); // Consume /
                    advance(lexer); // Consume *
                    while (!(peek(lexer) == '*' && peek_next(lexer) == '/') && !is_at_end(lexer)) {
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->col = 1;
                        }
                        advance(lexer);
                    }
                    if (!is_at_end(lexer)) advance(lexer); // Consume *
                    if (!is_at_end(lexer)) advance(lexer); // Consume /
                } else {
                    return; // Not a comment, just a slash
                }
                break;
            default:
                return;
        }
    }
}

// Scans a string literal
static void scan_string(Lexer *lexer) {
    const char *start = lexer->current -1; // Already consumed the opening "
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') { // Strings can be multi-line in some languages
            lexer->line++;
            lexer->col = 1; // Reset col, though string internal newlines are complex
        }
        // Handle escape sequences (e.g., \n, \t, \", \\)
        if (peek(lexer) == '\\' && peek_next(lexer) != '\0') {
            advance(lexer); // Consume '\'
        }
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        add_error_token(lexer, "Unterminated string.");
        return;
    }

    advance(lexer); // Consume the closing "
    add_token(lexer, TOKEN_STRING, start, lexer->current - start);
}

// Scans a number literal (integer or float)
static void scan_number(Lexer *lexer) {
    const char *start = lexer->current -1; // Already consumed the first digit
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    // Look for a fractional part (for floats)
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer); // Consume the "."
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
        // TODO: Add TOKEN_FLOAT if language supports it. For now, all numbers are TOKEN_INTEGER
        // This is a simplification. Proper float parsing is more complex.
        // add_token(lexer, TOKEN_FLOAT, start, lexer->current - start);
        // For now, let's treat numbers with '.' as an error or just part of an integer if not fully supported
        add_token(lexer, TOKEN_INTEGER, start, lexer->current - start); // Simplified
    } else {
        add_token(lexer, TOKEN_INTEGER, start, lexer->current - start);
    }
}

// Scans an identifier or keyword
static void scan_identifier_or_keyword(Lexer *lexer) {
    const char *start = lexer->current -1; // Already consumed the first char
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    size_t length = lexer->current - start;
    TokenType type = TOKEN_IDENTIFIER; // Default to identifier

    // Keyword checking (simple approach)
    // A more efficient way would be a hash map or a trie for keywords.
    switch (start[0]) {
        case 'd': if (length == 4 && memcmp(start, "data", 4) == 0) type = TOKEN_DATA; break;
        case 'e': if (length == 4 && memcmp(start, "else", 4) == 0) type = TOKEN_ELSE; break;
        case 'f':
            if (length == 2 && memcmp(start, "fn", 2) == 0) type = TOKEN_FN;
            else if (length == 5 && memcmp(start, "false", 5) == 0) type = TOKEN_FALSE;
            break;
        case 'i': if (length == 2 && memcmp(start, "if", 2) == 0) type = TOKEN_IF; break;
        case 'l': if (length == 3 && memcmp(start, "let", 3) == 0) type = TOKEN_LET; break;
        case 'm':
            if (length == 5 && memcmp(start, "match", 5) == 0) type = TOKEN_MATCH;
            else if (length == 3 && memcmp(start, "mut", 3) == 0) type = TOKEN_MUT;
            break;
        case 'r': if (length == 6 && memcmp(start, "return", 6) == 0) type = TOKEN_RETURN; break;
        case 't':
            if (length == 4 && memcmp(start, "true", 4) == 0) type = TOKEN_TRUE;
            else if (length == 4 && memcmp(start, "type", 4) == 0) type = TOKEN_TYPE; // Future
            break;
        // Add more keywords here
    }
    add_token(lexer, type, start, length);
}


// Scans a single token
static void scan_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    if (is_at_end(lexer)) {
        // Only add one EOF token at the very end.
        // The main loop in lexer_scan_tokens handles adding the final EOF.
        return;
    }

    const char *token_start_lexeme = lexer->current;
    char c = advance(lexer);

    if (isalpha(c) || c == '_') {
        scan_identifier_or_keyword(lexer);
        return;
    }
    if (isdigit(c)) {
        scan_number(lexer);
        return;
    }

    switch (c) {
        // Single-character tokens
        case '(': add_token(lexer, TOKEN_LPAREN, token_start_lexeme, 1); break;
        case ')': add_token(lexer, TOKEN_RPAREN, token_start_lexeme, 1); break;
        case '{': add_token(lexer, TOKEN_LBRACE, token_start_lexeme, 1); break;
        case '}': add_token(lexer, TOKEN_RBRACE, token_start_lexeme, 1); break;
        case '[': add_token(lexer, TOKEN_LBRACKET, token_start_lexeme, 1); break;
        case ']': add_token(lexer, TOKEN_RBRACKET, token_start_lexeme, 1); break;
        case ',': add_token(lexer, TOKEN_COMMA, token_start_lexeme, 1); break;
        case '.': add_token(lexer, TOKEN_DOT, token_start_lexeme, 1); break;
        case '-': add_token(lexer, TOKEN_MINUS, token_start_lexeme, 1); break;
        case '+': add_token(lexer, TOKEN_PLUS, token_start_lexeme, 1); break;
        case ';': add_token(lexer, TOKEN_SEMICOLON, token_start_lexeme, 1); break;
        case '*': add_token(lexer, TOKEN_ASTERISK, token_start_lexeme, 1); break;
        case '/': add_token(lexer, TOKEN_SLASH, token_start_lexeme, 1); break; // Comments handled in skip_whitespace
        case '%': add_token(lexer, TOKEN_PERCENT, token_start_lexeme, 1); break;
        case ':': add_token(lexer, TOKEN_COLON, token_start_lexeme, 1); break;
        // case '|': add_token(lexer, TOKEN_PIPE, token_start_lexeme, 1); break; // This is the duplicated one, removed.


        // One or two character tokens
        case '!': {
            bool is_ne = match_char(lexer, '=');
            TokenType type = is_ne ? TOKEN_NOT_EQUAL : TOKEN_NOT;
            size_t length = lexer->current - token_start_lexeme;
            add_token(lexer, type, token_start_lexeme, length);
            break;
        }
        case '=': // Already correct structure
            if (match_char(lexer, '>')) { // =>
                add_token(lexer, TOKEN_ARROW, token_start_lexeme, 2); // Length is 2
            } else if (match_char(lexer, '=')) { // ==
                add_token(lexer, TOKEN_EQUAL, token_start_lexeme, 2); // Length is 2
            } else { // =
                add_token(lexer, TOKEN_ASSIGN, token_start_lexeme, 1); // Length is 1
            }
            break;
        case '<': {
            bool is_le = match_char(lexer, '=');
            TokenType type = is_le ? TOKEN_LESS_EQUAL : TOKEN_LESS;
            size_t length = lexer->current - token_start_lexeme;
            add_token(lexer, type, token_start_lexeme, length);
            break;
        }
        case '>': {
            bool is_ge = match_char(lexer, '=');
            TokenType type = is_ge ? TOKEN_GREATER_EQUAL : TOKEN_GREATER;
            size_t length = lexer->current - token_start_lexeme;
            add_token(lexer, type, token_start_lexeme, length);
            break;
        }
        case '&': {
            bool is_and = match_char(lexer, '&');
            TokenType type = is_and ? TOKEN_AND : TOKEN_AMPERSAND;
            size_t length = lexer->current - token_start_lexeme;
            add_token(lexer, type, token_start_lexeme, length);
            break;
        }
        case '|':  // Already correct structure
             if (match_char(lexer, '|')) { // ||
                 add_token(lexer, TOKEN_OR, token_start_lexeme, 2);
             } else {
                 add_token(lexer, TOKEN_PIPE, token_start_lexeme, 1);
             }
             break;

        // String literals
        case '"':
            scan_string(lexer);
            break;

        default:
            {
                char msg_buf[64];
                snprintf(msg_buf, sizeof(msg_buf), "Unexpected character: '%c'", c);
                add_error_token(lexer, msg_buf); // This message needs to be managed carefully if not static
            }
            break;
    }
}


Lexer* lexer_create(const char *source) {
    Lexer *lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;

    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->col = 1;
    // Using item_size = sizeof(Token) for the dynamic array, as we will store copies of Tokens.
    lexer->tokens = da_create(16, sizeof(Token));
    if (!lexer->tokens) {
        free(lexer);
        return NULL;
    }
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;
    // Free the tokens themselves if they were copied (as they are now)
    for (size_t i = 0; i < da_count(lexer->tokens); ++i) {
        free(da_get(lexer->tokens, i));
    }
    da_destroy(lexer->tokens);
    free(lexer);
}

bool lexer_scan_tokens(Lexer *lexer) {
    if (!lexer || !lexer->source) return false;

    bool had_error = false;
    while (!is_at_end(lexer)) {
        // Store start of token for length calculation if needed by add_token
        // const char* start_of_token_lexeme = lexer->current;
        scan_token(lexer); // scan_token now calls add_token internally

        // Check if the last token added was an error token
        if (da_count(lexer->tokens) > 0) {
            Token* last_token = (Token*)da_get(lexer->tokens, da_count(lexer->tokens) - 1);
            if (last_token && last_token->type == TOKEN_ERROR) {
                had_error = true;
                // Optionally, stop lexing on first error or collect all errors
                // For now, let's continue to find all lexical errors.
            }
        }
         // Safety break for misbehaving skip_whitespace or other loops
        if (lexer->current == lexer->source && !is_at_end(lexer) && peek(lexer) != '\0') {
             // This case should ideally not happen if skip_whitespace and token scanning always advance.
             // If it does, it means no progress is being made.
            char c = peek(lexer);
            char msg_buf[100];
            snprintf(msg_buf, sizeof(msg_buf), "Lexer stuck at char '%c' (ASCII %d) at line %d, col %d. Advancing.", c, c, lexer->line, lexer->col);
            add_error_token(lexer, msg_buf); // This message needs careful memory management
            advance(lexer); // Force advance
            had_error = true;
        }
    }

    // Add EOF token at the very end
    Token eof_token = token_create(TOKEN_EOF, lexer->current, 0, lexer->line, lexer->col);
    da_push(lexer->tokens, (void*)memcpy(malloc(sizeof(Token)), &eof_token, sizeof(Token)));

    return !had_error;
}

DynamicArray* lexer_get_tokens(Lexer *lexer) {
    if (!lexer) return NULL;
    return lexer->tokens;
}
