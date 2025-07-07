#include "string_builder.h"
#include <stdlib.h>
#include <string.h> // For strlen, strcpy, strcat, memcpy
#include <stdio.h>  // For vsnprintf if we add formatted append

#define SB_DEFAULT_INITIAL_CAPACITY 16 // Includes null terminator
#define SB_GROWTH_FACTOR 2

// Ensures capacity for at least 'additional_len' more characters plus null terminator.
// Returns 0 on success, -1 on failure.
static int sb_ensure_capacity(StringBuilder *sb, size_t additional_len) {
    if (!sb) return -1;

    size_t required_capacity = sb->length + additional_len + 1; // +1 for null terminator
    if (required_capacity <= sb->capacity) {
        return 0; // Enough capacity
    }

    size_t new_capacity = sb->capacity;
    if (new_capacity == 0) {
        new_capacity = SB_DEFAULT_INITIAL_CAPACITY;
    }
    while (new_capacity < required_capacity) {
        new_capacity *= SB_GROWTH_FACTOR;
    }

    char *new_buffer = (char*)realloc(sb->buffer, new_capacity);
    if (!new_buffer) {
        return -1; // Allocation failure
    }
    sb->buffer = new_buffer;
    sb->capacity = new_capacity;
    return 0;
}

StringBuilder* sb_create(size_t initial_capacity) {
    StringBuilder *sb = (StringBuilder*)malloc(sizeof(StringBuilder));
    if (!sb) {
        return NULL;
    }

    sb->length = 0;
    sb->capacity = (initial_capacity == 0) ? SB_DEFAULT_INITIAL_CAPACITY : initial_capacity;
    if (sb->capacity == 0) sb->capacity = SB_DEFAULT_INITIAL_CAPACITY; // Ensure minimum

    sb->buffer = (char*)malloc(sb->capacity);
    if (!sb->buffer) {
        free(sb);
        return NULL;
    }
    sb->buffer[0] = '\0'; // Null-terminate the empty string
    return sb;
}

void sb_destroy(StringBuilder *sb) {
    if (!sb) {
        return;
    }
    if (sb->buffer) {
        free(sb->buffer);
    }
    free(sb);
}

int sb_append_char(StringBuilder *sb, char c) {
    if (!sb) return -1;
    if (sb_ensure_capacity(sb, 1) != 0) {
        return -1;
    }
    sb->buffer[sb->length++] = c;
    sb->buffer[sb->length] = '\0';
    return 0;
}

int sb_append_str(StringBuilder *sb, const char *str) {
    if (!sb || !str) return -1;
    size_t len = strlen(str);
    return sb_append_buf(sb, str, len);
}

int sb_append_buf(StringBuilder *sb, const char *buf, size_t len) {
    if (!sb || !buf || len == 0) { // Allow empty buf append (no-op)
        if (len == 0) return 0;
        return -1;
    }
    if (sb_ensure_capacity(sb, len) != 0) {
        return -1;
    }
    memcpy(sb->buffer + sb->length, buf, len);
    sb->length += len;
    sb->buffer[sb->length] = '\0';
    return 0;
}

const char* sb_get_str(const StringBuilder *sb) {
    if (!sb || !sb->buffer) {
        // Should ideally return a static empty string "" to avoid NULL issues
        static const char* empty_str = "";
        return empty_str;
    }
    return sb->buffer;
}

size_t sb_get_length(const StringBuilder *sb) {
    if (!sb) return 0;
    return sb->length;
}

void sb_clear(StringBuilder *sb) {
    if (sb) {
        sb->length = 0;
        if (sb->buffer && sb->capacity > 0) {
            sb->buffer[0] = '\0';
        }
    }
}

void sb_truncate(StringBuilder *sb, size_t new_length) {
    if (!sb || new_length >= sb->length) {
        return;
    }
    sb->length = new_length;
    sb->buffer[sb->length] = '\0';
}

char* sb_to_string(const StringBuilder *sb) {
    if (!sb || !sb->buffer) return NULL;

    char *new_str = (char*)malloc(sb->length + 1);
    if (!new_str) {
        return NULL;
    }
    memcpy(new_str, sb->buffer, sb->length);
    new_str[sb->length] = '\0';
    return new_str;
}

int sb_reset_and_append_str(StringBuilder *sb, const char *str) {
    if (!sb) return -1;
    sb_clear(sb);
    return sb_append_str(sb, str);
}
