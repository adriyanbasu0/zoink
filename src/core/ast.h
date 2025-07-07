#ifndef AST_H
#define AST_H

#include <stdbool.h> // For bool type
#include "../util/dynamic_array.h" // For lists of things, e.g., variants, parameters
#include "token.h" // For Token (e.g., storing identifier tokens)

// Forward declarations for recursive structures if needed
struct Expr;
struct Stmt;

//------------------------------------------------------------------------------
// Expression Node Types
//------------------------------------------------------------------------------

typedef enum {
    EXPR_LITERAL,
    EXPR_VARIABLE,
    EXPR_BINARY, // e.g., a + b
    EXPR_UNARY,  // e.g., -a, !b
    EXPR_GROUPING, // e.g., (a + b)
    EXPR_CALL,     // e.g., func(a, b) - For ADT instantiation like Some(T) initially
    // Add more as needed: EXPR_ASSIGN, EXPR_LOGICAL, etc.
} ExprType;

// Base Expression structure (all expression nodes will start with this)
typedef struct Expr {
    ExprType type;
} Expr;

// Literal Expression (e.g., 123, "hello", true, false, None)
// For `None` variant of an ADT, it might be represented as a literal or specific variable.
typedef struct {
    Expr base;
    Token literal; // The token containing the literal value and type (e.g. TOKEN_INTEGER, TOKEN_STRING)
                   // Or a special identifier for things like `None` if treated as a literal keyword/value.
    // We might add parsed values here later, e.g.
    // long long int_val;
    // char* string_val; (owned)
} ExprLiteral;

// Variable Expression (e.g., x, option_value)
typedef struct {
    Expr base;
    Token name; // The identifier token
} ExprVariable;

// For ADT instantiation like `Some(value)` or `Color(255,0,0)`
// This can also be used for general function calls later.
typedef struct {
    Expr base;
    struct Expr* callee;        // Expression for the constructor/function name (e.g., `Some`, `List::Cons`)
                                // This might be an ExprVariable or ExprPath (e.g. MyModule::Type::Variant)
    DynamicArray* arguments;    // DynamicArray of Expr*
    Token closing_paren;        // For error reporting on mismatched parens
} ExprCall;


//------------------------------------------------------------------------------
// Statement Node Types
//------------------------------------------------------------------------------

typedef enum {
    STMT_LET,         // Variable declaration: let x = expr;
    STMT_EXPRESSION,  // Expression statement: expr; (e.g. a function call)
    STMT_DATA,        // ADT definition: data Option<T> { ... }
    // Add more as needed: STMT_IF, STMT_WHILE, STMT_RETURN, STMT_BLOCK, etc.
} StmtType;

// Base Statement structure
typedef struct Stmt {
    StmtType type;
} Stmt;

// Let Statement (Variable Declaration)
typedef struct {
    Stmt base;
    Token name;          // Identifier token for the variable name
    bool is_mutable;
    struct Expr* initializer; // Optional initializer expression (can be NULL)
    // TypeAnnotation* type_annot; // Optional type annotation (future)
} StmtLet;


// ADT Variant Field (for struct-like or tuple-like variants)
typedef struct {
    Token name; // Optional: field name (for struct-like variants, e.g. `Move { x: i32 }`)
                // If NULL, it's a positional/tuple field.
    // TypeAnnotation* type_annot; // The type of the field, e.g., T, String, i32
    // For Phase 1, we might store the type as a Token (identifier) or a simple string.
    Token type_name_token; // e.g., Token for "T", "String", "i32", "List<A>"
                           // This will need to be resolved to an actual type later.
} ADTVariantField;

// ADT Variant (e.g., Some(T), None, Cons(A, List<A>))
typedef struct {
    Token name;                 // Name of the variant (e.g., Some, None, Cons)
    DynamicArray* fields;       // DynamicArray of ADTVariantField*
                                // NULL or empty if the variant has no fields (e.g., None, Quit)
} ADTVariant;

// Data Statement (ADT Definition)
typedef struct {
    Stmt base;
    Token name;                 // Name of the ADT (e.g., Option, List)
    DynamicArray* type_params;  // Optional: DynamicArray of Token* (generic type parameters like T, A)
    DynamicArray* variants;     // DynamicArray of ADTVariant*
} StmtData;


// Program is a list of statements
typedef struct {
    DynamicArray* statements; // DynamicArray of Stmt*
} Program;


//------------------------------------------------------------------------------
// AST Node Constructor Functions (Prototypes)
//------------------------------------------------------------------------------

// Expressions
Expr* ast_expr_literal_create(Token literal);
Expr* ast_expr_variable_create(Token name);
Expr* ast_expr_call_create(Expr* callee, DynamicArray* arguments, Token closing_paren);
// More expression constructors...

// Statements
Stmt* ast_stmt_let_create(Token name, bool is_mutable, Expr* initializer);
Stmt* ast_stmt_data_create(Token name, DynamicArray* type_params, DynamicArray* variants);
ADTVariant* ast_adt_variant_create(Token name, DynamicArray* fields);
ADTVariantField* ast_adt_variant_field_create(Token name, Token type_name_token);

Program* ast_program_create(DynamicArray* statements);

//------------------------------------------------------------------------------
// AST Node Destructor Functions (Prototypes) - Crucial for memory management
//------------------------------------------------------------------------------
void ast_expr_destroy(Expr* expr);
void ast_stmt_destroy(Stmt* stmt);
void ast_program_destroy(Program* program);
// Specific destructors for variants, fields etc. might be needed if they own complex data.
// For now, assume DynamicArray itself handles freeing its internal pointers if elements are also freed.

#endif // AST_H
