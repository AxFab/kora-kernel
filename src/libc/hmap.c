/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
// #include <stdlib.h>
#include <string.h>
#include <assert.h>

void *malloc(size_t);
void free(void *);

#undef POW2
#define POW2(v)  (((v)&((v)-1))==0)

#define HMAP_FULL_RATIO  3

struct HMP_entry {
    int lg_;
    uint32_t hash_;
    HMP_entry *next_;
    void *value_;
    char *key_;
};

static void hmap_grow(HMP_map *map)
{
    HMP_entry *entry;
    int i, lg = (map->mask_ + 1) * 2;
    map->mask_ = lg - 1;
    HMP_entry **htable = (HMP_entry **)malloc(sizeof(HMP_entry *) * lg);
    memset(htable, 0, sizeof(HMP_entry *) * lg);
    for (i = 0; i < lg / 2; ++i) {
        while (map->hashes_[i]) {
            entry = map->hashes_[i];
            map->hashes_[i] = entry->next_;
            entry->next_ = htable[entry->hash_ & map->mask_];
            htable[entry->hash_ & map->mask_] = entry;
        }
    }
    free(map->hashes_);
    map->hashes_ = htable;
}

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

void *memdup(const char *buf, int lg)
{
    void *ptr = malloc(lg);
    memcpy(ptr, buf, lg);
    return ptr;
}

void hmp_init(HMP_map *map, int lg)
{
    assert(POW2(lg));
    map->mask_ = lg - 1;
    map->hashes_ = (HMP_entry **)malloc(sizeof(HMP_entry *) * lg);
    memset(map->hashes_, 0, sizeof(HMP_entry *) * lg);
    map->seed_ = 0xa5a5a5a5;
    map->count_ = 0;
}

void hmp_destroy(HMP_map *map, int all)
{
    int lg = all ? map->mask_ + 1 : 0;
    while (lg-- > 0) {
        while (map->hashes_[lg] != NULL) {
            HMP_entry *entry = map->hashes_[lg];
            map->hashes_[lg] = entry->next_;
            free(entry->key_);
            free(entry);
            --map->count_;
        }
    }
    free(map->hashes_);
    memset(map, 0, sizeof(*map));
}

void hmp_put(HMP_map *map, const char *key, int lg, void *value)
{
    uint32_t hash = murmur3_32(key, lg, map->seed_);
    HMP_entry *entry = map->hashes_[hash & map->mask_];
    while (entry != NULL) {
        if (entry->lg_ == lg && memcmp(entry->key_, key, lg) == 0) {
            entry->value_ = value;
            return;
        }
        entry = entry->next_;
    }
    if (map->count_ > HMAP_FULL_RATIO * (map->mask_ + 1))
        hmap_grow(map);
    entry = (HMP_entry *)malloc(sizeof(HMP_entry));
    entry->key_ = memdup(key, lg);
    entry->lg_ = lg;
    entry->hash_ = hash;
    entry->value_ = value;
    entry->next_ = map->hashes_[hash & map->mask_];
    map->hashes_[hash & map->mask_] = entry;
    ++map->count_;
}

void *hmp_get(HMP_map *map, const char *key, int lg)
{
    uint32_t hash = murmur3_32(key, lg, map->seed_);
    HMP_entry *entry = map->hashes_[hash & map->mask_];
    while (entry != NULL) {
        if (entry->lg_ == lg && memcmp(entry->key_, key, lg) == 0)
            return entry->value_;
        entry = entry->next_;
    }
    return NULL;
}

void hmp_remove(HMP_map *map, const char *key, int lg)
{
    uint32_t hash = murmur3_32(key, lg, map->seed_);
    HMP_entry *entry = map->hashes_[hash & map->mask_];
    if (entry == NULL)
        return;
    if (entry->lg_ == lg && memcmp(entry->key_, key, lg) == 0) {
        map->hashes_[hash & map->mask_] = entry->next_;
        --map->count_;
        free(entry->key_);
        free(entry);
        return;
    }
    while (entry->next_ != NULL) {
        if (entry->next_->lg_ == lg && memcmp(entry->next_->key_, key, lg) == 0) {
            HMP_entry *tmp = entry->next_;
            entry->next_ = entry->next_->next_;
            --map->count_;
            free(tmp->key_);
            free(tmp);
            return;
        }
        entry = entry->next_;
    }
}
