#include "user/user.h"

typedef struct NodeStruct {
    struct NodeStruct* left;
    struct NodeStruct* right;
    int value;
} Node;

static void add(Node** bst_ptr, int value) {
    if (!bst_ptr) return;

    while (*bst_ptr) {
        if (value < (*bst_ptr)->value) {
            bst_ptr = &(*bst_ptr)->left;
        } else if (value > (*bst_ptr)->value) {
            bst_ptr = &(*bst_ptr)->right;
        } else {
            return;
        }
    }

    Node* node = nmalloc(sizeof(Node));
    if (!node) return;

    node->left = 0;
    node->right = 0;
    node->value = value;

    *bst_ptr = node;
}

static void remove_node(Node** bst_ptr, int value) {
    if (!bst_ptr) return;

    while (*bst_ptr && (*bst_ptr)->value != value) {
        if (value < (*bst_ptr)->value) {
            bst_ptr = &(*bst_ptr)->left;
        } else {
            bst_ptr = &(*bst_ptr)->right;
        }
    }

    if (!*bst_ptr) return;

    Node* node = *bst_ptr;

    if (!node->left) {
        *bst_ptr = node->right;
        nfree(node);
    }
    else if (!node->right) {
        *bst_ptr = node->left;
        nfree(node);
    }
    else {
        Node** successor_ptr = &node->right;

        while ((*successor_ptr)->left) {
            successor_ptr = &(*successor_ptr)->left;
        }

        Node* successor = *successor_ptr;

        node->value = successor->value;

        *successor_ptr = successor->right;
        nfree(successor);
    }
}

static void clear(Node** bst_ptr) {
    if (!bst_ptr || !*bst_ptr) return;

    Node* node = *bst_ptr;

    clear(&node->left);
    clear(&node->right);

    nfree(node);
    *bst_ptr = 0;
}

static void print(Node* bst, int level) {
    if (!bst) return;

    printf("Level %d: %d\n", level, bst->value);

    print(bst->left, level + 1);
    print(bst->right, level + 1);
}

int main(void) {
    Node* bst = 0;

    add(&bst, 10);
    add(&bst, 5);
    add(&bst, 15);

    print(bst, 0);

    remove_node(&bst, 5);

    print(bst, 0);

    clear(&bst);

    return 0;
}
