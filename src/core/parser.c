#include "parser.h"
#include "ast.h"
#include "token.h"
#include <stdio.h>  // For error reporting (fprintf, stderr)
#include <stdlib.h> // For malloc, free
#include <string.h> // For strncmp (potentially)
#include <stdarg.h> // For va_list, va_start, va_end in error reporting
#include <ctype.h>  // For isupper

//------------------------------------------------------------------------------
// Parser Helper Functions
//------------------------------------------------------------------------------

static Token* peek(Parser *parser) {
    if (parser->current < 0 || (size_t)parser->current >= da_count(parser->tokens)) return NULL; // Should not happen with EOF
    return (Token*)da_get(parser->tokens, parser->current);
}

static Token* previous(Parser *parser) {
    if (parser->current <= 0) return NULL; // Should ideally not be called if current is 0
    return (Token*)da_get(parser->tokens, parser->current - 1);
}

static bool is_at_end(Parser *parser) {
    Token* p = peek(parser);
    return p && p->type == TOKEN_EOF;
}

static Token* advance(Parser *parser) {
    if (!is_at_end(parser)) {
        parser->current++;
    }
    return previous(parser);
}

static bool check(Parser *parser, TokenType type) {
    if (is_at_end(parser)) return false;
    Token* p = peek(parser);
    return p && p->type == type;
}

static bool match(Parser *parser, int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        TokenType type = va_arg(args, TokenType);
        if (check(parser, type)) {
            advance(parser);
            va_end(args);
            return true;
        }
    }
    va_end(args);
    return false;
}

//------------------------------------------------------------------------------
// Error Handling
//------------------------------------------------------------------------------

static void parser_error_at(Parser *parser, Token *token, const char *message) {
    parser->had_error = true;
    if (token && token->type == TOKEN_EOF) {
        fprintf(stderr, "[L%d C%d at EOF] Error: %s\n", token->line, token->col, message);
    } else if (token) {
        fprintf(stderr, "[L%d C%d at '%.*s'] Error: %s\n",
                token->line, token->col, (int)token->length, token->lexeme, message);
    } else {
        // This case should be rare, means error is not tied to a specific token
        fprintf(stderr, "Error: %s\n", message);
    }
}

static void parser_error_current(Parser *parser, const char *message) {
    parser_error_at(parser, peek(parser), message);
}

// static void parser_error_previous(Parser *parser, const char *message) { // Unused for now, declaration removed
//     parser_error_at(parser, previous(parser), message);
// }


// Consumes a token of the expected type, or reports an error.
static Token* consume(Parser *parser, TokenType type, const char *message) {
    if (check(parser, type)) {
        return advance(parser);
    }
    parser_error_current(parser, message);
    return NULL; // Return NULL on error to allow parser to attempt recovery or stop
}

//------------------------------------------------------------------------------
// Forward declarations for parsing functions (recursive descent)
//------------------------------------------------------------------------------
static Stmt* parse_statement(Parser *parser);
static Stmt* parse_data_declaration(Parser *parser);
static Stmt* parse_let_declaration(Parser *parser);
// static Expr* parse_expression(Parser *parser); // For later
// static Expr* parse_primary(Parser *parser);    // For later

//------------------------------------------------------------------------------
// Parsing Implementation
//------------------------------------------------------------------------------

// For Phase 1, we are parsing top-level declarations like 'data' and 'let'.
// A full expression parser will be added incrementally.
// For now, if `let` needs an initializer, it will be a placeholder or very simple.

static Stmt* parse_declaration(Parser *parser) {
    if (match(parser, 1, TOKEN_DATA)) {
        return parse_data_declaration(parser);
    }
    if (match(parser, 1, TOKEN_LET)) {
        return parse_let_declaration(parser);
    }
    // Add other declarations like fn, type, etc. here

    // If no declaration keyword is matched, it's an error or an expression statement (later)
    if (!is_at_end(parser) && peek(parser)->type != TOKEN_EOF) {
         parser_error_current(parser, "Expected a declaration (e.g., 'data', 'let').");
         // Synchronize: Advance until a potential statement boundary or EOF.
         // This is a simple synchronization strategy.
         while (!is_at_end(parser) && peek(parser)->type != TOKEN_SEMICOLON &&
                                      peek(parser)->type != TOKEN_DATA &&
                                      peek(parser)->type != TOKEN_LET &&
                                      peek(parser)->type != TOKEN_RBRACE /* for blocks later */ ) {
             advance(parser);
         }
         if(match(parser, 1, TOKEN_SEMICOLON)) { /* consume semicolon if that's where we stopped */ }
    }
    return NULL;
}


static Stmt* parse_data_declaration(Parser *parser) {
    Token* adt_name = consume(parser, TOKEN_IDENTIFIER, "Expected ADT name after 'data'.");
    if (!adt_name) return NULL;

    // Store Token* (pointers to heap-allocated Tokens) for params
    DynamicArray* type_params = da_create(2, sizeof(Token*));

    if (match(parser, 1, TOKEN_LESS)) { // Optional type parameters <T, U>
        if (!check(parser, TOKEN_GREATER)) { // Must not be empty like <>
            do {
                Token* param_name = consume(parser, TOKEN_IDENTIFIER, "Expected type parameter name.");
                if (!param_name) {
                    da_destroy(type_params); // Clean up partially created list
                    return NULL;
                }
                // Store a copy of the token. DynamicArray needs to be configured
                // to copy the Token struct if item_size was sizeof(Token).
                // If DA stores pointers, we'd need to malloc a Token copy.
                // Assuming da_push handles copying if item_size matches.
                Token* param_token_alloc = (Token*)malloc(sizeof(Token));
                if (!param_token_alloc) { /* memory error */ da_destroy(type_params); return NULL; }
                *param_token_alloc = *param_name; // Copy the token data
                da_push(type_params, param_token_alloc); // Store pointer to copied token
            } while (match(parser, 1, TOKEN_COMMA));
        }
        if (!consume(parser, TOKEN_GREATER, "Expected '>' after type parameters.")) {
            // Cleanup type_params which stores Token*
            for(size_t i=0; i<da_count(type_params); ++i) free(da_get(type_params, i));
            da_destroy(type_params);
            return NULL;
        }
    }

    if (!consume(parser, TOKEN_LBRACE, "Expected '{' before ADT variants.")) {
        for(size_t i=0; i<da_count(type_params); ++i) free(da_get(type_params, i));
        da_destroy(type_params);
        return NULL;
    }

    DynamicArray* variants = da_create(2, sizeof(ADTVariant*));

    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        Token* variant_name = consume(parser, TOKEN_IDENTIFIER, "Expected variant name.");
        if (!variant_name) { /* error, try to recover or fail */ break; }

        DynamicArray* fields = NULL;
        if (match(parser, 1, TOKEN_LPAREN)) { // Tuple-like variant: Some(T)
            fields = da_create(2, sizeof(ADTVariantField*));
            if (!check(parser, TOKEN_RPAREN)) { // Must not be empty like Some() unless that's allowed
                do {
                    // For now, assume fields are simple type identifiers.
                    // TODO: Parse full type annotations later (e.g. List<String>, &i32)
                    Token* field_type_name = consume(parser, TOKEN_IDENTIFIER, "Expected type name for variant field.");
                    if (!field_type_name) { /* error */ break; }

                    // For tuple-like fields, the 'name' in ADTVariantField is null.
                    ADTVariantField* field = ast_adt_variant_field_create((Token){0}, *field_type_name);
                    da_push(fields, field);
                } while (match(parser, 1, TOKEN_COMMA));
            }
            if(!consume(parser, TOKEN_RPAREN, "Expected ')' after variant fields.")) {
                // cleanup fields, variants, type_params
                // For now, continue, error is already flagged.
            }
        } else if (match(parser, 1, TOKEN_LBRACE)) { // Struct-like variant: Move { x: i32 } - TODO for later
            parser_error_current(parser, "Struct-like variants not yet supported in Phase 1.");
            // Skip until RBRACE for now
            while(!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) advance(parser);
            consume(parser, TOKEN_RBRACE, "Expected '}' for struct-like variant.");
        }
        // If neither ( nor { follows, it's a unit-like variant (e.g., None, Quit)

        ADTVariant* variant = ast_adt_variant_create(*variant_name, fields);
        da_push(variants, variant);

        if (!match(parser, 1, TOKEN_COMMA)) {
            // If no comma, next token must be '}' or it's an error (unless last variant)
            if (!check(parser, TOKEN_RBRACE)) {
                parser_error_current(parser, "Expected ',' or '}' after variant definition.");
                // Attempt to recover by finding next variant or '}'
            }
            break;
        }
        // If comma is consumed, loop continues for next variant.
        // Handle trailing comma: if after comma, next is '}', it's okay.
        if (check(parser, TOKEN_RBRACE)) break;
    }

    if (!consume(parser, TOKEN_RBRACE, "Expected '}' after ADT variants.")) {
        // cleanup variants, type_params
        // For now, just return NULL, error already flagged
        // Proper cleanup would involve freeing allocated variants and fields.
        ast_stmt_data_create(*adt_name, type_params, variants); // Create to destroy for cleanup
        return NULL;
    }

    return ast_stmt_data_create(*adt_name, type_params, variants);
}


static Stmt* parse_let_declaration(Parser *parser) {
    bool is_mutable = match(parser, 1, TOKEN_MUT);
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected variable name after 'let' or 'let mut'.");
    if (!name) return NULL;

    Expr* initializer = NULL;
    if (match(parser, 1, TOKEN_ASSIGN)) {
        // TODO: Parse actual expressions. For Phase 1, we might skip or parse only literals.
        // initializer = parse_expression(parser);
        // For now, let's assume it's a literal for simplicity if there's an initializer.
        if (check(parser, TOKEN_INTEGER) || check(parser, TOKEN_STRING)) {
            Token* literal_token = advance(parser);
            initializer = ast_expr_literal_create(*literal_token);
        } else if (check(parser, TOKEN_IDENTIFIER)) { // Could be an ADT constructor like None or Some(..)
            Token* id_token = peek(parser); // Don't consume yet, might be start of a call Some(val)
            // This is where expression parsing gets more complex.
            // For Phase 1, `let x = Some(y)` is too complex without expression parsing.
            // Let's just allow `let x = value_literal;` or `let x = existing_var;`
            if (id_token->lexeme && (id_token->length > 0 && isupper(id_token->lexeme[0])) && check(parser, TOKEN_LPAREN)) {
                 parser_error_current(parser, "ADT instantiation in 'let' initializer not yet fully supported in parser Phase 1 (basic expressions only).");
                 // For now, we won't parse the call expression correctly.
                 // Skip it by advancing past the identifier for now if it looks like a constructor.
                 // This part needs proper `parse_expression()`
                 advance(parser); // Consume the identifier
                 // Potentially skip ( ... ) for now
                 // This is a simplification for Phase 1.
            } else if (id_token->lexeme) { // Treat as simple variable
                 advance(parser);
                 initializer = ast_expr_variable_create(*id_token);
            } else {
                 parser_error_current(parser, "Expected an initializer expression after '='.");
            }
        }
         else {
            parser_error_current(parser, "Expected an initializer expression after '='.");
            // No initializer parsed if it's not a simple literal or var.
        }
    }
    // Type annotations (: type) would be parsed here if supported.

    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) {
        if (initializer) ast_expr_destroy(initializer); // Clean up if semicolon is missing
        return NULL;
    }
    return ast_stmt_let_create(*name, is_mutable, initializer);
}


//------------------------------------------------------------------------------
// Public API Implementation
//------------------------------------------------------------------------------

Parser* parser_create(DynamicArray *tokens) {
    Parser *parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->tokens = tokens;
    parser->current = 0;
    parser->had_error = false;
    return parser;
}

void parser_destroy(Parser *parser) {
    if (parser) {
        // Does not own tokens array, so doesn't free it.
        free(parser);
    }
}

Program* parser_parse(Parser *parser) {
    DynamicArray *statements = da_create(8, sizeof(Stmt*)); // Store Stmt*

    while (!is_at_end(parser)) {
        Stmt *declaration = parse_declaration(parser);
        if (declaration) {
            da_push(statements, declaration);
        } else if (parser->had_error) {
            // If parse_declaration returned NULL due to an error,
            // we might want to synchronize to the next statement.
            // Synchronization is partially handled in parse_declaration for now.
            // If not EOF, and error occurred, maybe advance until next likely statement start.
            // This basic synchronization might skip valid code if error recovery is poor.
            if (!is_at_end(parser)) {
                // A more robust sync would look for keywords like 'let', 'fn', 'data', etc.
                // or statement terminators like ';' or '}'
                // advance(parser); // Simple advance, might not be good.
            }
        } else if (!is_at_end(parser)) {
            // parse_declaration returned NULL but no error was flagged,
            // and not at EOF. This implies an unhandled token sequence.
            // This shouldn't happen if parse_declaration handles all cases or errors.
            parser_error_current(parser, "Unexpected token sequence.");
            advance(parser); // Try to move past it.
        }
    }

    if (parser->had_error) {
        // Free already parsed statements if there was a global error.
        // Or, the caller can decide based on parser_had_error().
        // For now, return the partially built AST along with the error flag.
        // The caller should check parser_had_error() and handle cleanup.
        // ast_program_destroy(ast_program_create(statements)); // This would clean up
        // return NULL;
    }

    return ast_program_create(statements);
}

bool parser_had_error(const Parser *parser) {
    return parser ? parser->had_error : true; // Treat NULL parser as error state
}
