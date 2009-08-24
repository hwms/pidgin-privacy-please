/*
 * pidgin privacy please
 * Copyright (C) 2005-2009 Stefan Ott
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
#include <string.h>

// pidgin headers for most plugins
#include <purple.h>
#include "plugin.h"
#include "version.h"

// pidgin headers for this plugin
#include "util.h"
#include "debug.h"
#include "account.h"
#include "privacy.h"
#include "blist.h"
#include "gtkutils.h"

// pidgin header needed for gettext
#define GETTEXT_PACKAGE "gtk20"
#include <glib/gi18n-lib.h>

// our auto-reply functionality
#include "auto-reply.h"

static const char*
conf_msg_unknown_autoreply()
{
	return purple_prefs_get_string
				("/plugins/core/pidgin_pp/unknown_message");
}

static gboolean
conf_block_unknown()
{
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/unknown_block");
}

static gboolean
conf_block_aol_sysmsg()
{
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/block_aol_sys");
}

static gboolean
conf_reply_unknown()
{
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/unknown_reply");
}

static const char*
conf_msg_blocked_autoreply()
{
	return purple_prefs_get_string("/plugins/core/pidgin_pp/message");
}

static gboolean
conf_reply_blocked()
{
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/reply");
}

static gboolean
conf_allow_all_irc()
{
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/allow_all_irc");
}

static GList*
conf_get_block_list()
{
	return purple_prefs_get_string_list("/plugins/core/pidgin_pp/block");
}

static gboolean
contact_is_blocked(const gchar *name)
{
	GList* blocklist = conf_get_block_list();

	// we don't care about what's after the slash
	gchar *clean_name = strtok(name, "/");

	while (blocklist)
	{
		if (!strcmp(blocklist->data, clean_name))
			return TRUE;

		blocklist = g_list_next(blocklist);
	}
	return FALSE;
}

static void
msg_blocked_cb(PurpleAccount* account, char *sender)
{
	purple_debug_info("pidgin-pp", "Message was blocked, reply?\n");

	if (conf_reply_blocked())
	{
		const char* msg = conf_msg_blocked_autoreply();
		auto_reply(account, sender, msg);
	}
}

/**
 * This is our callback for the receiving-im-msg signal.
 *
 * We return TRUE to block the IM, FALSE to accept the IM
 */
static gboolean
receiving_im_msg_cb(PurpleAccount* account, char **sender, char **message,
						int *flags, void *data)
{
	PurpleBuddy* buddy;

	purple_debug_info("pidgin-pp", "Got message from %s, protocol=%s\n",
			*sender, account->protocol_id);

	// accept all IRC messages if configured accordingly
	if ((!strcmp(account->protocol_id, "prpl-irc")) && conf_allow_all_irc())
		return FALSE;

	// block AOL system messages
	if (conf_block_aol_sysmsg() && !strcmp(*sender, "AOL System Msg"))
	{
		purple_debug_info("pidgin-pp", "Blocking AOL system message\n");
		return TRUE; // block
	}

	// block blocked buddies
	if (contact_is_blocked(*sender))
	{
		purple_debug_info("pidgin-pp", "Blocking %s\n", *sender);

		// TODO: pidgin should actually emit a signal when we block a
		// message (but it doesn't). remember to file a bug report.
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}

	buddy = purple_find_buddy (account, *sender);

	if (buddy == NULL) // No contact list entry
	{
		purple_debug_info("pidgin-pp", "Got message from unknown "
						"source: %s\n", *sender);

		if (conf_block_unknown())
		{
			purple_debug_info("pidgin-pp", "Blocked\n");

			if (conf_reply_unknown())
			{
				const char* msg = conf_msg_unknown_autoreply ();
				auto_reply(account, *sender, msg);
			}
			return TRUE; // block
		}
		else
		{
			purple_debug_info("pidgin-pp", "Allowed\n");
		}
	}
	else // Contact list entry exists
	{
		const char* alias = purple_buddy_get_alias_only(buddy);
		purple_debug_info("pidgin-pp", "Allowed %s\n", alias);
	}
	return FALSE; // default: accept
}

static gboolean
request_authorization_cb (PurpleAccount* account, char *sender)
{
	int retval;
	purple_debug_info("pidgin-pp", "request_authorization_cb");

	if (purple_prefs_get_bool("/plugins/core/pidgin_pp/block_auth_all"))
	{
		purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp", "Blocking "
				"authorization request from %s\n", sender);
		return -1;
	}

	if (!purple_prefs_get_bool("/plugins/core/pidgin_pp/block_denied"))
	{
		return 0; // don't interfere, just prompt user
	}

	purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp", "Processing authorization"
						" request from %s\n", sender);

	// < 0: deny
	// = 0: prompt user
	// > 0: accept
	retval = -!purple_privacy_check(account, sender);

	if (!retval && purple_prefs_get_bool
				("/plugins/core/pidgin_pp/auth_auto_info")) {
		// Show the info dialog
		PurpleConnection* con = purple_account_get_connection (account);
		pidgin_retrieve_user_info (con, sender);
	}

	return retval;
}

static void
authorization_deny_cb(PurpleAccount* account, char *sender)
{
	if (!purple_prefs_get_bool("/plugins/core/pidgin_pp/block_denied"))
		return;

	purple_debug_info("pidgin-pp", "Processing rejected "
				"authorization request from %s\n", sender);
	if (purple_privacy_check(account, sender))
		purple_privacy_deny_add(account, sender, FALSE);
}

static void
jabber_xmlnode_cb(PurpleConnection *gc, xmlnode **packet, gpointer null)
{
	xmlnode *node;
	char *node_name;

	// Immediately abort if we don't block headlines
	if (!purple_prefs_get_bool
			("/plugins/core/pidgin_pp/block_jabber_headlines"))
		return;

	node = *packet;

	if ((node == NULL) || (node->name == NULL))
		return;

	node_name = g_markup_escape_text (node->name, -1);

	if (!strcmp (node_name, "message"))
	{
		const char *type;
		type = xmlnode_get_attrib (node, "type");

		if (!type) {
			purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp",
				"JABBER XML: name=%s, no type\n", node_name);
			return;
		}

		purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp", "JABBER XML: "
				"name=%s, type=%s\n", node_name, type);

		if (!strcmp(type, "headline")) {
			purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp",
					"Discarding jabber headline message\n");
			xmlnode_free(*packet);
			*packet = NULL;
		}
	}
	g_free (node_name);
}

static void
block_contact_cb(PurpleBlistNode *node, gpointer data)
{
	purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp",
			"Adding %s to block list\n",
			PURPLE_BLIST_NODE_NAME(node));

	GList* blocklist = conf_get_block_list();
	blocklist = g_list_append(blocklist, PURPLE_BLIST_NODE_NAME(node));
	purple_prefs_set_string_list("/plugins/core/pidgin_pp/block",
			blocklist);

}

static void
unblock_contact_cb(PurpleBlistNode *node, gpointer data)
{
	const gchar *name = PURPLE_BLIST_NODE_NAME(node);
	purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp",
			"Removing %s from block list\n", name);

	GList* blocklist = conf_get_block_list();
	GList* tmp = blocklist;
	while (tmp)
	{
		if (!strcmp(tmp->data, name))
		{
			blocklist = g_list_delete_link(blocklist, tmp);
			break;
		}
		tmp = g_list_next(tmp);
	}
	purple_prefs_set_string_list("/plugins/core/pidgin_pp/block",
			blocklist);

}

static void
mouse_menu_cb(PurpleBlistNode *node, GList **menu)
{
	if (purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	PurpleMenuAction *action = NULL;

	*menu = g_list_append(*menu, action);

	if (contact_is_blocked(PURPLE_BLIST_NODE_NAME(node)))
	{
		action = purple_menu_action_new(_("Unblock (privacy please)"),
			PURPLE_CALLBACK(unblock_contact_cb), NULL, NULL);
	}
	else
	{
		action = purple_menu_action_new(_("Block (privacy please)"),
			PURPLE_CALLBACK(block_contact_cb), NULL, NULL);
	}

	*menu = g_list_append(*menu, action);
}

static PurplePluginPrefFrame *
get_plugin_pref_frame (PurplePlugin* plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_label(_("Blocked messages"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/reply", _("Auto-reply on blocked messages with:"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name
				("/plugins/core/pidgin_pp/message");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_label(_("Unknown people"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/unknown_block", _("Block messages from people not on your contact list"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/unknown_reply", _("Auto-reply on blocked messages with:"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name
				("/plugins/core/pidgin_pp/unknown_message");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_label(_("Authorization"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/block_denied", _("Suppress repeated authorization requests\n(requires privacy settings to block individual users)"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/block_auth_all", _("Block all authorization requests"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/auth_auto_info", _("Automatically show user info on authorization requests"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_label(_("Protocol specific"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/block_jabber_headlines", _("Block jabber headline messages\n(eg. MSN alerts, announcements etc.)"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/allow_all_irc", _("Allow all messages on IRC"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/block_aol_sys", _("Block AOL system messages"));
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static gboolean
plugin_load (PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle ();
	void *acct_handle = purple_accounts_get_handle ();

	PurplePlugin *jabber = purple_find_prpl("prpl-jabber");

	purple_prefs_add_bool ("/plugins/core/pidgin_pp/reply", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/unknown_block", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/unknown_reply", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/auth_auto_info", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/block_jabber_headlines", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/allow_all_irc", TRUE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/block_aol_sys", FALSE);
	purple_prefs_add_string ("/plugins/core/pidgin_pp/message",
				_("Your message could not be delivered"));
	purple_prefs_add_string ("/plugins/core/pidgin_pp/unknown_message",
		_("I currently only accept messages from people on my contact"
				" list - please request my authorization."));
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/block_denied", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/block_auth_all", FALSE);
	purple_prefs_add_string_list("/plugins/core/pidgin_pp/block", NULL);

	purple_signal_connect (conv_handle, "receiving-im-msg",
			plugin, PURPLE_CALLBACK (receiving_im_msg_cb), NULL);
	purple_signal_connect (acct_handle, "account-authorization-requested",
			plugin, PURPLE_CALLBACK (request_authorization_cb), NULL);
	purple_signal_connect (acct_handle, "account-authorization-denied",
			plugin, PURPLE_CALLBACK (authorization_deny_cb), NULL);
	purple_signal_connect (conv_handle, "blocked-im-msg",
			plugin, PURPLE_CALLBACK (msg_blocked_cb), NULL);

	if (jabber)
	{
		purple_signal_connect (jabber, "jabber-receiving-xmlnode",
			plugin, PURPLE_CALLBACK (jabber_xmlnode_cb), NULL);
	}
	else
	{
		purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp", "Jabber support "
					"missing - disabled headline blocking");
	}

	purple_signal_connect(purple_blist_get_handle(),
			"blist-node-extended-menu", plugin,
			PURPLE_CALLBACK(mouse_menu_cb), NULL);
	return TRUE;
}

static gboolean
plugin_unload (PurplePlugin * plugin)
{
	purple_signals_disconnect_by_handle (plugin);
	destroy_msg_list ();

	return TRUE;
}

static PurplePluginUiInfo prefs_info = { get_plugin_pref_frame };

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,				/**< type           */
	NULL,						/**< ui_requirement */
	0,						/**< flags          */
	NULL,						/**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,				/**< priority       */

	N_("core-pidgin_pp_"),				/**< id             */
	N_("Privacy Please"),				/**< name           */
	VERSION,					/**< version        */
	N_("Stops IM-spam"),				/**< summary	    */
							/**  description    */
	N_("A simple plugin to stop unwanted messages and repeated authorization requests from spammers."),
	"Stefan Ott <stefan@ott.net>",			/**< author         */
	"http://pidgin-privacy-please.googlecode.com/",	/**< homepage       */

	plugin_load,					/**< load           */
	plugin_unload,					/**< unload         */
	NULL,						/**< destroy        */

	NULL,						/**< ui_info        */
	NULL,						/**< extra_info     */
	&prefs_info,					/**< prefs_info     */
	NULL
};

static void
init_plugin(PurplePlugin * plugin)
{
	purple_prefs_add_none("/plugins");
	purple_prefs_add_none("/plugins/core");
	purple_prefs_add_none("/plugins/core/pidgin_pp");
}

PURPLE_INIT_PLUGIN(pidgin_pp, init_plugin, info)
