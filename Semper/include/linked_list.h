#pragma once

#if 1
#define pos(STRUCT, MEMBER) ((size_t) ((char*) (&(STRUCT)->MEMBER)-((char*)(STRUCT))))
#define element_of(current, type, member) (void*)(((char*)current - pos(type, member)))
#else

#undef offsetof
#define pos(STRUCT, MEMBER) ((size_t) & ((STRUCT*)0)->MEMBER)
#define element_of(current, type, member) ((type*)((char*)current - (char*)offsetof(type, member)))
#endif




typedef struct _list_entry
{
    struct _list_entry *next, *prev;
} list_entry;

static inline void _linked_list_add(list_entry* p, list_entry* c, list_entry* n)
{
    c->prev = p;
    c->next = n;
    n->prev = c;
    p->next = c;
}
static inline void _linked_list_remove(list_entry* p, list_entry* n)
{
    p->next = n;
    n->prev = p;
}
static inline void linked_list_add(list_entry* c, list_entry* head)
{
    _linked_list_add(head, c, head->next);
}

static inline void linked_list_add_last(list_entry* c, list_entry* head)
{
    _linked_list_add(head->prev, c, head);
}

static inline void linked_list_remove(list_entry* c)
{
    _linked_list_remove(c->prev, c->next);
    c->next = NULL;
    c->prev = NULL;
}

static inline int linked_list_empty(list_entry* head)
{
    return (head->next == head);
}

static inline void linked_list_replace(list_entry *cr, list_entry *nw)
{
    nw->next = cr->next;
    nw->prev = cr->prev;
    nw->next->prev = nw;
    nw->prev->next = nw;
}
static inline int linked_list_single(list_entry *head)
{
    return(head->next->next == head);
}

#define list_enum(pos, head) for(pos = (head->next); pos != (head); pos = pos->next)

#define list_enum_backward(pos, head) for(pos = (head->prev); pos != (head); pos = pos->prev)

#define list_enum_safe(pos, temp, head) \
    for(pos = (head->next), temp = pos->next; pos != (head); pos = temp, temp = temp->next)

#define list_enum_backward_safe(pos, temp, head) \
    for(pos = (head->prev), temp = pos->prev; pos != (head); pos = n, n = n->prev)

#define list_enum_part(pos, head, member)                                             \
    for(pos = element_of((head)->next, pos, member); &pos->member != (head); \
            pos = element_of((pos)->member.next, (pos), member))

#define list_enum_part_backward(pos, head, member)                                    \
    for(pos = element_of((head)->prev, pos, member); &pos->member != (head); \
            pos = element_of((pos)->member.prev, (pos), member))

#define list_enum_part_backward_safe(pos, temp, head, member)    \
    for(pos = element_of((head)->prev, (pos), member),    \
            temp = element_of((pos)->member.prev, (pos), member); \
            &pos->member != (head); pos = temp, temp = element_of((pos)->member.prev, (pos), member))

#define list_enum_part_safe(pos, temp, head, member)             \
    for(pos = element_of((head)->next, (pos), member),    \
            temp = element_of((pos)->member.next, (pos), member); \
            &pos->member != (head); pos = temp, temp = element_of((pos)->member.next, (pos), member))

#define list_entry_init(le)             \
    {                                   \
        (le)->next = (le)->prev = (le); \
    }
#define list_entry_get(current, type, member) (current, type, member)
