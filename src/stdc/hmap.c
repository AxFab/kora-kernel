/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kora/hmap.h>
#include <string.h>
#include <assert.h>

void *malloc(size_t);
void free(void *);

#undef POW2
#define POW2(v)  (((v)&((v)-1))==0)

#define HMAP_FULL_RATIO  3

struct hnode {
    hnode_t *next;
    void *value;
    uint32_t hash;
    int lg;
    char key[0];
};

int murmur3_32(const void *key, int bytes, uint32_t seed)
{
    static int rem_mask[] = { 0, 0xFF, 0xFFFF, 0xFFFFFF };
    int len = bytes >> 2;
    uint32_t k, hash = seed;
    const uint32_t *input = (const uint32_t *)key;

    while (len-- > 0) {
        k = *input;
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        hash ^= k;
        hash = (k << 13) | (k >> 19);
        hash = hash * 5 + 0xe6546b64;
        ++input;
    }

    if (bytes & 3) {
        k = (*input) & rem_mask[bytes & 3]; // TODO work only on little endian machine
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        hash ^= k;
    }

    hash ^= bytes;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);
    return hash;
}

void *memdup(const void *buf, size_t lg)
{
    void *ptr = malloc(lg);
    memcpy(ptr, buf, lg);
    return ptr;
}

void hmp_init(hmap_t *map, int lg)
{
    assert(POW2(lg));
    map->mask = lg - 1;
    map->hashes = (hnode_t **)malloc(sizeof(hnode_t *) * lg);
    memset(map->hashes, 0, sizeof(hnode_t *) * lg);
    map->seed = 0xa5a5a5a5;
    map->count = 0;
}

void hmp_destroy(hmap_t *map)
{
    int lg = map->mask + 1;
    while (lg-- > 0) {
        while (map->hashes[lg] != NULL) {
            hnode_t *entry = map->hashes[lg];
            map->hashes[lg] = entry->next;
            free(entry);
            --map->count;
        }
    }
    free(map->hashes);
    memset(map, 0, sizeof(*map));
}

static void hmap_grow(hmap_t *map)
{
    hnode_t *entry;
    int i, lg = (map->mask + 1) * 2;
    map->mask = lg - 1;
    hnode_t **htable = (hnode_t **)malloc(sizeof(hnode_t *) * lg);
    memset(htable, 0, sizeof(hnode_t *) * lg);
    for (i = 0; i < lg / 2; ++i) {
        while (map->hashes[i]) {
            entry = map->hashes[i];
            map->hashes[i] = entry->next;
            entry->next = htable[entry->hash & map->mask];
            htable[entry->hash & map->mask] = entry;
        }
    }
    free(map->hashes);
    map->hashes = htable;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void hmp_put(hmap_t *map, const char *key, int lg, void *value)
{
    uint32_t hash = murmur3_32(key, lg, map->seed);
    hnode_t *entry = map->hashes[hash & map->mask];
    while (entry != NULL) {
        if (entry->lg == lg && memcmp(entry->key, key, lg) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    if (map->count > HMAP_FULL_RATIO * (map->mask + 1))
        hmap_grow(map);
    entry = (hnode_t *)malloc(sizeof(hnode_t) + lg);
    memcpy(entry->key, key, lg);
    entry->lg = lg;
    entry->hash = hash;
    entry->value = value;
    entry->next = map->hashes[hash & map->mask];
    map->hashes[hash & map->mask] = entry;
    ++map->count;
}

void *hmp_get(hmap_t *map, const char *key, int lg)
{
    uint32_t hash = murmur3_32(key, lg, map->seed);
    hnode_t *entry = map->hashes[hash & map->mask];
    while (entry != NULL) {
        if (entry->lg == lg && memcmp(entry->key, key, lg) == 0)
            return entry->value;
        entry = entry->next;
    }
    return NULL;
}

void hmp_remove(hmap_t *map, const char *key, int lg)
{
    uint32_t hash = murmur3_32(key, lg, map->seed);
    hnode_t *entry = map->hashes[hash & map->mask];
    if (entry == NULL)
        return;
    if (entry->lg == lg && memcmp(entry->key, key, lg) == 0) {
        map->hashes[hash & map->mask] = entry->next;
        --map->count;
        free(entry);
        return;
    }
    while (entry->next != NULL) {
        if (entry->next->lg == lg && memcmp(entry->next->key, key, lg) == 0) {
            hnode_t *tmp = entry->next;
            entry->next = entry->next->next;
            --map->count;
            free(tmp);
            return;
        }
        entry = entry->next;
    }
}
