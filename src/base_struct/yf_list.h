#ifndef _YF_LIST_H
#define _YF_LIST_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

//note, safe iterator must not empty!!

/*
* single list
*/
struct yf_slist_part
{
        struct yf_slist_part *next;
};

#define YF_EMPTY_SLIST_INIT(name) { NULL }
#define yf_init_slist_head(part) ((part)->next = NULL)

#define yf_slist_push(new_node, part) do { \
        (new_node)->next = (part)->next; \
        (part)->next = new_node; \
} while (0)

static inline yf_slist_part_t* yf_slist_pop(yf_slist_part_t* part)
{
        if ((part)->next == NULL)
                return NULL;
        yf_slist_part_t* poped = (part)->next;
        (part)->next = poped->next;
        poped->next = NULL;
        return  poped;
}

static inline void yf_slist_delete(yf_slist_part_t* pre, yf_slist_part_t* target)
{
        if (target->next)
        {
                pre->next = target->next;
                target->next = NULL;
        }
}

#define yf_slist_empty(part) ((part)->next == NULL)

#define yf_slist_for_each(pos, part) \
        for (pos = (part)->next; pos != NULL; pos = pos->next)

#define yf_slist_for_each_safe(pos, n, part) \
        for (pos = (part)->next, n = pos->next; pos != NULL; \
             pos = n, n = pos->next)


/*
* double list
*/
struct yf_list_part
{
        struct yf_list_part *next, *prev;
};

#define YF_EMPTY_LIST_INIT(name) { &(name), &(name) }

//#define YF_LIST_HEAD(name) \
//        struct yf_list_part_t name = YF_EMPTY_LIST_INIT(name)

#define yf_init_list_head(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)


static inline void __yf_list_add(yf_list_part_t *new_node
                , yf_list_part_t *_prev, yf_list_part_t *_next)
{
        (_next)->prev = new_node;
        (new_node)->next = _next;
        (new_node)->prev = _prev;
        (_prev)->next = new_node;
}

#define yf_list_insert_after(new_node, target)  __yf_list_add(new_node, target, (target)->next)
#define yf_list_insert_before(new_node, target)  __yf_list_add(new_node, (target)->prev, target)

#define yf_list_add_head(new_node, list) yf_list_insert_after(new_node, list)
#define yf_list_add_tail(new_node, list) yf_list_insert_before(new_node, list)

/*
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __yf_list_del(yf_list_part_t *_prev, yf_list_part_t *_next)
{
        if (_prev && _next) {
                (_next)->prev = _prev;
                (_prev)->next = _next;
        }
}

#define yf_list_linked(entry) ((entry)->prev)


#define  yf_list_del(entry) do { \
        __yf_list_del((entry)->prev, (entry)->next); \
        (entry)->next = NULL; \
        (entry)->prev = NULL; \
} while(0)

#define  yf_list_del_init(entry) do { \
        __yf_list_del((entry)->prev, (entry)->next); \
        yf_init_list_head(entry); \
} while (0)


/**
 * list_move_part - delete from one list and add as another's part
 * @list: the entry to move
 * @part: the part that will precede our entry
 */
#define yf_list_move_head(entry, head) do { \
        __yf_list_del((entry)->prev, (entry)->next); \
        yf_list_add_head(entry, head); \
} while (0)

#define yf_list_move_tail(entry, head) do { \
        __yf_list_del((entry)->prev, (entry)->next); \
        yf_list_add_tail(entry, head); \
} while (0)


/**
 * list_empty - tests whether a list is empty
 * @part: the list to test.
 */
#define yf_list_empty(part) ((part)->next == part)

static inline void __yf_list_splice(yf_list_part_t *list, yf_list_part_t *part)
{
        yf_list_part_t *first = list->next;
        yf_list_part_t *last = list->prev;
        yf_list_part_t *at = (part)->next;

        first->prev = part;
        (part)->next = first;

        last->next = at;
        at->prev = last;
}

/*
* pop head or tail
*/
static inline yf_list_part_t* yf_list_pop_head(yf_list_part_t* part)
{
        if (yf_list_empty(part))
                return NULL;

        yf_list_part_t* head = part->next;
        __yf_list_del(part, head->next);
        return head;
}

static inline yf_list_part_t* yf_list_pop_tail(yf_list_part_t* part)
{
        if (yf_list_empty(part))
                return NULL;

        yf_list_part_t* tail = part->prev;
        __yf_list_del(tail->prev, part);
        return tail;
}

/**
 * yf_list_splice - move all nodes of lists to part
 * @list: the new list to op.
 * @part: the place to add it in the first list.
 */
#define  yf_list_splice(list, part) do { \
        if (!yf_list_empty(list)) { \
                __yf_list_splice(list, part); \
                yf_init_list_head(list); \
        } \
} while(0)


/**
 * yf_list_entry - get the struct for this entry
 */
#define yf_list_entry(ptr, type, member) \
        container_of(ptr, type, member)

/**
 * yf_list_for_each    -   iterate over a list
 * @pos:    the &struct yf_list_part_t to use as a loop counter.
 * @part:   the part for your list.
 */
#define yf_list_for_each(pos, part) \
        for (pos = (part)->next; pos != (part); pos = pos->next)

#define yf_list_for_each_r(pos, part) \
        for (pos = (part)->prev; pos != (part); pos = pos->prev)

/**
 * list_for_each_safe   -   iterate over a list safe against removal of list entry
 * @pos:    the &struct yf_list_part_t to use as a loop counter.
 * @n:      another &struct yf_list_part_t to use as temporary storage
 * @part:   the part for your list.
 */

#define __yf_list_for_each_safe(pos, n, part, dirct) \
        for (pos = (part)->dirct, n = pos->dirct; pos != (part); \
             pos = n, n = pos->dirct)
             
#define yf_list_for_each_safe(pos, n, part) \
        __yf_list_for_each_safe(pos, n, part, next)
        
#define yf_list_for_each_safe_r(pos, p, part) \
        __yf_list_for_each_safe(pos, n, part, prev)

/**
 * list_for_each_entry  -   iterate over list of given type
 * @pos:    the type * to use as a loop counter.
 * @part:   the part for your list.
 * @member: the name of the list_struct within the struct.
 */
#define __yf_list_for_each_entry(pos, part, member, dirct) \
        for (pos = yf_list_entry((part)->dirct, typeof(*pos), member);  \
             &pos->member != (part);    \
             pos = yf_list_entry(pos->member.dirct, typeof(*pos), member))

#define yf_list_for_each_entry(pos, part, member)              \
        __yf_list_for_each_entry(pos, part, member, next)

#define yf_list_for_each_entry_r(pos, part, member)          \
        __yf_list_for_each_entry(pos, part, member, prev)
        
/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop counter.
 * @n:      another type * to use as temporary storage
 * @part:   the part for your list.
 * @member: the name of the list_struct within the struct.
 */
#define __yf_list_for_each_entry_safe(pos, n, part, member, dirct)          \
        for (pos = yf_list_entry((part)->dirct, typeof(*pos), member),  \
             n = yf_list_entry(pos->member.dirct, typeof(*pos), member); \
             &pos->member != (part);                    \
             pos = n, n = yf_list_entry(n->member.dirct, typeof(*n), member))

#define yf_list_for_each_entry_safe(pos, n, part, member) \
        __yf_list_for_each_entry_safe(pos, n, part, member, next)

#define yf_list_for_each_entry_safe_r(pos, n, part, member) \
        __yf_list_for_each_entry_safe(pos, n, part, member, prev)

#endif
