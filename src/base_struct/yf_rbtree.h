#ifndef _YF_RBTREE_H
#define _YF_RBTREE_H

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

//must check if is empty

typedef struct yf_rbtree_node_s yf_rbtree_node_t;

struct yf_rbtree_node_s
{
        yf_rbtree_node_t *left;
        yf_rbtree_node_t *right;
        yf_rbtree_node_t *parent;
        char              color;
        char              padding[YF_SIZEOF_PTR - 1];
};


typedef struct yf_rbtree_s yf_rbtree_t;

typedef yf_int_t (*yf_cmp_pt)(void *left, void *right, yf_int_t cmp_by_node);

struct yf_rbtree_s
{
        yf_rbtree_node_t *root;
        yf_rbtree_node_t *nil;
        yf_cmp_pt         cmp;
};

/* a nil must be black */
#define yf_rbtree_nil_init(node)  yf_rbt_black(node)

#define yf_rbtree_init(tree, s, i) do {                                          \
                yf_rbtree_nil_init(s);                                              \
                (tree)->root = s;                                                         \
                (tree)->nil = s;                                                     \
                (tree)->cmp = i;                                                \
} while (0)


#define yf_rbt_red(node)               ((node)->color = 1)
#define yf_rbt_black(node)             ((node)->color = 0)
#define yf_rbt_is_red(node)            ((node)->color)
#define yf_rbt_is_black(node)          (!yf_rbt_is_red(node))
#define yf_rbt_copy_color(n1, n2)      (n1->color = n2->color)

#define yf_rbt_is_empty(tree) (tree->root == tree->nil)

yf_int_t 
__yf_rbtree_insert(yf_rbtree_t *tree, yf_rbtree_node_t *node, yf_int_t uniq);

#define yf_rbtree_insert(tree, node)  __yf_rbtree_insert(tree, node, 0)
#define yf_rbtree_insert_uniq(tree, node)  __yf_rbtree_insert(tree, node, 1)

void yf_rbtree_delete(yf_rbtree_t *tree, yf_rbtree_node_t *node);


static inline yf_rbtree_node_t *
__yf_rbtree_min(yf_rbtree_node_t *node, yf_rbtree_node_t *nil)
{
        while (node->left != nil)
                node = node->left;
        return node;
}

static inline yf_rbtree_node_t *
__yf_rbtree_max(yf_rbtree_node_t *node, yf_rbtree_node_t *nil)
{
        while (node->right != nil)
                node = node->right;
        return node;
}

#define yf_rbtree_min(tree) __yf_rbtree_min(tree->root, tree->nil)
#define yf_rbtree_max(tree) __yf_rbtree_max(tree->root, tree->nil)

static inline yf_rbtree_node_t *
yf_rbtree_next(yf_rbtree_node_t *node, yf_rbtree_node_t *nil)
{
        if (node == nil)
                return node;
        
        if (node->right != nil)
        {
                node = node->right;
                while (node->left != nil)
                        node = node->left;
        }
        else {
                yf_rbtree_node_t* y = node->parent;
                while (y && node == y->right)
                {
                        node = y;
                        if (node == nil)
                                return  node;
                        y = y->parent;
                }
                if (y == NULL)
                        return nil;
                if (node->right != y)
                        node = y;
        }

        return node;
}

static inline yf_rbtree_node_t *
yf_rbtree_prev(yf_rbtree_node_t *node, yf_rbtree_node_t *nil)
{
        if (node == nil)
                return node;
        
        if (yf_rbt_is_red(node) && node->parent->parent == node) 
        {
                node = node->right;
        }
        else if (node->left != nil)
        {
                yf_rbtree_node_t* y = node->left;
                while (y->right != nil)
                        y = y->right;
                node = y;
        }
        else {
                yf_rbtree_node_t* y = node->parent;
                while (y && node == y->left)
                {
                        node = y;
                        if (node == nil)
                                return  node;       
                        y = y->parent;
                }
                if (y == NULL)
                        return nil;                
                node = y;
        }
        return  node;
}


#define yf_rbtree_find(tree, type, n_member, k_member, k_val, n_res) \
do {\
        yf_rbtree_node_t* __y = NULL; \
        yf_rbtree_node_t* __x = tree->root; \
        while (__x != tree->nil) { \
                if (!tree->cmp((void*)container_of(__x, type, n_member)->k_member, (void*)k_val, 0)) \
                        __y = __x, __x = __x->left; \
                else \
                        __x = __x->right; \
        } \
        n_res = ((__y == NULL || tree->cmp((void*)k_val, (void*)container_of(__y, type, n_member)->k_member, 0)) \
                ? NULL : __y); \
} while (0)


#define yf_rbtree_for_each(pos, tree) \
        for (pos = yf_rbtree_min(tree); pos != (tree)->nil; \
                pos = yf_rbtree_next(pos, (tree)->nil))

#define yf_rbtree_for_each_r(pos, tree) \
        for (pos = yf_rbtree_max(tree); pos != (tree)->nil; \
                pos = yf_rbtree_prev(pos, (tree)->nil))

#define yf_rbtree_for_each_safe(pos, n, tree) \
        for (pos = yf_rbtree_min(tree), n = yf_rbtree_next(pos, (tree)->nil); \
                pos != (tree)->nil; \
                pos = n, n = yf_rbtree_next(pos, (tree)->nil))

#define yf_rbtree_for_each_safe_r(pos, n, tree) \
        for (pos = yf_rbtree_max(tree), n = yf_rbtree_prev(pos, (tree)->nil); \
                pos != (tree)->nil; \
                pos = n, n = yf_rbtree_prev(pos, (tree)->nil))


#endif
