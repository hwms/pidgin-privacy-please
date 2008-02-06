/*
 * gaim-blocky
 * Copyright (C) 2005/2006 Stefan Ott
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

// config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// system headers
#include <glib.h>
#include <error.h>
#include <string.h>

// gaim headers for most plugins
#include "plugin.h"
#include "version.h"

// gaim headers for this plugin
#include "util.h"
#include "debug.h"

#include "auto-reply.h"

llnode *head = NULL;

void
destroy_msg_list ()
{
	gaim_debug_info ("gaim-blocky", "Freeing message list\n");
	llnode *node = head;

	while (node != NULL)
	{
		free (node);
		node = node->next;
	}
}

void
debug_msg_list ()
{
	gaim_debug_info ("gaim-blocky", ",----- Current message list -----\n");

	llnode *current = head;

	while (current != NULL)
	{
		char *sender = current->sender;
		gaim_debug_info ("gaim-blocky", "| %s\n", sender);
		current = current->next;
	}

	gaim_debug_info ("gaim-blocky", "`--------------------------------\n");
}

void
rm_from_msg_list (llnode *node)
{
	gaim_debug_info ("gaim-blocky", "Removing %s from list\n",
								node->sender);
	llnode *current = head, *prev = NULL;
	
	while (current != NULL)
	{
		if (current == node)
		{
			if (node == head)
			{
				head = node->next;
			}
			else
			{
				prev->next = node->next;
			}
			free (node);
		}
		prev = current;
		current = current->next;
	}
}

gboolean
is_in_msg_list (const char *sender)
{
	llnode *node = head;

	while (node != NULL)
	{
		if (strcmp (sender, node->sender) == 0) return TRUE;
		node = node->next;
	}
	return FALSE;
}

void
timer_expired (void *data)
{
	llnode *node = (llnode *) data;

	gaim_debug_info ("gaim-blocky", "Timer for %s expired\n", node->sender);

	g_source_remove (node->timer);

	rm_from_msg_list (node);
	debug_msg_list ();
}

void 
add_to_msg_list (const char *sender)
{
	llnode *node;

	if ((node = malloc (sizeof (llnode))) == NULL)
	{
		perror ("add_to_msg_list");
		(void) exit (EXIT_FAILURE);
	}

	if ((node->sender = malloc (MAX_NAME_LENGTH + 1)) == NULL)
	{
		perror ("add_to_msg_list");
		(void) exit (EXIT_FAILURE);
	}
	strncpy (node->sender, sender, MAX_NAME_LENGTH);
	node->next = head;
	head = node;

	node->timer = g_timeout_add(MSG_LIST_TIMEOUT,
					(GSourceFunc) timer_expired, node);

	debug_msg_list ();
}

void
auto_reply (GaimAccount* account, const char *recipient, const char *message)
{
	// Don't send another message within MSG_LIST_TIMEOUT
	if (is_in_msg_list (recipient)) return;

	GaimConnection *gc = NULL;
	GaimPluginProtocolInfo *prpl_info = NULL;

	gc = gaim_account_get_connection (account);

	if (gc != NULL && gc->prpl != NULL)
	{
		prpl_info = GAIM_PLUGIN_PROTOCOL_INFO (gc->prpl);
	}

	if (prpl_info && prpl_info->send_im)
	{
		prpl_info->send_im(gc, recipient, message,
#if GAIM_MAJOR_VERSION < 2
					GAIM_CONV_IM_AUTO_RESP
#else
					GAIM_MESSAGE_AUTO_RESP
#endif
		);
		add_to_msg_list (recipient);
	}

	return;
}
