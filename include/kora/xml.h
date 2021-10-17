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
#ifndef _KORA_XML_H
#define _KORA_XML_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XML_BUF_SIZE 8192
#define XML_BUF_XS_SIZE 512

/* List XML node types */
#define XML_ELEMENT 1
#define XML_ATTRIBUTE 2
#define XML_TEXT 3
#define XML_CDATA 4
// 5   ENTITY_REFERENCE_NODE
// 6   ENTITY_NODE
#define XML_DECLARATION 7
#define XML_COMMENT 8
#define XML_DOCUMENT 9
#define XML_DOCTYPE 10
// 11  DOCUMENT_FRAGMENT_NODE
// 12  NOTATION_NODE

/* Create bulding/parsing flags */
#define XML_BLD_CLOSING  (1 << 0)
#define XML_BLD_CLOSED  (1 << 1)

/* List errors */
#define XML_ERR 1 // "Undefined error"
#define XML_ERR_CLOSING 2 // "Wrong closing type"
#define XML_ERR_FOUND_AFTER 3 //"Element found after the root element"
#define XML_ERR_BUFFER_TOO_SMALL 4

/* Define types */
typedef struct XML_node XML_node;
typedef struct XML_attribute XML_attribute;
typedef struct XML_builder XML_builder;

struct XML_node {
    int type;
    int length;
    int children_count;
    char *litteral;
    XML_node *parent;
    XML_node *next_sibling;
    XML_node *previous_sibling;
    XML_node *first_child;
    XML_node *last_child;
    XML_attribute *first_attribute;
    XML_attribute *last_attribute;
    char *node_name;
    char *content;
    int build_flags;
    int row;
    int col;
    int rows;
    int ecol;
};

struct XML_attribute {
    char *key;
    char *value;
    XML_attribute *next;
    XML_attribute *previous;
};

typedef XML_node *(*XML_pusher)(XML_node *cursor, XML_node *node, void *param);


/* Build XML document */
void xml_add_attribute(XML_node *node, const char *key, const char *value);
XML_node *xml_create_node(int type, const char *name, const char *content);
void xml_add_child_node(XML_node *node, XML_node *child);
void xml_remove_node(XML_node *node);

/* Parse XML sub-strings */
void xml_parse_attributes(XML_node *node, const char *buf, int lg);
XML_node *xml_parse_node(int nodetype, const char *data, int lg);

/* Serialization of XML DOM */
XML_node *xml_read_file(FILE *stream);
XML_node *xml_read_file_with(FILE *stream, XML_pusher pusher, void *param);
void xml_write_file(FILE *stream, XML_node *node, int opt);

/* Free all memory block occupied for the DOM */
void xml_free_node(XML_node *node);


#endif  /* _KORA_XML_H */
