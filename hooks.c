/* $OpenBSD$ */

/*
 * Copyright (c) 2012 Thomas Adam <thomas@xteddy.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

struct hooks {
	RB_HEAD(hooks_tree, hook) tree;
	struct hooks	*parent;
};

static int	hooks_cmp(struct hook *, struct hook *);
RB_PROTOTYPE(hooks_tree, hook, entry, hooks_cmp);
RB_GENERATE(hooks_tree, hook, entry, hooks_cmp);

struct hook	*hooks_find1(struct hooks *, const char *);

static int
hooks_cmp(struct hook *hook1, struct hook *hook2)
{
	return (strcmp(hook1->name, hook2->name));
}

struct hooks *
hooks_create(struct hooks *parent)
{
	struct hooks	*hooks;

	hooks = xcalloc(1, sizeof *hooks);
	RB_INIT(&hooks->tree);
	hooks->parent = parent;
	return (hooks);
}

void
hooks_free(struct hooks *hooks)
{
	struct hook	*hook, *hook1;

	RB_FOREACH_SAFE(hook, hooks_tree, &hooks->tree, hook1)
		hooks_remove(hooks, hook);
	free(hooks);
}

struct hook *
hooks_first(struct hooks *hooks)
{
	return (RB_MIN(hooks_tree, &hooks->tree));
}

struct hook *
hooks_next(struct hook *hook)
{
	return (RB_NEXT(hooks_tree, &hooks->tree, hook));
}

void
hooks_add(struct hooks *hooks, const char *name, struct cmd_list *cmdlist)
{
	struct hook	*hook;

	if ((hook = hooks_find1(hooks, name)) != NULL)
		hooks_remove(hooks, hook);

	hook = xcalloc(1, sizeof *hook);
	hook->name = xstrdup(name);
	hook->cmdlist = cmdlist;
	hook->cmdlist->references++;
	RB_INSERT(hooks_tree, &hooks->tree, hook);
}

void
hooks_remove(struct hooks *hooks, struct hook *hook)
{
	RB_REMOVE(hooks_tree, &hooks->tree, hook);
	cmd_list_free(hook->cmdlist);
	free((char *) hook->name);
	free(hook);
}

struct hook *
hooks_find1(struct hooks *hooks, const char *name)
{
	struct hook	hook;

	hook.name = name;
	return (RB_FIND(hooks_tree, &hooks->tree, &hook));
}

struct hook *
hooks_find(struct hooks *hooks, const char *name)
{
	struct hook	 hook0, *hook;

	hook0.name = name;
	hook = RB_FIND(hooks_tree, &hooks->tree, &hook0);
	while (hook == NULL) {
		hooks = hooks->parent;
		if (hooks == NULL)
			break;
		hook = RB_FIND(hooks_tree, &hooks->tree, &hook0);
	}
	return (hook);
}

void
hooks_run(struct hooks *hooks, const char *name, struct client *c)
{
	struct hook	*hook;
	struct cmd_q	*cmdq;

	hook = hooks_find(hooks, name);
	if (hook == NULL)
		return;
	log_debug("running hook %s", name);

	cmdq = cmdq_new(c);
	cmdq_run(cmdq, hook->cmdlist, NULL);
	cmdq_free(cmdq);
}
