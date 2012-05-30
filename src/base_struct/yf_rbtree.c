#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

static inline void yf_rbtree_left_rotate(yf_rbtree_node_t **root
        , yf_rbtree_node_t *nil
        , yf_rbtree_node_t *node);
static inline void yf_rbtree_right_rotate(yf_rbtree_node_t **root
        , yf_rbtree_node_t *nil
        , yf_rbtree_node_t *node);
static yf_int_t  yf_rbtree_insert_value(yf_rbtree_node_t *temp
        , yf_rbtree_node_t *node
        , yf_rbtree_node_t *nil
        , yf_cmp_pt cmp_pt
        , yf_int_t uniq);

yf_int_t
__yf_rbtree_insert(yf_rbtree_t *tree, yf_rbtree_node_t *node, yf_int_t uniq)
{
        yf_rbtree_node_t **root, *temp, *nil;

        /* a binary tree insert */

        root = (yf_rbtree_node_t **)&tree->root;
        nil = tree->nil;

        if (*root == nil)
        {
                node->parent = NULL;
                node->left = nil;
                node->right = nil;
                yf_rbt_black(node);
                *root = node;

                return YF_OK;
        }

        CHECK_OK(yf_rbtree_insert_value(*root, node, nil, tree->cmp, uniq));

        /* re-balance tree */

        while (node != *root && yf_rbt_is_red(node->parent))
        {
                if (node->parent == node->parent->parent->left)
                {
                        temp = node->parent->parent->right;

                        if (yf_rbt_is_red(temp))
                        {
                                yf_rbt_black(node->parent);
                                yf_rbt_black(temp);
                                yf_rbt_red(node->parent->parent);
                                node = node->parent->parent;
                        }
                        else {
                                if (node == node->parent->right)
                                {
                                        node = node->parent;
                                        yf_rbtree_left_rotate(root, nil, node);
                                }

                                yf_rbt_black(node->parent);
                                yf_rbt_red(node->parent->parent);
                                yf_rbtree_right_rotate(root, nil, node->parent->parent);
                        }
                }
                else {
                        temp = node->parent->parent->left;

                        if (yf_rbt_is_red(temp))
                        {
                                yf_rbt_black(node->parent);
                                yf_rbt_black(temp);
                                yf_rbt_red(node->parent->parent);
                                node = node->parent->parent;
                        }
                        else {
                                if (node == node->parent->left)
                                {
                                        node = node->parent;
                                        yf_rbtree_right_rotate(root, nil, node);
                                }

                                yf_rbt_black(node->parent);
                                yf_rbt_red(node->parent->parent);
                                yf_rbtree_left_rotate(root, nil, node->parent->parent);
                        }
                }
        }

        yf_rbt_black(*root);
        return  YF_OK;
}


void
yf_rbtree_delete(yf_rbtree_t *tree, yf_rbtree_node_t *node)
{
        yf_uint_t red;
        yf_rbtree_node_t **root, *nil, *subst, *temp, *w;

        /* a binary tree delete */

        root = (yf_rbtree_node_t **)&tree->root;
        nil = tree->nil;

        if (node->left == nil)
        {
                temp = node->right;
                subst = node;
        }
        else if (node->right == nil)
        {
                temp = node->left;
                subst = node;
        }
        else {
                subst = __yf_rbtree_min(node->right, nil);

                if (subst->left != nil)
                {
                        temp = subst->left;
                }
                else {
                        temp = subst->right;
                }
        }

        if (subst == *root)
        {
                *root = temp;
                yf_rbt_black(temp);

                /* DEBUG stuff */
                node->left = NULL;
                node->right = NULL;
                node->parent = NULL;

                return;
        }

        red = yf_rbt_is_red(subst);

        if (subst == subst->parent->left)
        {
                subst->parent->left = temp;
        }
        else {
                subst->parent->right = temp;
        }

        if (subst == node)
        {
                temp->parent = subst->parent;
        }
        else {
                if (subst->parent == node)
                {
                        temp->parent = subst;
                }
                else {
                        temp->parent = subst->parent;
                }

                subst->left = node->left;
                subst->right = node->right;
                subst->parent = node->parent;
                yf_rbt_copy_color(subst, node);

                if (node == *root)
                {
                        *root = subst;
                }
                else {
                        if (node == node->parent->left)
                        {
                                node->parent->left = subst;
                        }
                        else {
                                node->parent->right = subst;
                        }
                }

                if (subst->left != nil)
                {
                        subst->left->parent = subst;
                }

                if (subst->right != nil)
                {
                        subst->right->parent = subst;
                }
        }

        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;

        if (red)
        {
                return;
        }

        /* a delete fixup */

        while (temp != *root && yf_rbt_is_black(temp))
        {
                if (temp == temp->parent->left)
                {
                        w = temp->parent->right;

                        if (yf_rbt_is_red(w))
                        {
                                yf_rbt_black(w);
                                yf_rbt_red(temp->parent);
                                yf_rbtree_left_rotate(root, nil, temp->parent);
                                w = temp->parent->right;
                        }

                        if (yf_rbt_is_black(w->left) && yf_rbt_is_black(w->right))
                        {
                                yf_rbt_red(w);
                                temp = temp->parent;
                        }
                        else {
                                if (yf_rbt_is_black(w->right))
                                {
                                        yf_rbt_black(w->left);
                                        yf_rbt_red(w);
                                        yf_rbtree_right_rotate(root, nil, w);
                                        w = temp->parent->right;
                                }

                                yf_rbt_copy_color(w, temp->parent);
                                yf_rbt_black(temp->parent);
                                yf_rbt_black(w->right);
                                yf_rbtree_left_rotate(root, nil, temp->parent);
                                temp = *root;
                        }
                }
                else {
                        w = temp->parent->left;

                        if (yf_rbt_is_red(w))
                        {
                                yf_rbt_black(w);
                                yf_rbt_red(temp->parent);
                                yf_rbtree_right_rotate(root, nil, temp->parent);
                                w = temp->parent->left;
                        }

                        if (yf_rbt_is_black(w->left) && yf_rbt_is_black(w->right))
                        {
                                yf_rbt_red(w);
                                temp = temp->parent;
                        }
                        else {
                                if (yf_rbt_is_black(w->left))
                                {
                                        yf_rbt_black(w->right);
                                        yf_rbt_red(w);
                                        yf_rbtree_left_rotate(root, nil, w);
                                        w = temp->parent->left;
                                }

                                yf_rbt_copy_color(w, temp->parent);
                                yf_rbt_black(temp->parent);
                                yf_rbt_black(w->left);
                                yf_rbtree_right_rotate(root, nil, temp->parent);
                                temp = *root;
                        }
                }
        }

        yf_rbt_black(temp);
}


static yf_int_t yf_rbtree_insert_value(yf_rbtree_node_t *temp
        , yf_rbtree_node_t *node
        , yf_rbtree_node_t *nil
        , yf_cmp_pt cmp_pt
        , yf_int_t uniq)
{
        yf_rbtree_node_t **p;
        yf_int_t  cmp_ret;

        for (;; )
        {
                cmp_ret = cmp_pt(node, temp, 1);
                if (uniq)
                {
                        if (cmp_ret == cmp_pt(temp, node, 1)) {
                                yf_errno = YF_EEXIST;
                                return  YF_ERROR;
                        }
                }

                p = cmp_ret ? &temp->left : &temp->right;

                if (*p == nil)
                {
                        break;
                }

                temp = *p;
        }

        *p = node;
        node->parent = temp;
        node->left = nil;
        node->right = nil;
        yf_rbt_red(node);

        return YF_OK;
}


static inline void
yf_rbtree_left_rotate(yf_rbtree_node_t **root
        , yf_rbtree_node_t *nil
        , yf_rbtree_node_t *node)
{
        yf_rbtree_node_t *temp;

        temp = node->right;
        node->right = temp->left;

        if (temp->left != nil)
        {
                temp->left->parent = node;
        }

        temp->parent = node->parent;

        if (node == *root)
        {
                *root = temp;
        }
        else if (node == node->parent->left)
        {
                node->parent->left = temp;
        }
        else {
                node->parent->right = temp;
        }

        temp->left = node;
        node->parent = temp;
}


static inline void
yf_rbtree_right_rotate(yf_rbtree_node_t **root
        , yf_rbtree_node_t *nil
        , yf_rbtree_node_t *node)
{
        yf_rbtree_node_t *temp;

        temp = node->left;
        node->left = temp->right;

        if (temp->right != nil)
        {
                temp->right->parent = node;
        }

        temp->parent = node->parent;

        if (node == *root)
        {
                *root = temp;
        }
        else if (node == node->parent->right)
        {
                node->parent->right = temp;
        }
        else {
                node->parent->left = temp;
        }

        temp->right = node;
        node->parent = temp;
}
