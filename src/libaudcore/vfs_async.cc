/*
 * vfs_async.c
 * Copyright 2010-2012 William Pitcock and John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
#include <pthread.h>

#include "mainloop.h"
#include "vfs_async.h"

struct VFSAsyncTrampoline {
    String filename;
    void *buf;
    int64_t size;
    pthread_t thread;
    void * userdata;

    VFSConsumer cons_f;
};

static QueuedFunc queued_trampoline;

void
vfs_async_file_get_contents_trampoline(void * data)
{
    VFSAsyncTrampoline *tr = (VFSAsyncTrampoline *) data;

    pthread_join (tr->thread, NULL);

    tr->cons_f(tr->buf, tr->size, tr->userdata);

    delete tr;
}

void *
vfs_async_file_get_contents_worker(void * data)
{
    VFSAsyncTrampoline *tr = (VFSAsyncTrampoline *) data;

    vfs_file_get_contents(tr->filename, &tr->buf, &tr->size);

    queued_trampoline.queue (vfs_async_file_get_contents_trampoline, tr);

    return NULL;
}

EXPORT void
vfs_async_file_get_contents(const char *filename, VFSConsumer cons_f, void * userdata)
{
    VFSAsyncTrampoline * tr = new VFSAsyncTrampoline ();

    tr->filename = String (filename);
    tr->cons_f = cons_f;
    tr->userdata = userdata;

    pthread_create (& tr->thread, NULL, vfs_async_file_get_contents_worker, tr);
}
