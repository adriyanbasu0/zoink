#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stddef.h> // For size_t
#include <stdbool.h> // For bool

// StringBuilder structure for creating and manipulating strings dynamically.
typedef struct {
    char *buffer;       // Pointer to the character buffer
    size_t length;      // Current length of the string (excluding null terminator)
    size_t capacity;    // Current capacity of the buffer (including space for null terminator)
} StringBuilder;

// Creates a new StringBuilder with an initial capacity.
// If initial_capacity is 0, a default capacity will be used.
StringBuilder* sb_create(size_t initial_capacity);

// Frees the StringBuilder and its internal buffer.
void sb_destroy(StringBuilder *sb);

// Appends a single character to the StringBuilder.
// Returns 0 on success, -1 on failure (e.g., memory allocation error).
int sb_append_char(StringBuilder *sb, char c);

// Appends a null-terminated C string to the StringBuilder.
// Returns 0 on success, -1 on failure.
int sb_append_str(StringBuilder *sb, const char *str);

// Appends a string of a specific length (may not be null-terminated) to the StringBuilder.
// Returns 0 on success, -1 on failure.
int sb_append_buf(StringBuilder *sb, const char *buf, size_t len);

// Returns a pointer to the internal C string.
// The returned string is null-terminated.
// The pointer is valid until the StringBuilder is modified or destroyed.
const char* sb_get_str(const StringBuilder *sb);

// Returns the current length of the string in the StringBuilder.
size_t sb_get_length(const StringBuilder *sb);

// Clears the StringBuilder, making it an empty string.
// Capacity is retained.
void sb_clear(StringBuilder *sb);

// Truncates the string in the StringBuilder to a specific length.
// If new_length is greater than current length, this function does nothing.
// Ensures the string is null-terminated after truncation.
void sb_truncate(StringBuilder *sb, size_t new_length);

// Converts the StringBuilder to a heap-allocated C string.
// The caller is responsible for freeing the returned string.
// The StringBuilder itself is NOT destroyed by this call and can continue to be used.
char* sb_to_string(const StringBuilder *sb);

// Resets the StringBuilder and then appends the given C string.
// Effectively sb_clear() followed by sb_append_str().
// Returns 0 on success, -1 on failure.
int sb_reset_and_append_str(StringBuilder *sb, const char *str);

#endif // STRING_BUILDER_H
