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
 *
 *      String Hash-map implementation.
 */
#ifndef _KORA_HMAP_H
#define _KORA_HMAP_H 1

#include <stddef.h>
#include <stdint.h>

typedef struct HMP_map HMP_map;
typedef struct HMP_entry HMP_entry;

struct HMP_map {
    HMP_entry **hashes_;
    uint32_t mask_;
    uint32_t seed_;
    uint32_t count_;
};

int murmur3_32(const void *key, int bytes, uint32_t seed);

void hmp_init(HMP_map *map, int lg);
void hmp_destroy(HMP_map *map, int all);

void hmp_put(HMP_map *map, const char *key, int lg, void *value);
void *hmp_get(HMP_map *map, const char *key, int lg);
void hmp_remove(HMP_map *map, const char *key, int lg);

#endif  /* _KORA_HMAP_H */
