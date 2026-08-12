/*
 * Reviewed reconstruction of W:\SWJediPowerBattles\Work\list.c.
 *
 * Provenance:
 *   direct     - names, signatures, type layouts, RVAs, and code extents from
 *                the exact game.pdb / game.exe pair.
 *   decompiled - control flow checked against Ghidra output and the x64
 *                instructions at RVAs 0xBBBB0 through 0xBBCF2.
 *
 * The deliberately unusual behaviors (list_InitList returning NULL,
 * list_RemoveNode refusing the tail, and list_RemoveTail being a stub) match
 * the reference machine code and are covered by tests.
 */

#include "jpb/list.h"

#include <stddef.h>

/* Reference RVA 0xBBBB0, 29 bytes. */
Node *list_AddHead(List *list, Node *node)
{
    Node *old_head;

    if (list->tail == NULL) {
        list->tail = node;
    }
    old_head = list->head;
    list->head = node;
    if (old_head != NULL) {
        node->next = old_head;
    }
    return node;
}

/* Reference RVA 0xBBBD0, 36 bytes. */
Node *list_AddTail(List *list, Node *node)
{
    if (list->head == NULL) {
        list->head = node;
    }
    if (list->tail != NULL) {
        list->tail->next = node;
    }
    list->tail = node;
    node->next = NULL;
    return node;
}

/* Reference RVA 0xBBC00, 10 bytes. */
List *list_InitList(List *list)
{
    list->tail = NULL;
    list->head = NULL;
    return NULL;
}

/* Reference RVA 0xBBC10, 9 bytes. */
int list_IsListEmpty(List *list)
{
    return list->head == NULL;
}

/* Reference RVA 0xBBC20, 69 bytes. */
void list_MoveList(List *to, List *from)
{
    Node *head;

    while ((head = list_RemoveHead(from)) != NULL) {
        list_AddTail(to, head);
    }
    from->tail = NULL;
    from->head = NULL;
}

/* Reference RVA 0xBBC70, 34 bytes. */
Node *list_RemoveHead(List *list)
{
    Node *head = list->head;

    if (head == NULL) {
        return NULL;
    }
    list->head = head->next;
    if (head->next == NULL) {
        list->tail = NULL;
    }
    head->next = NULL;
    return head;
}

/* Reference RVA 0xBBCA0, 79 bytes. */
Node *list_RemoveNode(List *list, Node *node)
{
    Node *current = list->head;
    Node *previous;

    if (current == NULL) {
        return NULL;
    }
    if (node == current) {
        return list_RemoveHead(list);
    }
    if (node == list->tail) {
        return NULL;
    }

    previous = current;
    while (current != NULL) {
        Node *next = current->next;

        if (current == node) {
            current->next = NULL;
            previous->next = next;
            return current;
        }
        previous = current;
        current = next;
    }
    return NULL;
}

/* Reference RVA 0xBBCF0, 3 bytes: xor eax, eax; ret. */
Node *list_RemoveTail(List *list)
{
    (void)list;
    return NULL;
}
