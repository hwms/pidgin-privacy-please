/*
 * pidgin privacy please
 * Copyright (C) 2005-2010 Stefan Ott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// system headers
#include <glib.h>
#include <string.h>
#include <stdlib.h>

// pidgin headers
#include <purple.h>
#include <plugin.h>

// pidgin-pp headers
#include "autoreply.h"
#include "botcheck.h"
#include "pp-prefs.h"

llnode *botcheck_passed_senders = NULL;

void
botcheck_cleanup()
{
	llnode *node;

	purple_debug_info("pidgin-pp", "Freeing botcheck list\n");

	node = botcheck_passed_senders;

	while (node != NULL)
	{
		free(node);
		node = node->next;
	}
}

gboolean
botcheck_passed(const char *sender)
{
	llnode *node = botcheck_passed_senders;

	while (node != NULL)
	{
		if (strcmp(sender, node->sender) == 0)
			return TRUE;
		node = node->next;
	}
	return FALSE;
}

gboolean
botcheck_verify(const char *sender, const char *message)
{
	const char *correct = prefs_botcheck_answer();

	// we only want the correct string to be part of the message,
	// the body may contain other stuff, too
	if (strstr(message, correct))
	{
		purple_debug_info("pidgin-pp", "Botcheck: Right answer\n");
		return TRUE;
	}
	else
	{
		purple_debug_info("pidgin-pp",
			"Botcheck: Wrong answer or initial message\n");
		return FALSE;
	}
}

static void
botcheck_send(PurpleAccount* account, const char *recipient, const char *msg)
{
	PurpleConnection *gc = purple_account_get_connection(account);
	PurplePluginProtocolInfo *prpl_info = NULL;

	if (gc != NULL && gc->prpl != NULL)
	{
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl);
	}

	if (prpl_info && prpl_info->send_im)
	{
		prpl_info->send_im(gc, recipient, msg,
						PURPLE_MESSAGE_AUTO_RESP);
	}
	else
	{
		purple_debug_error("pidgin-pp", "Failed to send message\n");
	}
}

static void
botcheck_add_to_list(const char *sender)
{
	llnode *node;

	if ((node = malloc(sizeof(llnode))) == NULL)
	{
		purple_debug_fatal("pidgin-pp", "Malloc failed\n");
		return;
	}

	if ((node->sender = malloc(MAX_NAME_LENGTH + 1)) == NULL)
	{
		free(node);
		purple_debug_fatal("pidgin-pp", "Malloc failed\n");
		return;
	}
	strncpy(node->sender, sender, MAX_NAME_LENGTH);
	node->next = botcheck_passed_senders;
	botcheck_passed_senders = node;
}

void
botcheck_ask(PurpleAccount* account, const char *sender)
{
	purple_debug_info("pidgin-pp", "Botcheck: asking question\n");
	const char *message = prefs_botcheck_question();
	botcheck_send(account, sender, message);
}

void
botcheck_ok(PurpleAccount* account, const char *sender)
{
	botcheck_add_to_list(sender);
	purple_debug_info("pidgin-pp", "Botcheck: confirming answer\n");
	const char *message = prefs_botcheck_ok();
	botcheck_send(account, sender, message);
}
