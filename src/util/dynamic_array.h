#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h> // For size_t

// Generic dynamic array structure.
// It stores pointers to elements, so it can be used for any type.
// The user is responsible for managing the memory of the elements themselves.
typedef struct {
    void **items;       // Pointer to the array of (void*) items
    size_t capacity;    // Total number of items the array can currently hold
    size_t count;       // Current number of items in the array
    size_t item_size;   // Size of each item (useful for non-pointer arrays, but here we store void*)
} DynamicArray;

// Initializes a new dynamic array.
// Initial capacity can be 0, it will grow on first push.
// item_size is stored for potential future use or for arrays not storing void*.
// For our generic void* array, item_size isn't strictly needed for core ops but good practice.
DynamicArray* da_create(size_t initial_capacity, size_t item_size);

// Frees the dynamic array structure and the internal items array.
// Does NOT free the elements pointed to by the items.
// A separate function or manual iteration is needed if elements themselves are heap-allocated.
void da_destroy(DynamicArray *da);

// Pushes an item onto the end of the dynamic array.
// Returns 0 on success, -1 on failure (e.g., memory allocation failed).
int da_push(DynamicArray *da, void *item);

// Pops an item from the end of the dynamic array.
// Returns the item, or NULL if the array is empty.
void* da_pop(DynamicArray *da);

// Gets an item at a specific index.
// Returns the item, or NULL if the index is out of bounds.
void* da_get(const DynamicArray *da, size_t index);

// Sets an item at a specific index.
// The index must be less than the current count.
// Returns 0 on success, -1 if index is out of bounds.
// This function replaces an existing item. It does not free the old item.
int da_set(DynamicArray *da, size_t index, void *item);

// Removes an item at a specific index and shifts subsequent elements.
// Returns the removed item, or NULL if the index is out of bounds.
// Does NOT free the removed item.
void* da_remove(DynamicArray *da, size_t index);

// Returns the number of items in the array.
size_t da_count(const DynamicArray *da);

// Trims the capacity of the array to its current count.
// Returns 0 on success, -1 on failure.
int da_trim(DynamicArray *da);

// Clears all items from the array (sets count to 0).
// Does NOT free the items themselves. Capacity remains the same.
void da_clear(DynamicArray *da);

#endif // DYNAMIC_ARRAY_H
