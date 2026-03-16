#include "stage.h"
#include "object.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "tree.h"
#include "wrappers.h"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int normalize_mode(uint32_t st_mode)
{
    if (S_ISLNK(st_mode))
        return 0120000;

    if (S_ISREG(st_mode)) {
        if (st_mode & 0111) {
            return 0100755; // exe
        } else {
            return 0100644; // normal file
        }
    }

    return 0;
}

static int write_stage_entry(struct stage_entry* ent, FILE* f)
{
    uint32_t ctime_sec = htonl(ent->stat_data.st_ctim.tv_sec);
    uint32_t ctime_nsec = htonl(ent->stat_data.st_ctim.tv_nsec);
    uint32_t mtime_sec = htonl(ent->stat_data.st_mtim.tv_sec);
    uint32_t mtime_nsec = htonl(ent->stat_data.st_mtim.tv_nsec);
    uint32_t dev = htonl(ent->stat_data.st_dev);
    uint32_t ino = htonl(ent->stat_data.st_ino);
    uint32_t mode = htonl(ent->mode);
    uint32_t uid = htonl(ent->stat_data.st_uid);
    uint32_t gid = htonl(ent->stat_data.st_gid);
    uint32_t size = htonl(ent->stat_data.st_size);
    uint32_t flags = htonl(ent->flags);

    fprintf(f, "%u %u %u %u %u %u %u %u %u %u ",
        ctime_sec, ctime_nsec, mtime_sec, mtime_nsec, dev, ino,
        mode, uid, gid, size);

    fwrite(ent->oid.hash, sizeof(uint8_t), 32, f);
    fputc(' ', f);

    fprintf(f, "%u", flags);
    fputc(' ', f);
    fprintf(f, "%s", ent->path);
    fputc('\0', f);
    return 0;
}

static FILE* open_stage(struct repository* repo, char* mode)
{
    strbuf path = STRBUF_INIT;
    strbuf_addf(&path, "%s/%s", repo->repo_dir, STAGE_DIR);
    FILE* stagefile = fopen(path.buf, mode);
    strbuf_free(&path);
    return stagefile;
}

int write_stage(struct repository* repo)
{
    FILE* stage_file = open_stage(repo, "wb+");
    for (int i = 0; i < repo->stage->entry_count; i++) {
        write_stage_entry(repo->stage->entries[i], stage_file);
    }
    fclose(stage_file);
    return 0;
}

static int read_entry(struct stage_entry* ent, FILE* f)
{
    uint32_t ctime_sec;
    uint32_t ctime_nsec;
    uint32_t mtime_sec;
    uint32_t mtime_nsec;
    uint32_t dev;
    uint32_t ino;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t flags;

    if (fscanf(f, "%u %u %u %u %u %u %u %u %u %u ",
            &ctime_sec, &ctime_nsec, &mtime_sec, &mtime_nsec, &dev, &ino,
            &mode, &uid, &gid, &size)
        != 10) {
        return -1;
    }

    ent->stat_data.st_ctim.tv_sec = ntohl(ctime_sec);
    ent->stat_data.st_ctim.tv_nsec = ntohl(ctime_nsec);
    ent->stat_data.st_mtim.tv_sec = ntohl(mtime_sec);
    ent->stat_data.st_mtim.tv_nsec = ntohl(mtime_nsec);
    ent->stat_data.st_dev = ntohl(dev);
    ent->stat_data.st_ino = ntohl(ino);
    ent->mode = ntohl(mode);
    ent->stat_data.st_uid = ntohl(uid);
    ent->stat_data.st_gid = ntohl(gid);
    ent->stat_data.st_size = ntohl(size);

    ent->path_len = 0;
    ent->path = xmalloc(1, char);
    ent->path[0] = '\0';

    // fgetc(f); // get the space
    fread(ent->oid.hash, sizeof(uint8_t), 32, f);
    // fgetc(f); // get the space again.
    fscanf(f, "%u", &flags);
    ent->flags = ntohl(flags);
    char c = fgetc(f);
    while ((c = fgetc(f)) != '\0') {
        ent->path = realloc(ent->path, ++ent->path_len);
        ent->path[ent->path_len - 1] = c;
    }
    ent->path[ent->path_len] = '\0';

    return 0;
}

int load_stage(struct repository* repo)
{
    struct stage* out = repo->stage;
    out->entry_count = 0;
    out->entries = NULL;
    FILE* stage_file = open_stage(repo, "rb");
    if (!stage_file)
        return 1;

    stage_entry* ent = xmalloc(1, stage_entry);
    while (read_entry(ent, stage_file) == 0) {
        out->entries = realloc(out->entries, ++out->entry_count);
        out->entries[out->entry_count - 1] = ent;
        ent = xmalloc(1, stage_entry);
    }

    free(ent); // free the last allocation.
    return 0;
}

void add_to_stage(stage* stage, const char* path, object_id oid, struct stat st)
{
    int index = index_on_stage(stage, path);

    // TOOD: figure out these merge statuses.

    // Insert new file sorted by path.

    stage_entry* new_entry;
    if (index == -1) {
        new_entry = xmalloc(1, stage_entry);
        stage->entries = xrealloc(stage->entries, ++stage->entry_count, stage_entry*);
        int new_index = 0;
        for (int i = 0; i < stage->entry_count - 1; i++) {
            stage_entry* ent = stage->entries[i];
            if (strcmp(path, ent->path) < 0)
                break;
            new_index++;
        }

        for (int i = stage->entry_count - 1; i > new_index; i--) {
            stage->entries[i] = stage->entries[i - 1];
        }
        stage->entries[new_index] = new_entry;
    } else {
        new_entry = stage->entries[index];
        free(new_entry->path);
    }

    new_entry->path = strdup(path);
    new_entry->path_len = strlen(path);
    new_entry->stat_data = st;
    new_entry->flags = 0;
    new_entry->mode = normalize_mode(st.st_mode);
    oidcpy(&new_entry->oid, &oid);
}

int index_on_stage(stage* stage, const char* path)
{
    for (int i = 0; i < stage->entry_count; i++) {
        if (strcmp(path, stage->entries[i]->path) == 0) {
            return i;
        }
    }

    return -1;
}

int is_on_stage(stage* stage, const char* path)
{
    return index_on_stage(stage, path) != -1;
}

void remove_from_stage(stage* stage, const char* path)
{
    int index = index_on_stage(stage, path);
    if (index == -1)
        return;

    stage_entry* to_remove = stage->entries[index];
    for (int i = index; i < stage->entry_count - 1; i++) {
        stage->entries[i] = stage->entries[i + 1];
    }
    stage->entry_count--;

    free(to_remove->path);
    free(to_remove);
}

int stage_can_be_written(stage* stage)
{
    for (int i = 0; i < stage->entry_count; i++) {
        if (stage->entries[i]->flags & 0b11)
            return 0;
    }

    return 1;
}

void construct_stage_tree(struct tree_node* out_root, stage* stage)
{
    init_root(out_root);

    for (int i = 0; i < stage->entry_count; i++) {
        stage_entry* ent = stage->entries[i];
        add_leaf(out_root, ent->path, ent->mode, &ent->oid);
    }
}

void get_modified_entries(stage* out, repository* repo)
{
    stage* s = repo->stage;
    out->entry_count = 0;
    out->entries = NULL;

    for (int i = 0; i < s->entry_count; i++) {
        strbuf realpath = STRBUF_INIT;
        stage_entry* ent = s->entries[i];
        strbuf_addf(&realpath, "%s/%s", repo->repo_root, ent->path);

        struct stat st;
        if (lstat(realpath.buf, &st) != 0) {
            strbuf_free(&realpath);
            continue;
        }

        int time_changed = (st.st_mtim.tv_sec != ent->stat_data.st_mtim.tv_sec) || (st.st_mtim.tv_nsec != ent->stat_data.st_mtim.tv_nsec);
        int size_changed = (st.st_size != ent->stat_data.st_size);

        if (time_changed || size_changed) {
            out->entries = xrealloc(out->entries, ++out->entry_count, stage_entry*);
            out->entries[out->entry_count - 1] = ent;
        }
        strbuf_free(&realpath);
    }
}

void get_deleted_entries(stage* out, repository* repo)
{
    stage* s = repo->stage;
    out->entry_count = 0;
    out->entries = NULL;

    for (int i = 0; i < s->entry_count; i++) {
        strbuf realpath = STRBUF_INIT;
        stage_entry* ent = s->entries[i];
        strbuf_addf(&realpath, "%s/%s", repo->repo_root, ent->path);

        struct stat st;
        int del = 0;
        if (lstat(realpath.buf, &st) != 0) {
            if (errno == ENOENT)
                del = 1;
        }

        if (del) {
            out->entries = xrealloc(out->entries, ++out->entry_count, stage_entry*);
            out->entries[out->entry_count - 1] = ent;
        }

        strbuf_free(&realpath);
    }
}

static void recursive_reconstruction_helper(stage* out, char* path, tree_node* node)
{
    for (int i = 0; i < node->child_count; i++) {
        tree_node* child = node->children[i];
        int is_blob = child->child_count == 0;
        if (is_blob) {
            strbuf newpathbuf = STRBUF_INIT;
            strbuf_addf(&newpathbuf, "%s/%s", path, child->name);
            struct stat st = { 0 };
            st.st_mode = child->mode;
            add_to_stage(out, newpathbuf.buf, child->oid, st);
            strbuf_free(&newpathbuf);
        } else {
            strbuf newpathbuf = STRBUF_INIT;
            strbuf_addf(&newpathbuf, "%s/%s/", path, child->name);
            recursive_reconstruction_helper(out, newpathbuf.buf, child);
            strbuf_free(&newpathbuf);
        }
    }
}

void reconstruct_stage_from_tree(stage* out_stage, tree_node* root)
{
    memset(out_stage, 0, sizeof(stage));
    for (int i = 0; i < root->child_count; i++) {
        tree_node* child = root->children[i];
        int is_blob = child->child_count == 0;
        if (is_blob) {
            struct stat st = { 0 };
            st.st_mode = child->mode;
            add_to_stage(out_stage, child->name, child->oid, st);
        } else {
            strbuf newpathbuf = STRBUF_INIT;
            strbuf_addf(&newpathbuf, "%s", child->name);
            recursive_reconstruction_helper(out_stage, newpathbuf.buf, child);
            strbuf_free(&newpathbuf);
        }
    }
}
