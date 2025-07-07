#include <stdio.h>
#include <stdlib.h> // Added for free()
#include <string.h> // Added for strcmp
#include "util/dynamic_array.h"
#include "util/string_builder.h"
#include "core/lexer.h"
#include "core/token.h"
#include "core/parser.h"
#include "core/ast.h"
#include "core/ast_printer.h"
#include "core/semantic_analyzer.h" // Added

// Function to read entire file into a string (allocates memory)
char* read_file_to_string(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Memory error reading file\n");
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, length, file) != (size_t)length) {
        fprintf(stderr, "Error reading file\n");
        fclose(file);
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}


void run_utility_tests() {
    printf("\n--- Testing Utilities ---\n");
    // Test DynamicArray
    DynamicArray *da = da_create(2, sizeof(int)); // item_size is just for info here
    if (!da) {
        fprintf(stderr, "Failed to create dynamic array.\n");
        exit(1); // Exit if critical utility fails
    }
    printf("DynamicArray created. Count: %zu, Capacity: %zu\n", da_count(da), da->capacity);
    int x = 10, y = 20, z = 30;
    da_push(da, &x); // Storing pointers to stack variables, okay for this test
    da_push(da, &y);
    printf("Pushed 2 items. Count: %zu, Capacity: %zu\n", da_count(da), da->capacity);
    da_push(da, &z);
    printf("Pushed 3rd item. Count: %zu, Capacity: %zu\n", da_count(da), da->capacity);

    printf("Items: ");
    for (size_t i = 0; i < da_count(da); ++i) {
        printf("%d ", *(int*)da_get(da, i));
    }
    printf("\n");
    da_destroy(da);
    printf("DynamicArray destroyed.\n");

    // Test StringBuilder
    StringBuilder *sb = sb_create(10);
    if (!sb) {
        fprintf(stderr, "Failed to create string builder.\n");
        exit(1); // Exit
    }
    printf("StringBuilder created. Length: %zu, Capacity: %zu\n", sb_get_length(sb), sb->capacity);
    sb_append_str(sb, "Hello, ");
    sb_append_char(sb, 'W');
    sb_append_str(sb, "orld!");
    printf("StringBuilder content: '%s', Length: %zu, Capacity: %zu\n", sb_get_str(sb), sb_get_length(sb), sb->capacity);

    char* final_str = sb_to_string(sb);
    printf("Copied string: '%s'\n", final_str);
    free(final_str);

    sb_destroy(sb);
    printf("StringBuilder destroyed.\n");
    printf("--- End Utility Tests ---\n");
}


int main(int argc, char *argv[]) {
    // run_utility_tests(); // Optional: run tests if needed

    if (argc < 2) {
        printf("Mylang Compiler (mylangc)\n");
        printf("Usage: %s <source_file> [-test-lexer]\n", argv[0]);
        printf("       %s -test-lexer \"<source_string>\"\n", argv[0]);
        return 1;
    }

    const char *mode_or_file = argv[1];
    const char *source_to_lex = NULL;
    char *file_content_buffer = NULL; // To hold content read from file

    bool test_lexer_mode_string = false;
    if (strcmp(mode_or_file, "-test-lexer") == 0) {
        if (argc > 2) {
            source_to_lex = argv[2];
            test_lexer_mode_string = true;
            printf("Lexer test mode with direct string input.\n");
        } else {
            fprintf(stderr, "Error: -test-lexer flag requires a source string argument.\n");
            return 1;
        }
    } else {
        // Default mode: compile file
        printf("Compiling source file: %s\n", mode_or_file);
        file_content_buffer = read_file_to_string(mode_or_file);
        if (!file_content_buffer) {
            return 1; // Error reading file
        }
        source_to_lex = file_content_buffer;
        // Check if -test-lexer is an optional trailing arg for file mode
        if (argc > 2 && strcmp(argv[2], "-test-lexer") == 0) {
            test_lexer_mode_string = true; // Treat as test mode for printing tokens
             printf("Lexer test mode for file input (will print tokens).\n");
        }
    }

    if (!source_to_lex) {
        fprintf(stderr, "No source code provided for lexing.\n");
        if (file_content_buffer) free(file_content_buffer);
        return 1;
    }

    // --- Lexer Execution ---
    if (test_lexer_mode_string) { // Only print tokens if in a test mode
        printf("\n--- Lexer Test Output ---\n");
        printf("Source:\n%s\n\nTokens:\n", source_to_lex);
    }

    Lexer *lexer = lexer_create(source_to_lex);
    if (!lexer) {
        fprintf(stderr, "Failed to create lexer.\n");
        if (file_content_buffer) free(file_content_buffer);
        return 1;
    }

    bool lex_success = lexer_scan_tokens(lexer);

    if (test_lexer_mode_string) { // Print tokens if in test mode
        if (lex_success) {
            printf("Lexing successful.\n");
        } else {
            printf("Lexing completed with errors (see below or error tokens).\n");
        }

        DynamicArray *tokens = lexer_get_tokens(lexer);
        for (size_t i = 0; i < da_count(tokens); ++i) {
            Token *token = (Token*)da_get(tokens, i);
            // Basic print, can be enhanced by token_print from token.c if made public
            printf("[%s] '%.*s' (L%d C%d)\n",
                   token_type_to_string(token->type),
                   (int)token->length,
                   token->lexeme,
                   token->line,
                   token->col);
            if (token->type == TOKEN_ERROR) {
                 // The lexeme of an error token IS the error message
                 printf("  ERROR DETAILS: %.*s\n", (int)token->length, token->lexeme);
            }
        }
        printf("--- End Lexer Test Output ---\n");
    } else if (!lex_success) {
        // In normal compilation mode, if lexing fails, print errors and exit.
        fprintf(stderr, "Lexical analysis failed. Errors:\n");
        DynamicArray *tokens = lexer_get_tokens(lexer);
        for (size_t i = 0; i < da_count(tokens); ++i) {
            Token *token = (Token*)da_get(tokens, i);
            if (token->type == TOKEN_ERROR) {
                 fprintf(stderr, "L%d C%d: %.*s\n", token->line, token->col, (int)token->length, token->lexeme);
            }
        }
        lexer_destroy(lexer);
        if (file_content_buffer) free(file_content_buffer);
        return 1; // Indicate failure
    }

    Program *program = NULL;
    bool parse_errors = false;
    bool semantic_errors = false;

    if (lex_success) {
        Parser *parser = parser_create(lexer_get_tokens(lexer));
        // ... (parser creation and parsing logic as before) ...
        if (!parser) {
            fprintf(stderr, "Failed to create parser.\n");
            lexer_destroy(lexer);
            if (file_content_buffer) free(file_content_buffer);
            return 1;
        }
        printf("\n--- Parsing ---\n");
        program = parser_parse(parser);

        if (parser_had_error(parser) || !program) {
            fprintf(stderr, "Parsing failed with errors.\n");
            parse_errors = true;
        } else {
            printf("Parsing successful.\n");
            if (test_lexer_mode_string) {
                printf("\n--- AST Output ---\n");
                ast_print_program(program, stdout);
            }

            // --- Semantic Analysis ---
            printf("\n--- Semantic Analysis ---\n");
            SemanticAnalyzer *analyzer = semantic_analyzer_create();
            if (!analyzer) {
                fprintf(stderr, "Failed to create semantic analyzer.\n");
                semantic_errors = true; // Critical failure
            } else {
                if (semantic_analyzer_analyze(analyzer, program)) {
                    printf("Semantic analysis successful.\n");
                } else {
                    fprintf(stderr, "Semantic analysis failed with errors.\n");
                    semantic_errors = true;
                }
                semantic_analyzer_destroy(analyzer);
            }
        }
        parser_destroy(parser);
    } else {
        parse_errors = true;
    }

    if (!test_lexer_mode_string && lex_success && !parse_errors && !semantic_errors) {
         printf("\nCompilation pipeline (Lexer + Parser + Semantic Analyzer) successful.\n");
         // Next steps: More Semantic Analysis (type checking, ownership), Code Generation
    } else if (!test_lexer_mode_string && (parse_errors || !lex_success || semantic_errors)) {
        fprintf(stderr, "\nCompilation failed during lexing, parsing, or semantic analysis.\n");
    }

    // Cleanup
    if (program) {
        ast_program_destroy(program);
    }
    lexer_destroy(lexer);
    if (file_content_buffer) free(file_content_buffer);

    return 0;
}
