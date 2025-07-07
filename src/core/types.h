#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h> // For bool
#include "../util/dynamic_array.h" // For fields in ADTs, type parameters
#include "token.h" // For names

// Forward declarations
struct Type;
struct Symbol;         // For symbol table entries that might include type info
struct ADTDefinition;  // Full definition of an ADT stored in symbol table
struct ADTVariantSymbol;
struct ADTFieldSymbol;


// Basic Type Kinds
typedef enum {
    TYPE_PRIMITIVE,  // e.g., i32, bool, String (if String is primitive)
    TYPE_ADT,        // Algebraic Data Type defined by 'data'
    TYPE_GENERIC_PARAM, // A type parameter like T in Option<T>
    TYPE_FUNCTION,   // For later: (T1, T2) -> R
    TYPE_REFERENCE,  // For later: &T, &mut T
    TYPE_VOID,       // Represents no type, e.g. for functions not returning a value
    TYPE_ERROR,      // Represents an error in type resolution or a problematic type
    TYPE_UNKNOWN,    // Placeholder until type is inferred or resolved
} TypeKind;

// Base Type structure
typedef struct Type {
    TypeKind kind;
    // Add common properties if any, e.g., size, alignment (for later stages)
    // For ownership, we might add flags or pointers to lifetime info here or in Symbol.
    // bool is_copyable; // Based on Copy trait/typeclass
} Type;

// Primitive Types
typedef struct {
    Type base;
    Token name; // e.g., "i32", "bool", "String" token
} TypePrimitive;

// ADT Types (a reference to a defined ADT)
// This represents an *instance* of an ADT, like `Option<i32>` or `List<String>`
typedef struct {
    Type base;
    Token name;                 // Name of the ADT (e.g., Option, List)
    DynamicArray* type_args;    // DynamicArray of Type* (resolved actual types for generic parameters)
                                // e.g., for Option<i32>, type_args would contain a Type* for i32.
                                // This points to the ADT's *definition* or declaration symbol.
    struct Symbol* adt_symbol;  // Pointer to the Symbol table entry for the ADT's definition
                                // This allows access to variants, fields, original type params etc.
} TypeADT;

// Generic Type Parameter (e.g., T within the definition of Option<T>)
typedef struct {
    Type base;
    Token name; // The token for 'T'
    // We might need constraints here later (e.g. T: Display)
} TypeGenericParam;

// Type for "void" or unit type
typedef struct {
    Type base;
} TypeVoid;

// Type for representing an error in type processing
typedef struct {
    Type base;
    // char* error_message; // Could store a specific message
} TypeError;


// --- Symbol Table related Type information (often part of a Symbol structure) ---
// This describes the definition of an ADT, to be stored in the symbol table.
// A TypeADT above would point to a Symbol containing this.
typedef struct {
    Token name;                 // Name of the ADT (e.g., Option, List)
    DynamicArray* type_params;  // DynamicArray of TypeGenericParam* (defined generic parameters like T, A)
    DynamicArray* variants;     // DynamicArray of ADTVariantSymbol* (defined variants)
                                // These are not AST ADTVariant nodes, but symbol table representations.
    // Scope* own_scope;        // Each ADT might define its own scope for variants if they are not global.
} ADTDefinition;

// Represents a variant's definition within an ADTDefinition
typedef struct {
    Token name;
    DynamicArray* fields; // DynamicArray of ADTFieldSymbol* (Type and optional name)
    // Type* constructed_type; // The type this variant constructs, e.g. Option<T>
} ADTVariantSymbol;

// Represents a field's definition within an ADTVariantSymbol
typedef struct {
    Token name; // Optional field name
    Type* type; // Resolved type of the field
} ADTFieldSymbol;


// --- Type System API ---

// Create basic types
Type* type_primitive_create(Token name);
Type* type_adt_create(Token name, DynamicArray* type_args, struct Symbol* adt_symbol);
Type* type_generic_param_create(Token name);
Type* type_void_create();
Type* type_error_create();
Type* type_unknown_create(); // Useful as a placeholder

// Destroy types
void type_destroy(Type* type); // Should handle all kinds

// Compare types (for type checking)
// Returns true if types are considered equivalent.
// This will get complex with generics, subtyping (if any), etc.
bool types_are_equal(Type* type1, Type* type2);

// Get a string representation of a type (for error messages, debugging)
// Caller should free the returned string.
char* type_to_string(Type* type);


// Helper to create ADTDefinition (for symbol table population)
ADTDefinition* adt_definition_create(Token name, DynamicArray* type_params, DynamicArray* variants);
void adt_definition_destroy(ADTDefinition* def);

ADTVariantSymbol* adt_variant_symbol_create(Token name, DynamicArray* fields);
void adt_variant_symbol_destroy(ADTVariantSymbol* var_sym);

ADTFieldSymbol* adt_field_symbol_create(Token name, Type* type);
void adt_field_symbol_destroy(ADTFieldSymbol* field_sym);


#endif // TYPES_H
