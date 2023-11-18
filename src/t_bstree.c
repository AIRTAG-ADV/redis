#include "server.h"

/*-----------------------------------------------------------------------------
 * Binary Search Tree API
 * Assignment of 2023 HNU CE Embedded System (Prof. Joonhyuk Jang)
 * Team AIRTAG - Haechan Moon, Kyungwon Ko, Sangmyoung Lee
 *----------------------------------------------------------------------------*/

struct BSTreeNode {
    sds value;
    int hash;
    struct BSTreeNode *left;
    struct BSTreeNode *right;
};

/* Simple hashing algorithm from Java String API's hashCode() */
int sdsjavahash(sds c) {
    int h = 0;
    for (; *c != '\0'; c++) h = h * 31 + *c;
    return h;
}

/* Allocate memory of root node's pointer */
robj *createBSTreeObject(void) {
    struct BSTreeNode **root = (struct BSTreeNode **)zmalloc(sizeof(struct BSTreeNode *));
    *root = NULL;

    robj *o = createObject(OBJ_BSTREE, root);
    o->encoding = OBJ_ENCODING_BSTREE;

    return o;
}

/* Create root node or put as child node */
int bstreeTypeSet(robj *o, sds value) {
    struct BSTreeNode *newNode;
    struct BSTreeNode *node;

    newNode = (struct BSTreeNode *)zmalloc(sizeof(struct BSTreeNode));
    newNode->value = sdsdup(value);
    newNode->hash = sdsjavahash(value);
    newNode->left = NULL;
    newNode->right = NULL;

    if (!(*(struct BSTreeNode **)o->ptr)) {
        *(struct BSTreeNode **)o->ptr = newNode;

        return 1;
    }

    node = *(struct BSTreeNode **)o->ptr;

    while (node) {
        if (newNode->hash < node->hash) {
            if (!node->left) {
                node->left = newNode;
                return 1;
            }

            node = node->left;
        } else if (newNode->hash > node->hash) {
            if (!node->right) {
                node->right = newNode;
                return 1;
            }

            node = node->right;
        } else {
            zfree(newNode);
            return 0;
        }
    }

    return 0;
}

/* Find target node */
sds bstreeTypeGet(robj *o, sds value) {
    struct BSTreeNode *node;
    int hash;

    if (!o->ptr || !(*(struct BSTreeNode **)o->ptr)) return NULL;

    node = *(struct BSTreeNode **)o->ptr;
    hash = sdsjavahash(value);

    while (node) {
        if (hash < node->hash) node = node->left;
        else if (hash > node->hash) node = node->right;
        else return node->value;
    }

    return NULL;
}

/* Lookup BSTREE from DB or create it */
robj *bstreeTypeLookupWriteOrCreate(client *c, robj *key) {
    robj *o = lookupKeyWrite(c->db, key);
    if (checkType(c, o, OBJ_BSTREE)) return NULL;

    if (!o) {
        o = createBSTreeObject();
        dbAdd(c->db, key, o);
    }

    return o;
}

void bssetCommand(client *c) {
    robj *o;
    int i, cnt = 0;

    if (!(o = bstreeTypeLookupWriteOrCreate(c, c->argv[1]))) return;
    for (i = 2; i < c->argc; i++) cnt += bstreeTypeSet(o, (sds)c->argv[i]->ptr);

    addReplyLongLong(c, cnt);
    signalModifiedKey(c, c->db, c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_BSTREE, "bsset", c->argv[1], c->db->id);

    server.dirty += c->argc - 2;
}

void bsgetCommand(client *c) {
    robj *o;
    sds value;

    if ((o = lookupKeyReadOrReply(c, c->argv[1], shared.null[c->resp])) == NULL || checkType(c, o, OBJ_BSTREE)) return;

    if ((value = bstreeTypeGet(o, (sds)c->argv[2]->ptr))) addReplyBulkCBuffer(c, value, sdslen(value));
    else addReplyNull(c);
}
