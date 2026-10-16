#include "nnsys/g3d.h"

// NitroSystem's G3D resource accessors (res_struct_accessor.c): finding the entry of a dictionary by its name, and the
// blocks of a resource file. The functions have NitroSystem's names, and g3d.h notes swan's

// A dictionary of fewer entries than this is searched in order, a larger one through its Patricia tree
#define RESDICT_LINEARSEARCH_MAX 16

// Returns the data of the entry of the name, or NULL. The name is a key of four words, which the search compares with
// the names of the entries
void *NNS_G3dGetResDataByName(const NNSG3dResDict *dict, const NNSG3dResName *name) {
    if (name == NULL) {
        return NULL;
    }

    if (dict->numEntry < RESDICT_LINEARSEARCH_MAX) {
        u32 w0 = name->val[0];
        u32 w1 = name->val[1];
        u32 w2 = name->val[2];
        u32 w3 = name->val[3];
        u32 i;

        for (i = 0; i < dict->numEntry; i++) {
            const NNSG3dResName *entry = NNS_G3dGetResNameByIdx(dict, i);

            if (entry->val[0] == w0 && entry->val[1] == w1 && entry->val[2] == w2 && entry->val[3] == w3) {
                return NNS_G3dGetResDataByIdx(dict, i);
            }
        }
    } else {
        const NNSG3dResDictTreeNode *tree = dict->node;

        if (tree->idxChild[0] != 0) {
            const NNSG3dResDictTreeNode *prev = tree;
            const NNSG3dResDictTreeNode *node = &tree[prev->idxChild[0]];
            const NNSG3dResName *entry;

            while (prev->refBit > node->refBit) {
                u32 bit = (name->val[node->refBit >> 5] >> (node->refBit & 31)) & 1;

                prev = node;
                node = &tree[node->idxChild[bit]];
            }
            entry = NNS_G3dGetResNameByIdx(dict, node->idxEntry);
            if (entry->val[0] == name->val[0] && entry->val[1] == name->val[1] && entry->val[2] == name->val[2] &&
                entry->val[3] == name->val[3]) {
                return NNS_G3dGetResDataByIdx(dict, node->idxEntry);
            }
        }
    }
    return NULL;
}

// The same search for the index of the entry, or -1
int NNS_G3dGetResDictIdxByName(const NNSG3dResDict *dict, const NNSG3dResName *name) {
    if (name == NULL) {
        return -1;
    }

    if (dict->numEntry < RESDICT_LINEARSEARCH_MAX) {
        u32 w0 = name->val[0];
        u32 w1 = name->val[1];
        u32 w2 = name->val[2];
        u32 w3 = name->val[3];
        u32 i;

        for (i = 0; i < dict->numEntry; i++) {
            const NNSG3dResName *entry = NNS_G3dGetResNameByIdx(dict, i);

            if (entry->val[0] == w0 && entry->val[1] == w1 && entry->val[2] == w2 && entry->val[3] == w3) {
                return i;
            }
        }
    } else {
        const NNSG3dResDictTreeNode *tree = dict->node;

        if (tree->idxChild[0] != 0) {
            const NNSG3dResDictTreeNode *prev = tree;
            const NNSG3dResDictTreeNode *node = &tree[prev->idxChild[0]];
            const NNSG3dResName *entry;

            while (prev->refBit > node->refBit) {
                u32 bit = (name->val[node->refBit >> 5] >> (node->refBit & 31)) & 1;

                prev = node;
                node = &tree[node->idxChild[bit]];
            }
            entry = NNS_G3dGetResNameByIdx(dict, node->idxEntry);
            if (entry->val[0] == name->val[0] && entry->val[1] == name->val[1] && entry->val[2] == name->val[2] &&
                entry->val[3] == name->val[3]) {
                return node->idxEntry;
            }
        }
    }
    return -1;
}

// The model block is the first block of a model file
NNSG3dResMdlSet *NNS_G3dGetMdlSet(const NNSG3dResFileHeader *header) {
    const u32 *blockOfs = (const u32 *)((const u8 *)header + header->headerSize);

    return (NNSG3dResMdlSet *)((u8 *)header + blockOfs[0]);
}

// The texture block is the only block of a texture file (BTX0), and the second one of a model file (BMD0). Any other
// file with one block has none
NNSG3dResTex *NNS_G3dGetTex(const NNSG3dResFileHeader *header) {
    const u32 *blockOfs = (const u32 *)((const u8 *)header + header->headerSize);

    if (header->dataBlocks == 1) {
        if (header->signature == NNS_G3D_SIGNATURE_BTX0) {
            return (NNSG3dResTex *)((u8 *)header + blockOfs[0]);
        }
        return NULL;
    }
    return (NNSG3dResTex *)((u8 *)header + blockOfs[1]);
}
