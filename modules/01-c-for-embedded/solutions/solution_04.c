/**
 * @file solution_04.c
 * @brief Solution for Exercise 04: Linked List for Embedded (No malloc)
 *
 * Implements a singly linked list backed by a static node pool. All
 * allocation comes from the pool; no heap usage. This is the standard
 * approach for linked data structures in embedded systems.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o solution_04 solution_04.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Configuration
 * ----------------------------------------------------------------------- */
#define MAX_NODES  16

/* -----------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------- */
typedef struct node {
    int32_t      data;
    struct node *next;
} node_t;

typedef struct {
    node_t   pool[MAX_NODES];   /* Static storage for all nodes */
    node_t  *free_list;         /* Head of available-node chain */
    node_t  *head;              /* Head of the user's list */
    uint16_t count;             /* Number of active nodes */
} static_list_t;

/* -----------------------------------------------------------------------
 * alloc_node - Take a node from the free list (private helper)
 *
 * Detaches the first node of the free list and returns it.
 * Returns NULL if the pool is exhausted.
 * ----------------------------------------------------------------------- */
static node_t *alloc_node(static_list_t *list)
{
    if (list->free_list == NULL) {
        return NULL;    /* Pool exhausted */
    }

    node_t *node     = list->free_list;
    list->free_list  = node->next;
    node->next       = NULL;
    list->count++;

    return node;
}

/* -----------------------------------------------------------------------
 * free_node - Return a node to the free list (private helper)
 *
 * Prepends the node to the free list head for O(1) release.
 * ----------------------------------------------------------------------- */
static void free_node(static_list_t *list, node_t *node)
{
    node->data       = 0;
    node->next       = list->free_list;
    list->free_list  = node;
    list->count--;
}

/* -----------------------------------------------------------------------
 * list_init - Initialize the pool and free list
 *
 * Chains all pool nodes into the free list:
 *   pool[0] -> pool[1] -> ... -> pool[MAX_NODES-1] -> NULL
 * ----------------------------------------------------------------------- */
void list_init(static_list_t *list)
{
    list->head  = NULL;
    list->count = 0;

    /* Build the free list by chaining pool entries */
    list->free_list = &list->pool[0];
    for (int i = 0; i < MAX_NODES - 1; i++) {
        list->pool[i].data = 0;
        list->pool[i].next = &list->pool[i + 1];
    }
    list->pool[MAX_NODES - 1].data = 0;
    list->pool[MAX_NODES - 1].next = NULL;
}

/* -----------------------------------------------------------------------
 * list_push_front - Insert at the head of the list (O(1))
 * ----------------------------------------------------------------------- */
int list_push_front(static_list_t *list, int32_t data)
{
    node_t *node = alloc_node(list);
    if (node == NULL) {
        return -1;  /* Pool exhausted */
    }

    node->data = data;
    node->next = list->head;
    list->head = node;

    return 0;
}

/* -----------------------------------------------------------------------
 * list_push_back - Insert at the tail of the list (O(n))
 *
 * Must walk to the end to find the last node.
 * ----------------------------------------------------------------------- */
int list_push_back(static_list_t *list, int32_t data)
{
    node_t *node = alloc_node(list);
    if (node == NULL) {
        return -1;
    }

    node->data = data;
    node->next = NULL;

    if (list->head == NULL) {
        list->head = node;
    } else {
        /* Walk to the last node */
        node_t *curr = list->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = node;
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * list_pop_front - Remove and return the head value (O(1))
 * ----------------------------------------------------------------------- */
int list_pop_front(static_list_t *list, int32_t *data)
{
    if (list->head == NULL) {
        return -1;  /* Empty list */
    }

    node_t *node = list->head;
    *data        = node->data;
    list->head   = node->next;

    free_node(list, node);
    return 0;
}

/* -----------------------------------------------------------------------
 * list_find - Search for a value in the list
 *
 * Returns a pointer to the first matching node, or NULL if not found.
 * ----------------------------------------------------------------------- */
node_t *list_find(const static_list_t *list, int32_t data)
{
    node_t *curr = list->head;
    while (curr != NULL) {
        if (curr->data == data) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/* -----------------------------------------------------------------------
 * list_remove - Remove the first node with a given value
 *
 * Uses the "pointer to pointer" technique for clean removal.
 * pp points to the pointer that references the current node:
 *   - Initially: pp = &list->head
 *   - In loop:   pp = &(current_node->next)
 *
 * When found, *pp is updated to skip the removed node.
 * ----------------------------------------------------------------------- */
int list_remove(static_list_t *list, int32_t data)
{
    node_t **pp = &list->head;

    while (*pp != NULL) {
        if ((*pp)->data == data) {
            node_t *to_remove = *pp;
            *pp = to_remove->next;   /* Unlink the node */
            free_node(list, to_remove);
            return 0;
        }
        pp = &((*pp)->next);
    }

    return -1;  /* Not found */
}

/* -----------------------------------------------------------------------
 * list_insert_sorted - Insert maintaining ascending order
 *
 * Uses the pointer-to-pointer technique to find the insertion point
 * without special-casing head insertion.
 * ----------------------------------------------------------------------- */
int list_insert_sorted(static_list_t *list, int32_t data)
{
    node_t *node = alloc_node(list);
    if (node == NULL) {
        return -1;
    }

    node->data = data;

    /* Find insertion point: pp is the pointer we need to update */
    node_t **pp = &list->head;
    while (*pp != NULL && (*pp)->data < data) {
        pp = &((*pp)->next);
    }

    node->next = *pp;
    *pp = node;

    return 0;
}

/* -----------------------------------------------------------------------
 * list_print - Display all elements
 * ----------------------------------------------------------------------- */
void list_print(const static_list_t *list)
{
    node_t *curr = list->head;
    while (curr != NULL) {
        printf("%d -> ", curr->data);
        curr = curr->next;
    }
    printf("NULL\n");
}

/* -----------------------------------------------------------------------
 * list_count / list_free_count
 * ----------------------------------------------------------------------- */
uint16_t list_count(const static_list_t *list)
{
    return list->count;
}

uint16_t list_free_count(const static_list_t *list)
{
    return MAX_NODES - list->count;
}

/* ======================================================================= */
int main(void)
{
    static_list_t list;

    printf("=== Static Linked List Test ===\n");
    printf("Pool size: %d nodes\n\n", MAX_NODES);

    list_init(&list);

    /* -----------------------------------------------------------
     * Test 1: Push front
     * ----------------------------------------------------------- */
    printf("Test 1: Push front\n");
    list_push_front(&list, 10);
    list_push_front(&list, 20);
    list_push_front(&list, 30);
    printf("  List: ");
    list_print(&list);
    printf("  Count: %d, Free: %d\n\n", list_count(&list), list_free_count(&list));

    /* -----------------------------------------------------------
     * Test 2: Push back
     * ----------------------------------------------------------- */
    printf("Test 2: Push back\n");
    list_push_back(&list, 40);
    list_push_back(&list, 50);
    printf("  List: ");
    list_print(&list);
    printf("  Count: %d, Free: %d\n\n", list_count(&list), list_free_count(&list));

    /* -----------------------------------------------------------
     * Test 3: Pop front
     * ----------------------------------------------------------- */
    printf("Test 3: Pop front\n");
    int32_t popped;
    list_pop_front(&list, &popped);
    printf("  Popped: %d\n", popped);
    printf("  List: ");
    list_print(&list);
    printf("\n");

    /* -----------------------------------------------------------
     * Test 4: Remove from middle
     * ----------------------------------------------------------- */
    printf("Test 4: Remove from middle\n");
    int ret = list_remove(&list, 10);
    printf("  Removed 10: %s\n", (ret == 0) ? "OK" : "FAIL");
    printf("  List: ");
    list_print(&list);
    printf("\n");

    /* -----------------------------------------------------------
     * Test 5: Insert sorted (fresh list)
     * ----------------------------------------------------------- */
    printf("Test 5: Insert sorted\n");

    /* Clear the list first */
    int32_t dummy;
    while (list_pop_front(&list, &dummy) == 0) { }

    list_insert_sorted(&list, 25);
    list_insert_sorted(&list, 5);
    list_insert_sorted(&list, 45);
    list_insert_sorted(&list, 15);
    list_insert_sorted(&list, 35);

    printf("  List: ");
    list_print(&list);

    /* Verify sorted order */
    int sorted_pass = 1;
    node_t *curr = list.head;
    while (curr != NULL && curr->next != NULL) {
        if (curr->data > curr->next->data) {
            sorted_pass = 0;
            break;
        }
        curr = curr->next;
    }
    printf("  Sorted order: %s\n\n", sorted_pass ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Test 6: Pool exhaustion
     * ----------------------------------------------------------- */
    printf("Test 6: Pool exhaustion\n");

    /* Clear again */
    while (list_pop_front(&list, &dummy) == 0) { }

    /* Fill the entire pool */
    int fill_pass = 1;
    for (int i = 0; i < MAX_NODES; i++) {
        ret = list_push_back(&list, i * 100);
        if (ret != 0) {
            printf("  Unexpected failure at node %d\n", i);
            fill_pass = 0;
        }
    }

    /* Next push should fail */
    ret = list_push_front(&list, 9999);
    if (ret != -1) {
        fill_pass = 0;
    }

    printf("  Filled %d nodes. Push returned %d %s\n",
           list_count(&list), ret, fill_pass ? "[PASS]" : "[FAIL]");
    printf("  Free count: %d\n\n", list_free_count(&list));

    /* -----------------------------------------------------------
     * Test 7: Reuse after clearing
     * ----------------------------------------------------------- */
    printf("Test 7: Reuse after clearing\n");
    while (list_pop_front(&list, &dummy) == 0) { }

    int reuse_pass = 1;
    for (int i = 0; i < MAX_NODES; i++) {
        ret = list_push_back(&list, i);
        if (ret != 0) {
            reuse_pass = 0;
        }
    }
    printf("  Re-filled %d nodes after clearing: %s\n",
           list_count(&list), reuse_pass ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Summary
     * ----------------------------------------------------------- */
    int all_pass = sorted_pass && fill_pass && reuse_pass;
    printf("\n%s\n", all_pass ? "All tests passed!" : "Some tests FAILED!");

    printf("\n=== End of Exercise 04 ===\n");
    return 0;
}
