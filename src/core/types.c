#define _DEFAULT_SOURCE // Or _GNU_SOURCE, for strdup
#include "types.h"
#include "symbol_table.h" // For struct Symbol forward declaration, actual include needed if used.
#include "../util/string_builder.h" // For type_to_string
#include <stdlib.h>
#include <stdio.h> // For snprintf in type_to_string
#include <string.h> // For strcmp in types_are_equal, and strdup

// --- Type Creation Functions ---

Type* type_primitive_create(Token name) {
    TypePrimitive* type = (TypePrimitive*)malloc(sizeof(TypePrimitive));
    if (!type) return NULL;
    type->base.kind = TYPE_PRIMITIVE;
    type->name = name; // Token struct copied
    return (Type*)type;
}

Type* type_adt_create(Token name, DynamicArray* type_args, struct Symbol* adt_symbol) {
    TypeADT* type = (TypeADT*)malloc(sizeof(TypeADT));
    if (!type) return NULL;
    type->base.kind = TYPE_ADT;
    type->name = name; // Token struct copied
    type->type_args = type_args; // Ownership of DA and its Type* elements assumed
    type->adt_symbol = adt_symbol;
    return (Type*)type;
}

Type* type_generic_param_create(Token name) {
    TypeGenericParam* type = (TypeGenericParam*)malloc(sizeof(TypeGenericParam));
    if (!type) return NULL;
    type->base.kind = TYPE_GENERIC_PARAM;
    type->name = name; // Token struct copied
    return (Type*)type;
}

Type* type_void_create() {
    TypeVoid* type = (TypeVoid*)malloc(sizeof(TypeVoid));
    if (!type) return NULL;
    type->base.kind = TYPE_VOID;
    return (Type*)type;
}

Type* type_error_create() {
    TypeError* type = (TypeError*)malloc(sizeof(TypeError));
    if (!type) return NULL;
    type->base.kind = TYPE_ERROR;
    return (Type*)type;
}

Type* type_unknown_create() {
    Type* type = (Type*)malloc(sizeof(Type)); // Base Type struct is enough
    if (!type) return NULL;
    type->kind = TYPE_UNKNOWN; // Corrected: directly access kind
    return type;
}

// --- Type Destruction ---

void type_destroy(Type* type) {
    if (!type) return;

    switch (type->kind) {
        case TYPE_PRIMITIVE:
            // TypePrimitive* prim = (TypePrimitive*)type;
            // Token name is a struct, its lexeme points to source, no specific free needed for name.
            break;
        case TYPE_ADT: {
            TypeADT* adt_type = (TypeADT*)type;
            // adt_symbol is not owned by TypeADT, it's a pointer to symbol table entry.
            if (adt_type->type_args) {
                for (size_t i = 0; i < da_count(adt_type->type_args); ++i) {
                    type_destroy((Type*)da_get(adt_type->type_args, i));
                }
                da_destroy(adt_type->type_args);
            }
            break;
        }
        case TYPE_GENERIC_PARAM:
            // TypeGenericParam* gp = (TypeGenericParam*)type;
            // Token name is a struct.
            break;
        case TYPE_VOID:
        case TYPE_ERROR:
        case TYPE_UNKNOWN:
            // No specific fields to free for these simple types beyond the base struct.
            break;
        default:
            // Handle other types like TYPE_FUNCTION, TYPE_REFERENCE later
            break;
    }
    free(type);
}

// --- Type Comparison (Simplified for Phase 1) ---
bool types_are_equal(Type* type1, Type* type2) {
    if (!type1 || !type2) return false; // Or true if both NULL? Depends on semantics.
    if (type1 == type2) return true; // Same instance
    if (type1->kind != type2->kind) return false;

    switch (type1->kind) {
        case TYPE_PRIMITIVE: {
            TypePrimitive* p1 = (TypePrimitive*)type1;
            TypePrimitive* p2 = (TypePrimitive*)type2;
            // Compare by name token (assuming lexemes are comparable, e.g. from source or interned)
            return p1->name.length == p2->name.length &&
                   strncmp(p1->name.lexeme, p2->name.lexeme, p1->name.length) == 0;
        }
        case TYPE_ADT: {
            TypeADT* adt1 = (TypeADT*)type1;
            TypeADT* adt2 = (TypeADT*)type2;
            // Compare by underlying ADT definition (symbol) and type arguments
            if (adt1->adt_symbol != adt2->adt_symbol) return false; // Must be same ADT base
            if (da_count(adt1->type_args) != da_count(adt2->type_args)) return false;
            for (size_t i = 0; i < da_count(adt1->type_args); ++i) {
                if (!types_are_equal((Type*)da_get(adt1->type_args, i), (Type*)da_get(adt2->type_args, i))) {
                    return false;
                }
            }
            return true;
        }
        case TYPE_GENERIC_PARAM: {
            TypeGenericParam* gp1 = (TypeGenericParam*)type1;
            TypeGenericParam* gp2 = (TypeGenericParam*)type2;
            // Generic parameters are typically only equal if they are the exact same instance/definition.
            // Comparing by name might be okay within the same scope, but risky across different generic contexts.
            // For now, simple name comparison. This needs refinement for a full type system.
            return gp1->name.length == gp2->name.length &&
                   strncmp(gp1->name.lexeme, gp2->name.lexeme, gp1->name.length) == 0;
        }
        case TYPE_VOID:
        case TYPE_ERROR: // Should error types be considered equal? Usually not.
        case TYPE_UNKNOWN: // Two unknown types are not necessarily equal.
            return type1->kind == type2->kind; // Only equal if both are VOID. Error/Unknown are distinct.
        default:
            return false; // Unhandled or different complex types
    }
}

// --- Type to String Conversion ---
char* type_to_string(Type* type) {
    if (!type) return strdup("<null_type>");

    StringBuilder* sb = sb_create(32);
    if (!sb) return NULL;

    switch (type->kind) {
        case TYPE_PRIMITIVE: {
            TypePrimitive* p = (TypePrimitive*)type;
            sb_append_buf(sb, p->name.lexeme, p->name.length);
            break;
        }
        case TYPE_ADT: {
            TypeADT* adt = (TypeADT*)type;
            sb_append_buf(sb, adt->name.lexeme, adt->name.length);
            if (adt->type_args && da_count(adt->type_args) > 0) {
                sb_append_char(sb, '<');
                for (size_t i = 0; i < da_count(adt->type_args); ++i) {
                    char* arg_str = type_to_string((Type*)da_get(adt->type_args, i));
                    if (arg_str) {
                        sb_append_str(sb, arg_str);
                        free(arg_str);
                    } else {
                        sb_append_str(sb, "?");
                    }
                    if (i < da_count(adt->type_args) - 1) {
                        sb_append_str(sb, ", ");
                    }
                }
                sb_append_char(sb, '>');
            }
            break;
        }
        case TYPE_GENERIC_PARAM: {
            TypeGenericParam* gp = (TypeGenericParam*)type;
            sb_append_buf(sb, gp->name.lexeme, gp->name.length);
            break;
        }
        case TYPE_VOID: sb_append_str(sb, "void"); break;
        case TYPE_ERROR: sb_append_str(sb, "<type_error>"); break;
        case TYPE_UNKNOWN: sb_append_str(sb, "<unknown>"); break;
        default: sb_append_str(sb, "<unhandled_type_kind>"); break;
    }

    char* result = sb_to_string(sb);
    sb_destroy(sb);
    return result;
}


// --- ADT Definition, VariantSymbol, FieldSymbol Helpers ---

ADTDefinition* adt_definition_create(Token name, DynamicArray* type_params, DynamicArray* variants) {
    ADTDefinition* def = (ADTDefinition*)malloc(sizeof(ADTDefinition));
    if (!def) return NULL;
    def->name = name;
    def->type_params = type_params; // Assumes ownership of DA and its TypeGenericParam*
    def->variants = variants;       // Assumes ownership of DA and its ADTVariantSymbol*
    return def;
}

void adt_definition_destroy(ADTDefinition* def) {
    if (!def) return;
    if (def->type_params) {
        for (size_t i = 0; i < da_count(def->type_params); ++i) {
            type_destroy((Type*)da_get(def->type_params, i)); // Assuming type_params stores Type* (specifically TypeGenericParam*)
        }
        da_destroy(def->type_params);
    }
    if (def->variants) {
        for (size_t i = 0; i < da_count(def->variants); ++i) {
            adt_variant_symbol_destroy((ADTVariantSymbol*)da_get(def->variants, i));
        }
        da_destroy(def->variants);
    }
    free(def);
}

ADTVariantSymbol* adt_variant_symbol_create(Token name, DynamicArray* fields) {
    ADTVariantSymbol* var_sym = (ADTVariantSymbol*)malloc(sizeof(ADTVariantSymbol));
    if (!var_sym) return NULL;
    var_sym->name = name;
    var_sym->fields = fields; // Assumes ownership of DA and its ADTFieldSymbol*
    return var_sym;
}

void adt_variant_symbol_destroy(ADTVariantSymbol* var_sym) {
    if (!var_sym) return;
    if (var_sym->fields) {
        for (size_t i = 0; i < da_count(var_sym->fields); ++i) {
            adt_field_symbol_destroy((ADTFieldSymbol*)da_get(var_sym->fields, i));
        }
        da_destroy(var_sym->fields);
    }
    free(var_sym);
}

ADTFieldSymbol* adt_field_symbol_create(Token name, Type* type) {
    ADTFieldSymbol* field_sym = (ADTFieldSymbol*)malloc(sizeof(ADTFieldSymbol));
    if (!field_sym) return NULL;
    field_sym->name = name; // Copied (Token is a struct)
    field_sym->type = type; // Assumes ownership of the type
    return field_sym;
}

void adt_field_symbol_destroy(ADTFieldSymbol* field_sym) {
    if (!field_sym) return;
    type_destroy(field_sym->type); // Destroy the type owned by the field
    free(field_sym);
}
