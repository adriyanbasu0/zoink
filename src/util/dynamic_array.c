#include "dynamic_array.h"
#include <stdlib.h>
#include <string.h> // For memcpy, memmove

// Initial capacity if 0 is passed to da_create
#define DA_DEFAULT_INITIAL_CAPACITY 8

// Factor by which the array grows
#define DA_GROWTH_FACTOR 2

// Internal function to resize the dynamic array
static int da_resize(DynamicArray *da, size_t new_capacity) {
    if (new_capacity == 0) { // Ensure capacity is at least 1 if count is > 0, or default if count is 0
        new_capacity = (da->count > 0) ? da->count : DA_DEFAULT_INITIAL_CAPACITY;
    }
    if (new_capacity < da->count) { // Cannot shrink smaller than current count
        return -1; // Or perhaps resize to da->count? For now, error.
    }

    void **new_items = realloc(da->items, new_capacity * sizeof(void*));
    if (!new_items) {
        return -1; // Allocation failure
    }
    da->items = new_items;
    da->capacity = new_capacity;
    return 0;
}

DynamicArray* da_create(size_t initial_capacity, size_t item_size) {
    DynamicArray *da = (DynamicArray*) malloc(sizeof(DynamicArray));
    if (!da) {
        return NULL;
    }

    da->count = 0;
    da->item_size = item_size; // Store it, though for void* array it's mostly for info
    da->capacity = (initial_capacity == 0) ? DA_DEFAULT_INITIAL_CAPACITY : initial_capacity;

    da->items = (void**) malloc(da->capacity * sizeof(void*));
    if (!da->items) {
        free(da);
        return NULL;
    }
    return da;
}

void da_destroy(DynamicArray *da) {
    if (!da) {
        return;
    }
    if (da->items) {
        free(da->items);
    }
    free(da);
}

int da_push(DynamicArray *da, void *item) {
    if (!da) {
        return -1;
    }
    if (da->count >= da->capacity) {
        size_t new_capacity = (da->capacity == 0) ? DA_DEFAULT_INITIAL_CAPACITY : da->capacity * DA_GROWTH_FACTOR;
        if (da_resize(da, new_capacity) != 0) {
            return -1; // Resize failed
        }
    }
    da->items[da->count++] = item;
    return 0;
}

void* da_pop(DynamicArray *da) {
    if (!da || da->count == 0) {
        return NULL;
    }
    return da->items[--da->count];
}

void* da_get(const DynamicArray *da, size_t index) {
    if (!da || index >= da->count) {
        return NULL;
    }
    return da->items[index];
}

int da_set(DynamicArray *da, size_t index, void *item) {
    if (!da || index >= da->count) {
        return -1;
    }
    da->items[index] = item;
    return 0;
}

void* da_remove(DynamicArray *da, size_t index) {
    if (!da || index >= da->count) {
        return NULL;
    }
    void *item_to_remove = da->items[index];

    // Shift elements if not the last element
    if (index < da->count - 1) {
        memmove(&da->items[index], &da->items[index + 1], (da->count - 1 - index) * sizeof(void*));
    }
    da->count--;

    // Optional: Shrink if array becomes too sparse (e.g., count < capacity / (2*GROWTH_FACTOR))
    // For now, not implementing automatic shrink on remove to avoid thrashing.
    // User can call da_trim if needed.

    return item_to_remove;
}

size_t da_count(const DynamicArray *da) {
    if (!da) {
        return 0;
    }
    return da->count;
}

int da_trim(DynamicArray *da) {
    if (!da) {
        return -1;
    }
    if (da->count == da->capacity) {
        return 0; // Already trimmed
    }
    if (da->count == 0) { // Special case: trim to a small default or 0?
        // Let's free and set to NULL if count is 0, or resize to a minimal capacity.
        // For now, let's resize to DA_DEFAULT_INITIAL_CAPACITY if count is 0, or da->count.
        size_t new_capacity = (da->count == 0) ? DA_DEFAULT_INITIAL_CAPACITY : da->count;
         // If count is 0 and we want to free entirely:
        if (da->count == 0) {
            free(da->items);
            da->items = NULL; // Or malloc a tiny block
            da->capacity = 0;
            return 0;
        }
        return da_resize(da, new_capacity);
    }
    return da_resize(da, da->count);
}

void da_clear(DynamicArray *da) {
    if (da) {
        // Note: This does not free the items pointed to by the elements,
        // only makes them inaccessible through the array.
        da->count = 0;
        // Optionally, could also free da->items and reallocate a small default here.
        // For now, keeps capacity, similar to std::vector::clear() often.
    }
}
