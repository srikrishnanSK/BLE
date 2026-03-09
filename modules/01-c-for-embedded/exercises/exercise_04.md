# Exercise 04: Linked List for Embedded Systems (No malloc)

## Objective

Implement a singly linked list that uses a static memory pool instead of
dynamic allocation. In embedded systems, `malloc` is often unavailable or
forbidden due to fragmentation risks and non-deterministic timing. This
exercise teaches you how to manage linked data structures with pre-allocated
memory.

## Background

A static pool allocator pre-allocates an array of nodes. A "free list" tracks
which nodes are available. When you need a node, you take one from the free
list. When you release a node, you put it back.

```
Pool:  [Node0] [Node1] [Node2] [Node3] [Node4] [Node5] [Node6] [Node7]

Free list: Node3 -> Node5 -> Node6 -> Node7 -> NULL
Used list: Node0 -> Node1 -> Node2 -> Node4 -> NULL  (your linked list)
```

## Task

Implement a singly linked list with a static node pool:

```c
#define MAX_NODES 16

typedef struct node {
    int32_t data;
    struct node *next;
} node_t;

typedef struct {
    node_t pool[MAX_NODES];    /* Pre-allocated node storage */
    node_t *free_list;         /* Head of free node chain */
    node_t *head;              /* Head of the user's list */
    uint16_t count;            /* Number of nodes in use */
} static_list_t;

/* Initialize the pool and free list */
void list_init(static_list_t *list);

/* Insert a value at the head of the list. Returns 0 on success, -1 if pool exhausted. */
int list_push_front(static_list_t *list, int32_t data);

/* Insert a value at the tail of the list. Returns 0 on success, -1 if pool exhausted. */
int list_push_back(static_list_t *list, int32_t data);

/* Remove and return the value at the head. Returns 0 on success, -1 if empty. */
int list_pop_front(static_list_t *list, int32_t *data);

/* Find a node by value. Returns pointer to node, or NULL if not found. */
node_t *list_find(const static_list_t *list, int32_t data);

/* Remove a node by value. Returns 0 on success, -1 if not found. */
int list_remove(static_list_t *list, int32_t data);

/* Insert in sorted order. Returns 0 on success, -1 if pool exhausted. */
int list_insert_sorted(static_list_t *list, int32_t data);

/* Print all elements in the list */
void list_print(const static_list_t *list);

/* Return the number of nodes currently in use */
uint16_t list_count(const static_list_t *list);

/* Return the number of free nodes remaining */
uint16_t list_free_count(const static_list_t *list);
```

## Requirements

1. No calls to `malloc`, `calloc`, `realloc`, or `free`.
2. Allocating a node takes it from the free list.
3. Releasing a node returns it to the free list.
4. The pool must be fully reusable: after removing all nodes, you should be
   able to allocate all of them again.

5. Write tests that:
   - Push and pop elements, verifying LIFO order for push_front/pop_front
   - Push to back and verify order
   - Fill the pool to capacity and verify allocation failure
   - Remove from middle, head, and tail
   - Insert sorted and verify ordering
   - Verify free count is correct after operations

## Hints

- During `list_init`, chain all pool nodes into the free list:
  `pool[0] -> pool[1] -> ... -> pool[MAX_NODES-1] -> NULL`
- To allocate: detach the head of the free list.
- To free: prepend the node back to the free list head.
- For `list_remove`, you need the **previous** pointer to relink the chain.
  Use a "pointer to pointer" (`node_t **pp`) pattern for elegant removal.

## Expected Output

```
=== Static Linked List Test ===

Test 1: Push front
  List: 30 -> 20 -> 10 -> NULL
  Count: 3, Free: 13

Test 2: Push back
  List: 30 -> 20 -> 10 -> 40 -> 50 -> NULL
  Count: 5, Free: 11

Test 3: Pop front
  Popped: 30
  List: 20 -> 10 -> 40 -> 50 -> NULL

Test 4: Remove from middle
  Removed 10
  List: 20 -> 40 -> 50 -> NULL

Test 5: Insert sorted
  List: 5 -> 15 -> 25 -> 35 -> 45 -> NULL

Test 6: Pool exhaustion
  Filled 16 nodes. Push returned -1 [PASS]
  Free count: 0

All tests passed!
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_04 solution_04.c
```
