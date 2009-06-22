/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "cmd.h"
#include "parser.tab.h"

extern int yylex();
extern FILE *yyin;
extern char **environ;
extern YYSTYPE yylval;

cmd_t *cmds[MAX_CMDS];
int n_cmds = 0;

int sparse_pos = 0;
int sparse_buffsz = 0;
char *sparse_buff = 0;

int data_pos = 0;
int data_buffsz = 0;
char *data_buff = 0;


#define UNICODE_POINT 0x80000000
#define SPECIAL       0x40000000

typedef struct table_entry
{
    uint32_t flags; // UNICODE_POINT | SPECIAL | ALTGR_F | ALT_F | CTRL_F | SHIFT_F
    uint32_t val;   // If flags&UNICODE_POINT, U+val; else number giving special key.
} table_entry_t;

#define TABLE_IDX(alt, modifiers, escape, scancode) ( ((alt&3)<<10) | ((modifiers&3)<<8) | ((escape&1)<<7) | (scancode&0x7F) )

#define TABLE_MAX TABLE_IDX(2,3,1,128)

table_entry_t table[TABLE_MAX+1];

#define SPARSE_NULL 0
#define SPARSE_DATA_FLAG 0x8000

typedef struct sparse_entry
{
    uint16_t left;
    uint16_t right;
} sparse_t;

int sparse_add()
{
    int ret = sparse_pos;
    int sz = sizeof(sparse_t);
    if (sparse_pos+sz > sparse_buffsz)
    {
        if (sparse_pos+sz >= 32768)
        {
            fprintf(stderr, "Too many sparse nodes.\n");
            exit(1);
        }
        sparse_buff = realloc(sparse_buff, sparse_buffsz+sz+32);
        sparse_buffsz += sz+32;
    }
    sparse_pos += sz;

    return ret;
}

int data_add(table_entry_t *start, table_entry_t *end)
{
    int ret = data_pos;
    int sz = (unsigned long)end-(unsigned long)start;
    if (data_pos+sz > data_buffsz)
    {
        if (data_pos+sz >= 32768)
        {
            fprintf(stderr, "Too much data.\n");
            exit(1);
        }
        data_buff = realloc(data_buff, data_buffsz+sz+32);
        data_buffsz += sz+32;
    }

    memcpy(&data_buff[data_pos], (char*)start, sz);
    data_pos += sz;

    return ret;
}

int all_set(table_entry_t *start, table_entry_t *end)
{
    for (; start < end; start++)
    {
        if (start->flags == 0 && start->val == 0)
            return 0;
    }
    return 1;
}

int all_clear(table_entry_t *start, table_entry_t *end)
{
    for (; start < end; start++)
    {
        if (start->flags != 0 || start->val != 0)
            return 0;
    }
    return 1;
}

int yywrap()
{
}

int yyerror()
{
  printf("Syntax error!\n");
  return 1;
}

void usage()
{
    printf("keymap: Compile or install a keyboard mapping.\n\
Usage: keymap [compile|install] file \n");
}

void parse(char *filename)
{
    FILE *stream = fopen(filename, "r");
    if (!stream)
    {
        fprintf(stderr, "Unable to open file `%s'\n", filename);
        exit(1);
    }

    yyin = stream;
    yyparse();

    memset(table, 0, sizeof(table_entry_t)*(TABLE_MAX+1));

    int i;
    for (i = 0; i < n_cmds; i++)
    {
        int alt = NONE_I;
        int modifiers = cmds[i]->modifiers&0x3;

        if (cmds[i]->modifiers&ALT_F) alt = ALT_I;
        else if (cmds[i]->modifiers&ALTGR_F)
        {
            alt = ALTGR_I;
            printf("scancode: %x, modifiers: %x\n", cmds[i]->scancode, modifiers);}
        
        unsigned int idx = TABLE_IDX(alt, modifiers, cmds[i]->escape, cmds[i]->scancode);
        if (alt == ALTGR_I)
            printf("Index: %x\n", idx);
        table[idx].flags = cmds[i]->set_modifiers;
        if (cmds[i]->unicode_point != 0)
        {
            table[idx].flags |= UNICODE_POINT;
            table[idx].val = cmds[i]->unicode_point;
        }
        else if (cmds[i]->val != 0)
        {
            table[idx].flags |= SPECIAL;
            memcpy((char*)&(table[idx].val), cmds[i]->val, 4);
        }
    }
}

int sparse(int idx, int bisect_size)
{
    int ime = sparse_add();
    sparse_t *me = (sparse_t*)(&sparse_buff[ime]);

    // Check the right child - is everything zero?
    if (all_clear(&table[idx-bisect_size], &table[idx]) > 0)
    {
        // All zero, don't do anything with this child.
        me->left = SPARSE_NULL;
    }
    else if ((all_set(&table[idx-bisect_size], &table[idx]) > 0) || (bisect_size % 2))
    {
        printf("DATA: %x -> %x (L)\n", idx-bisect_size, idx);
        // All set, add to data section and set ptr.
        me->left = data_add(&table[idx-bisect_size], &table[idx]) | SPARSE_DATA_FLAG /* This is actual data, not another node. */;
    }
    else
    {
        uint16_t tmp = sparse(idx-bisect_size/2, bisect_size/2);
        // Buffer location may have changed with the me->left call.
        me = (sparse_t*)(&sparse_buff[ime]);
        me->left = tmp;
    }

    // Check the right child - is everything zero?
    if (all_clear(&table[idx], &table[idx+bisect_size]) > 0)
    {
        // All zero, don't do anything with this child.
        me->right = SPARSE_NULL;
    }
    else if ((all_set(&table[idx], &table[idx+bisect_size]) > 0) || (bisect_size % 2))
    {
        printf("DATA: %x -> %x (R)\n", idx, idx+bisect_size);
        // All set, add to data section and set ptr.
        me->right = data_add(&table[idx], &table[idx+bisect_size]) | SPARSE_DATA_FLAG /* This is actual data, not another node. */;
    }
    else
    {
        uint16_t tmp = sparse(idx+bisect_size/2, bisect_size/2);
        // Buffer location may have changed with the me->left call.
        me = (sparse_t*)(&sparse_buff[ime]);
        me->right = tmp;
    }

    return ime;
}

void compile(char *filename)
{
    parse(filename);

    // Table created, now create the sparse tree.
    sparse((TABLE_MAX+1)/2, (TABLE_MAX+1)/2);

    char *fname = (char*)malloc(strlen(filename)+3);
    strcpy(fname, filename);
    strcat(fname, ".kmc");

    // Write out.
    FILE *stream = fopen(fname, "w");
    if (!stream)
    {
        fprintf(stderr, "Unable to open output file `%s'\n", fname);
        exit(1);
    }

    int idx = 2*sizeof(int);
    fwrite(&idx, sizeof(idx), 1, stream);
    idx = sparse_buffsz+2*sizeof(int);

    fwrite(&idx, sizeof(idx), 1, stream);
    fwrite(sparse_buff, 1, sparse_buffsz, stream);
    fwrite(data_buff, 1, data_buffsz, stream);
    fclose(stream);

    printf("Compiled keymap file written to `%s'.\n", fname);

    strcpy(fname, filename);
    strcat(fname, ".h");

    // Write out.
    stream = fopen(fname, "w");
    if (!stream)
    {
        fprintf(stderr, "Unable to open output file `%s'\n", fname);
        exit(1);
    }

    // Change the filename to uppercase and underscored for the header guard.
    char *header_guard = strdup(fname);
    int i;
    for (i = 0; i < strlen(fname)+2; i++)
    {
        if (fname[i] == '.')
            header_guard[i] = '_';
        else
            header_guard[i] = toupper(fname[i]);
    }

    fprintf(stream, "/*\n\
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin\n\
 *\n\
 * Permission to use, copy, modify, and distribute this software for any\n\
 * purpose with or without fee is hereby granted, provided that the above\n\
 * copyright notice and this permission notice appear in all copies.\n\
 *\n\
 * THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES\n\
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF\n\
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR\n\
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\n\
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\n\
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF\n\
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n\
 */\n\
\n\
#ifndef %s\n\
#define %s\n\
\n\
#define UNICODE_POINT    0x80000000\n\
#define SPECIAL          0x40000000\n\
#define SPARSE_NULL      0x00000000\n\
#define SPARSE_DATA_FLAG 0x8000\n\
\n\
#define NONE_I           0x0\n\
#define SHIFT_I          0x1\n\
#define CTRL_I           0x2\n\
\n\
#define ALT_I            0x1\n\
#define ALTGR_I          0x2\n\
\n\
#define TABLE_IDX(alt, modifiers, escape, scancode) ( ((alt&3)<<10) | ((modifiers&3)<<8) | ((escape&1)<<7) | (scancode&0x7F) )\n\
#define TABLE_MAX TABLE_IDX(2,3,1,128)\n\
\n\
typedef struct table_entry\n\
{\n\
    uint32_t flags;\n\
    uint32_t val;\n\
} table_entry_t;\n\
\n\
typedef struct sparse_entry\n\
{\n\
    uint16_t left;\n\
    uint16_t right;\n\
} sparse_t;\n\
\n\
char sparse_buff[%d] =\n\"", header_guard, header_guard, sparse_buffsz+1);

    for (i = 0; i < sparse_buffsz; i++)
    {
        fprintf(stream, "\\x%02x", (unsigned char)sparse_buff[i]);
        if ((i % 20) == 0 && i != 0) fprintf(stream, "\\\n");
    } 
    fprintf(stream, "\";\n\nchar data_buff[%d] =\n\"", data_buffsz+1);
    for (i = 0; i < data_buffsz; i++)
    {
        fprintf(stream, "\\x%02x", (unsigned char)data_buff[i]);
        if ((i % 20) == 0 && i != 0) fprintf(stream, "\\\n");
    }
    fprintf(stream, "\";\n\n#endif\n");

    printf("Compiled keymap header file written to `%s'.\n", fname);
}

void install(char *filename)
{
    printf("Install not implemented yet.\n");
}

int main(char argc, char **argv)
{
    if (argc != 3)
    {
        usage();
        exit(0);
    }

    if (!strcmp(argv[1], "compile"))
        compile(argv[2]);
    else if (!strcmp(argv[1], "install"))
        install(argv[2]);
    else
        usage();

    exit(0);
}
