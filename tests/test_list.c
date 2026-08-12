#include "jpb/list.h"

#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_initialization_and_empty_behavior(void)
{
    Node stale_head = {NULL};
    Node stale_tail = {NULL};
    List list = {&stale_head, &stale_tail};

    CHECK(list_InitList(&list) == NULL);
    CHECK(list.head == NULL);
    CHECK(list.tail == NULL);
    CHECK(list_IsListEmpty(&list));
    CHECK(list_RemoveHead(&list) == NULL);
    CHECK(list_RemoveTail(&list) == NULL);
    return 0;
}

static int test_add_and_remove_behavior(void)
{
    List list;
    Node a = {NULL};
    Node b = {NULL};
    Node c = {NULL};

    list_InitList(&list);
    CHECK(list_AddHead(&list, &b) == &b);
    CHECK(list_AddHead(&list, &a) == &a);
    CHECK(list_AddTail(&list, &c) == &c);
    CHECK(list.head == &a);
    CHECK(list.tail == &c);
    CHECK(a.next == &b);
    CHECK(b.next == &c);
    CHECK(c.next == NULL);

    CHECK(list_RemoveNode(&list, &b) == &b);
    CHECK(b.next == NULL);
    CHECK(a.next == &c);

    /* The original routine deliberately refuses to remove the tail. */
    CHECK(list_RemoveNode(&list, &c) == NULL);
    CHECK(list.tail == &c);
    CHECK(a.next == &c);

    CHECK(list_RemoveHead(&list) == &a);
    CHECK(a.next == NULL);
    CHECK(list_RemoveHead(&list) == &c);
    CHECK(list.head == NULL);
    CHECK(list.tail == NULL);
    return 0;
}

static int test_move_preserves_order(void)
{
    List from;
    List to;
    Node a = {NULL};
    Node b = {NULL};
    Node c = {NULL};

    list_InitList(&from);
    list_InitList(&to);
    list_AddTail(&from, &a);
    list_AddTail(&from, &b);
    list_AddTail(&to, &c);

    list_MoveList(&to, &from);
    CHECK(from.head == NULL);
    CHECK(from.tail == NULL);
    CHECK(to.head == &c);
    CHECK(to.tail == &b);
    CHECK(c.next == &a);
    CHECK(a.next == &b);
    CHECK(b.next == NULL);
    return 0;
}

static int test_add_head_retains_reference_precondition(void)
{
    List list;
    Node sentinel = {NULL};
    Node node = {&sentinel};

    list_InitList(&list);
    list_AddHead(&list, &node);

    /*
     * This unusual result is reference behavior. Callers must supply detached
     * nodes; documenting it prevents a "cleanup" from silently changing the
     * reconstructed contract.
     */
    CHECK(node.next == &sentinel);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= test_initialization_and_empty_behavior();
    result |= test_add_and_remove_behavior();
    result |= test_move_preserves_order();
    result |= test_add_head_retains_reference_precondition();
    return result;
}
