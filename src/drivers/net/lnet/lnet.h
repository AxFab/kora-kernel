/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#ifndef _SRC_LNET_H
#define _SRC_LNET_H 1

#include <kernel/net.h>

typedef struct lnet_dev lnet_dev_t;
typedef struct lnet_msg lnet_msg_t;
struct lnet_dev {
    netdev_t n;
    int fd;
};

struct lnet_msg {
    int request;
    int length;
};

#define MR_INIT 1
#define MR_PACKET 2
#define MR_LINK 3
#define MR_UNLINK 4


#endif  /* _SRC_LNET_H */
