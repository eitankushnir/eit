#include "commit.h"
#include "object.h"
#include "sha256.h"
#include "strbuf.h"
#include "tree.h"
#include "wrappers.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_commit(commit* c)
{
    memset(c, 0, sizeof(commit));
}

void add_author(commit* c, char* name, char* email, unsigned int time)
{
    if (strchr(email, ' ')) {
        die("Email cannot contains spaces\n");
    }
    c->author_name = substr(name, -1);
    c->author_email = substr(email, -1);
    c->author_time = time;
}

void add_commiter(commit* c, char* name, char* email, unsigned int time)
{
    if (strchr(email, ' ')) {
        die("Email cannot contains spaces\n");
    }

    c->committer_name = substr(name, -1);
    c->committer_email = substr(email, -1);
    c->commit_time = time;
}

void add_parent(commit* c, object_id* oid)
{
    c->parent_count++;
    c->parent_oids = xrealloc(c->parent_oids, c->parent_count, object_id);
    c->parent_oids[c->parent_count - 1] = *oid;
}

void set_tree(commit* c, object_id* oid)
{
    c->tree_oid = *oid;
}

void set_message(commit* c, const char* msg)
{
    c->commit_message = substr(msg, -1);
}

void free_commit(commit* c)
{
    free(c->committer_name);
    free(c->committer_email);
    free(c->author_name);
    free(c->author_email);
    free(c->parent_oids);
    free(c->commit_message);
}

void write_commit(commit* c, repository* repo)
{
    FILE* buffer = tmpfile();

    fprintf(buffer, "%u", c->parent_count);
    fputc(' ', buffer);
    for (unsigned int i = 0; i < c->parent_count; i++) {
        fwrite(c->parent_oids[i].hash, sizeof(uint8_t), 32, buffer);
    }
    fwrite(c->tree_oid.hash, sizeof(uint8_t), 32, buffer);

    fprintf(buffer, "%u %s %s", c->author_time, c->author_email, c->author_name);
    fputc('\0', buffer);
    fprintf(buffer, "%u %s %s", c->commit_time, c->committer_email, c->committer_name);
    fputc('\0', buffer);
    fprintf(buffer, "%s", c->commit_message);

    write_object(OBJ_COMMIT, buffer, repo, &c->oid);
}

void parse_commit(const oid_hex* hex, commit* out, repository* repo)
{
    init_commit(out);
    FILE* objfile = open_object(hex, repo);

    while (fgetc(objfile) != '\0') { } // get past the header.

    unsigned int pcount;
    fscanf(objfile, "%u", &pcount);
    fgetc(objfile); // get rid of the space

    for (unsigned int i = 0; i < pcount; i++) {
        object_id oid;
        fread(oid.hash, sizeof(uint8_t), 32, objfile);
        add_parent(out, &oid);
    }

    object_id oid;
    fread(oid.hash, sizeof(uint8_t), 32, objfile);
    set_tree(out, &oid);

    strbuf name = STRBUF_INIT, email = STRBUF_INIT;
    unsigned int time;
    fscanf(objfile, "%u", &time);
    char c = fgetc(objfile);
    while ((c = fgetc(objfile)) != ' ') {
        strbuf_addchr(&email, c);
    }
    while ((c = fgetc(objfile)) != '\0') {
        strbuf_addchr(&name, c);
    }
    add_author(out, name.buf, email.buf, time);
    strbuf_free(&name);
    strbuf_free(&email);
    strbuf_init(&name);
    strbuf_init(&email);

    fscanf(objfile, "%u", &time);
    c = fgetc(objfile);
    while ((c = fgetc(objfile)) != ' ') {
        strbuf_addchr(&email, c);
    }
    while ((c = fgetc(objfile)) != '\0') {
        strbuf_addchr(&name, c);
    }

    add_commiter(out, name.buf, email.buf, time);
    strbuf_free(&name);
    strbuf_free(&email);

    strbuf commit_msg = STRBUF_INIT;
    while ((c = fgetc(objfile)) != EOF) {
        strbuf_addchr(&commit_msg, c);
    }

    set_message(out, commit_msg.buf);
    strbuf_free(&commit_msg);
}

void print_commit(commit* c)
{
    oid_hex tree_hex = oid_tostring(&c->tree_oid);
    printf("tree %s\n", tree_hex.hex);

    if (c->parent_count == 0) {
        printf("no parents\n");
    } else if (c->parent_count == 1) {
        oid_hex hex = oid_tostring(&c->parent_oids[0]);
        printf("parent %s\n", hex.hex);
    } else {
        printf("parents ");
        for (unsigned int i = 0; i < c->parent_count; i++) {
            oid_hex hex = oid_tostring(&c->parent_oids[i]);
            printf("%s ", hex.hex);
        }
        printf("\n");
    }

    printf("author %s <%s> %u\n", c->author_name, c->author_email, c->author_time);
    printf("committer %s <%s> %u\n", c->committer_name, c->committer_email, c->commit_time);
    printf("\n%s\n", c->commit_message);
}
