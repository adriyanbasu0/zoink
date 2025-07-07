# Ownership Model Design

This document outlines the proposed ownership model for our new programming language. The goal is to achieve memory safety without requiring a garbage collector, drawing inspiration primarily from Rust's model but adapted for implementation in a C-based compiler.

## 1. Core Principles

*   **Ownership:** Each value in the language has a variable that is its *owner*.
*   **Single Owner:** There can only be one owner of a particular piece of data at a time.
*   **Scope-Based Lifetime:** When the owner goes out of scope, the owned value is *dropped* (i.e., its memory is deallocated, and any associated resources are freed).
*   **Stack vs. Heap:**
    *   Primitive types (integers, booleans, characters, floats) are typically stack-allocated and copied by default (Copy semantics).
    *   Complex types (ADTs, strings, vectors, etc., which might manage heap-allocated data) follow move semantics by default. They can implement a `Copy` trait/typeclass if they are simple enough to be bitwise copied without issues (e.g., an ADT containing only `Copy` types).

## 2. Move Semantics

*   **Default Behavior:** For types that are not `Copy`, assignment or passing them to a function by value *moves* ownership.
    ```
    let s1 = String::new("hello"); // s1 owns the string data
    let s2 = s1;                  // Ownership of the string data is moved to s2.
                                  // s1 is no longer valid and cannot be used.
                                  // Accessing s1 after this point is a compile-time error.
    ```
*   **Function Arguments:**
    ```
    fn take_ownership(some_string: String) {
        // some_string now owns the data passed to it.
        // When take_ownership goes out of scope, some_string is dropped.
    }

    let my_str = String::new("world");
    take_ownership(my_str);        // my_str's ownership is moved into the function.
                                   // my_str cannot be used here anymore.
    ```
*   **Function Return Values:** Functions can also transfer ownership of a value to their caller.
    ```
    fn create_string() -> String {
        let new_str = String::new("new data");
        return new_str; // Ownership of new_str is moved out to the caller.
    }

    let owned_str = create_string(); // owned_str now owns the string.
    ```

## 3. Borrowing and References

To access data without taking ownership, the language will support *references* (borrows).

*   **Immutable References (`&`):**
    *   Allows reading the data but not modifying it.
    *   Multiple immutable references to the same data can exist simultaneously.
    *   An immutable reference can exist at the same time as the owner.
    *   Syntax: `&value`
    ```
    fn print_string(s: &String) {
        // s is an immutable reference.
        // We can read from s, but not change it.
        // s does not own the data.
        println!("{}", s);
    }

    let owner_str = String::new("immutable borrow test");
    print_string(&owner_str); // Pass an immutable reference.
    print_string(&owner_str); // Can do it again.
    // owner_str is still valid here and owns the data.
    ```

*   **Mutable References (`&mut`):**
    *   Allows modifying the data.
    *   Only **one** mutable reference to a particular piece of data can exist in a particular scope.
    *   If a mutable reference exists, no other references (immutable or mutable) to that data can exist.
    *   The owner cannot be used (even immutably) while a mutable reference to its data exists.
    *   Syntax: `&mut value`
    ```
    fn append_to_string(s: &mut String) {
        // s is a mutable reference.
        // We can modify the string s points to.
        s.push_str(" appended");
    }

    let mut my_data = String::new("mutable borrow"); // Must be declared mutable to lend out a mutable reference
    append_to_string(&mut my_data);
    // After append_to_string returns, my_data can be used again.
    println!("{}", my_data); // Prints "mutable borrow appended"

    // Error examples (to be caught by the compiler):
    // let mut s = String::new("test");
    // let r1 = &mut s;
    // let r2 = &mut s; // ERROR: cannot have two mutable borrows at once
    // println!("{}", r1);

    // let mut s = String::new("test");
    // let r1 = &s;
    // let r2 = &mut s; // ERROR: cannot have mutable borrow while immutable one exists
    // println!("{}", r1);

    // let mut s = String::new("test");
    // let r1 = &mut s;
    // println!("{}", s); // ERROR: cannot use owner while mutable borrow exists
    ```

## 4. Lifetimes

Lifetimes ensure that references are always valid.

*   **Core Principle:** A reference's lifetime cannot outlive the lifetime of the data it refers to.
*   **Lexical Scopes (Initial Focus):** For the first implementation phase, lifetime analysis will primarily rely on lexical scopes.
    *   A reference created within a scope cannot escape that scope if the data it points to is owned by that scope or a parent scope and might be dropped.
    *   *Example of an error:*
        ```
        // fn dangle() -> &String { // ERROR: returns reference to data owned by this function
        //     let s = String::new("dangling");
        //     return &s; // s is dropped here, reference would be invalid
        // }
        ```
*   **Function Signatures:**
    *   For functions that take references as input and return references, if the output reference refers to input data, their lifetimes must be connected.
    *   Initially, we might enforce simple rules:
        1.  If a function takes one reference and returns one reference, the output reference is assumed to have the same lifetime as the input.
        2.  If there are multiple input references, or if the output reference is to newly created data within the function (which would typically be an error unless ownership is returned), more complex rules or explicit lifetime annotations (like Rust's `'a`) would be needed. We will defer explicit annotations for a later phase.
    *   *Example of a valid case (simplified rule 1):*
        ```
        fn get_first_word(s: &String) -> &str { // Assuming &str is a string slice type
            // ... logic to find first word ...
            // This is okay if the returned slice points into 's'.
        }
        ```
*   **Data Structures Containing References:** If ADTs or other structures contain references, they will also need lifetime considerations. This will be a more advanced topic, likely requiring explicit lifetime annotations in the future. For now, we might restrict ADTs from holding references unless they are very simple cases tied to constructor arguments.

## 5. Enforcement Strategy (C Compiler Context)

Implementing full static borrow checking and lifetime analysis like Rust's directly in C is a monumental task. Here's a pragmatic approach:

*   **Static Analysis (Best Effort):**
    *   **Ownership Tracking:** The compiler's semantic analysis phase will track ownership of variables.
        *   Mark variables as "valid" or "moved from". Attempts to use a moved variable will be a compile-time error.
        *   Track current owner of heap-allocated data.
    *   **Borrowing Rules within a Single Function:** Many borrow checking rules (e.g., one mutable borrow, or multiple immutable borrows) can be checked reasonably well within the scope of a single function's AST.
        *   Track active references and their types (`&` or `&mut`) to a given variable or data.
    *   **Lifetime Analysis (Lexical):** Enforce that references do not escape the scope of the data they point to, primarily for local variables.
*   **Runtime Checks (Where Necessary, Especially Initially):**
    *   For more complex inter-procedural lifetime issues or cases where static analysis is too hard for the initial compiler version, we might introduce runtime checks. This is a trade-off for safety but incurs performance overhead. This should be a last resort or a temporary measure.
    *   *Example:* A "fat pointer" for references could include a pointer to a validity flag or a generation count, checked at dereference. This adds overhead.
*   **C Code Generation:**
    *   **Ownership Transfer:** When a value is moved, the C code will literally involve assigning pointers/data and "nulling out" or marking the source as invalid (if it's a pointer wrapper).
    *   **Dropping:** Code to call deallocation functions (e.g., `free()` for data, or custom drop functions for ADTs) will be inserted at the end of scopes for owned data.
    *   **References:** References will likely be implemented as pointers in the generated C code.

## 6. `Copy` and `Drop` Traits/Typeclasses (Future Consideration)

*   **`Copy`:** Types that can be trivially bitwise copied (like simple structs of integers) can be marked with a `Copy` trait. For these types, assignment will copy the data instead of moving it.
*   **`Drop`:** Types that need custom deallocation logic (e.g., freeing multiple heap allocations, closing file handles) will implement a `Drop` trait. The compiler will ensure the `drop` method is called when the value goes out of scope.

This detailed design provides a solid foundation for the ownership model. The next step will be to detail ADT syntax/semantics and then pattern matching.
---

I have created the `docs/ownership_model.md` file with the detailed design for the ownership model.

Next, I will work on the **ADT Syntax and Semantics** and then **Basic Pattern Matching**.
After that, I will mark Step 1 as complete.
