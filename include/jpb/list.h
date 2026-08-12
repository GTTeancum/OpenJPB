#ifndef JPB_LIST_H
#define JPB_LIST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct PDB evidence:
 *   Node (TPI 0x10C5): next at offset 0, sizeof 8 in the x64 reference.
 *   List (TPI 0x112C): head at offset 0, tail at offset 8, sizeof 16.
 *
 * These are intrusive, native-pointer structures. They retain the reference
 * layout on a 64-bit evidence host and naturally become the intended 4-byte
 * Node / 8-byte List layouts on the 32-bit original Xbox target.
 */
typedef struct Node Node;

struct Node {
    Node *next;
};

typedef struct List {
    Node *head;
    Node *tail;
} List;

/*
 * Nodes passed to list_AddHead are expected to be detached (next == NULL).
 * This is an original caller invariant: the reference routine only writes
 * node->next when the destination already has a head.
 */
Node *list_AddHead(List *list, Node *node);
Node *list_AddTail(List *list, Node *node);
List *list_InitList(List *list);
int list_IsListEmpty(List *list);
void list_MoveList(List *to, List *from);
Node *list_RemoveHead(List *list);
Node *list_RemoveNode(List *list, Node *node);
Node *list_RemoveTail(List *list);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_LIST_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_LIST_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_LIST_STATIC_ASSERT(offsetof(Node, next) == 0, "Node.next layout changed");
JPB_LIST_STATIC_ASSERT(offsetof(List, head) == 0, "List.head layout changed");
JPB_LIST_STATIC_ASSERT(
    offsetof(List, tail) == sizeof(void *), "List.tail layout changed");

#if UINTPTR_MAX == UINT64_MAX
JPB_LIST_STATIC_ASSERT(sizeof(Node) == 8, "x64 Node must match PDB");
JPB_LIST_STATIC_ASSERT(sizeof(List) == 16, "x64 List must match PDB");
#elif UINTPTR_MAX == UINT32_MAX
JPB_LIST_STATIC_ASSERT(sizeof(Node) == 4, "Xbox Node must be four bytes");
JPB_LIST_STATIC_ASSERT(sizeof(List) == 8, "Xbox List must be eight bytes");
#else
#error Unsupported pointer width for the reconstructed list layout
#endif

#undef JPB_LIST_STATIC_ASSERT

#endif
