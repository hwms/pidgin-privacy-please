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

// config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// system headers
#include <glib.h>
#include <string.h>

// pidgin headers for most plugins
#include <purple.h>
#include <pidgin/gtkutils.h>
#include <pidgin/gtkplugin.h>

// pidgin-pp headers
#include "pp-prefs.h"
#include "autoreply.h"
#include "botcheck.h"
#include "blocklist.h"

#ifdef WIN32
#include "win32dep.h"
#endif

#ifdef ENABLE_NLS
#include <glib/gi18n-lib.h>
#else
#define _(String) ((const char *) (String))
#define N_(String) ((const char *) (String))
#endif // ENABLE NLS

static void
msg_blocked_cb(PurpleAccount* account, char *sender)
{
	purple_debug_info("pidgin-pp", "Message was blocked, reply?\n");

	if (prefs_autoreply_blocked())
	{
		const char* msg = prefs_autoreply_blocked_msg();
		auto_reply(account, sender, msg);
	}
}

#if GLIB_CHECK_VERSION(2,14,0)
static gboolean
pp_match_sender_regex(char *sender)
{
	purple_debug_info("pidgin-pp", "Block '%s' using regex?\n", sender);
	const gchar *pattern = prefs_block_account_regex();

	return g_regex_match_simple(pattern, sender, 0, 0);
}

static gboolean
pp_match_msg_regex(char *message)
{
	purple_debug_info("pidgin-pp", "Block '%s' using regex?\n", message);
	const gchar *pattern = prefs_block_message_regex();

	return g_regex_match_simple(pattern, message, 0, 0);
}
#endif // GLIB_CHECK_VERSION

//
// This is our callback for the receiving-im-msg signal.
//
// We return TRUE to block the IM, FALSE to accept the IM
//
static gboolean
receiving_im_msg_cb(PurpleAccount* account, char **sender, char **message,
			PurpleConversation *conv, PurpleMessageFlags *flags)
{
	PurpleBuddy* buddy;

	purple_debug_info("pidgin-pp", "Got message from %s, protocol=%s\n",
			*sender, account->protocol_id);

	if (conv)
	{
		purple_debug_info("pidgin-pp",
			"Message from an existing converstation, accepting\n");
		return FALSE; // accept
	}

	// accept all IRC messages if configured accordingly
	if ((!strcmp(account->protocol_id, "prpl-irc")) &&
			prefs_allow_all_irc())
	{
		purple_debug_info("pidgin-pp", "Accepting IRC message\n");
		return FALSE; // accept
	}

	// block AOL system messages
	if (prefs_block_aol_sysmsg() && !strcmp(*sender, "AOL System Msg"))
	{
		purple_debug_info("pidgin-pp", "Blocking AOL system message\n");
		return TRUE; // block
	}

#if GLIB_CHECK_VERSION(2,14,0)
	// block using account regex
	if (prefs_block_account_using_regex() && pp_match_sender_regex(*sender))
	{
		purple_debug_info("pidgin-pp", "Blocking account with regex\n");
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}

	// block using message regex
	if (prefs_block_message_using_regex() && pp_match_msg_regex(*message))
	{
		purple_debug_info("pidgin-pp", "Blocking message with regex\n");
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}
#endif // GLIB_CHECK_VERSION

	// block blocked buddies
	if (blocklist_contains(*sender))
	{
		purple_debug_info("pidgin-pp", "%s on blocklist, blocking\n",
				*sender);
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}

	// bot check
	if (prefs_botcheck_enabled())
	{
		if (botcheck_passed(*sender))
		{
			purple_debug_info("pidgin-pp",
					"Botcheck: User already verified\n");
			return FALSE; // accept
		}
		else if (botcheck_verify(*sender, *message))
		{
			botcheck_ok(account, *sender);

			// we can safely block the message because the sender
			// won't notice and we don't need to see the actual
			// message
			return TRUE; // block
		}
		else
		{
			botcheck_ask(account, *sender);
			return TRUE; // block
		}
	}

	buddy = purple_find_buddy (account, *sender);

	if (buddy == NULL) // No contact list entry
	{
		purple_debug_info("pidgin-pp", "Got message from unknown "
						"source: %s\n", *sender);

		if (prefs_block_unknown())
		{
			purple_debug_info("pidgin-pp", "Blocked\n");

			if (prefs_autoreply_unknown())
			{
				const char* msg = prefs_autoreply_unknown_msg();
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

static int
#if PURPLE_VERSION_CHECK(2, 8, 0)
request_authorization_cb(PurpleAccount* account, char *sender, char *msg)
#else
request_authorization_cb(PurpleAccount* account, char *sender)
#endif // PURPLE_VERSION_CHECK
{
	// < 0: deny
	// = 0: prompt user
	// > 0: accept

	int deny = -1;

	if (g_str_equal(account->protocol_id, "prpl-aim") ||
		g_str_equal(account->protocol_id, "prpl-icq"))
	{
		deny = -2; // silently drop ICQ requests
	}

	purple_debug_info("pidgin-pp",
			"Processing authorization request from %s\n", sender);

	if (prefs_auth_block_all())
	{
		purple_debug_info("pidgin-pp",
			"Blocking authorization request (blocking all)\n");
		return deny;
	}

	if (prefs_auth_block_oscar() &&
		(
			g_str_equal(account->protocol_id, "prpl-aim") ||
			g_str_equal(account->protocol_id, "prpl-icq")
		))
	{
		purple_debug_info("pidgin-pp", "Blocking OSCAR auth request\n");
		return deny;
	}

#if PURPLE_VERSION_CHECK(2, 8, 0)
	if (prefs_auth_block_with_url() && (msg != NULL))
	{
		const gchar *pattern = "http:\\/\\/";
		gboolean match = g_regex_match_simple(pattern, msg, 0, 0);

		if (match)
		{
			purple_debug_info("pidgin-pp",
					"Blocking auth request with url\n");
			return deny;
		}
	}
#endif // PURPLE_VERSION_CHECK

	if (prefs_auth_block_repeated() && blocklist_contains(sender))
	{
		purple_debug_info("pidgin-pp", "Blocking repeated request\n");
		return deny;
	}

	if (prefs_auth_auto_info())
	{
		// Show the info dialog
		PurpleConnection* con = purple_account_get_connection(account);
		pidgin_retrieve_user_info(con, sender);
	}

	return 0; // don't interfere, just prompt user
}

static void
authorization_deny_cb(PurpleAccount* account, char *sender)
{
	if (!prefs_auth_block_repeated())
		return;

	purple_debug_info("pidgin-pp",
		"Processing rejected authorization request from %s\n", sender);

	if (!blocklist_contains(sender))
		blocklist_add(sender);
}

static void
jabber_xmlnode_cb(PurpleConnection *gc, xmlnode **packet, gpointer null)
{
	xmlnode *node;
	char *node_name;

	// Immediately abort if we don't block headlines
	if (!prefs_block_jabber_headlines())
		return;

	node = *packet;

	if ((node == NULL) || (node->name == NULL))
		return;

	node_name = g_markup_escape_text(node->name, -1);

	if (!strcmp(node_name, "message"))
	{
		const char *type;
		type = xmlnode_get_attrib(node, "type");

		if (!type) {
			purple_debug_info("pidgin-pp",
				"JABBER XML: name=%s, no type\n", node_name);
			return;
		}

		purple_debug_info("pidgin-pp",
			"JABBER XML: name=%s, type=%s\n", node_name, type);

		if (!strcmp(type, "headline")) {
			purple_debug_info("pidgin-pp",
					"Discarding jabber headline message\n");
			xmlnode_free(*packet);
			*packet = NULL;
		}
	}
	g_free (node_name);
}

static GList *
actions(PurplePlugin *plugin, gpointer context)
{
	GList *actions = NULL;
	PurplePluginAction *action = NULL;

	action = purple_plugin_action_new(
				_("Manage blocked users"), blocklist_manage);
	actions = g_list_append(actions, action);

	return actions;
}

static gboolean
plugin_load(PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle ();
	void *acct_handle = purple_accounts_get_handle ();

	PurplePlugin *jabber = purple_find_prpl("prpl-jabber");

	prefs_load();

	purple_signal_connect(conv_handle, "receiving-im-msg",
			plugin, PURPLE_CALLBACK(receiving_im_msg_cb), NULL);
#if PURPLE_VERSION_CHECK(2, 8, 0)
	purple_signal_connect(acct_handle,
			"account-authorization-requested-with-message",
			plugin, PURPLE_CALLBACK(request_authorization_cb),
			NULL);
#else
	purple_signal_connect(acct_handle, "account-authorization-requested",
			plugin, PURPLE_CALLBACK(request_authorization_cb),
			NULL);
#endif // PURPLE_VERSION_CHECK
	purple_signal_connect(acct_handle, "account-authorization-denied",
			plugin, PURPLE_CALLBACK(authorization_deny_cb), NULL);
	purple_signal_connect(conv_handle, "blocked-im-msg",
			plugin, PURPLE_CALLBACK(msg_blocked_cb), NULL);

	if (jabber)
	{
		purple_signal_connect(jabber, "jabber-receiving-xmlnode",
			plugin, PURPLE_CALLBACK(jabber_xmlnode_cb), NULL);
	}
	else
	{
		purple_debug_info("pidgin-pp",
			"Jabber support missing - disabled headline blocking");
	}

	purple_signal_connect(purple_blist_get_handle(),
			"blist-node-extended-menu", plugin,
			PURPLE_CALLBACK(blocklist_mouse_action), NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin * plugin)
{
	purple_signals_disconnect_by_handle(plugin);
	autoreply_cleanup();
	botcheck_cleanup();

	return TRUE;
}

static PidginPluginUiInfo ui_info =
{
	get_plugin_config_frame,
	0, // page_num (reserved)

	// padding
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,				/**< type           */
	PIDGIN_PLUGIN_TYPE,				/**< ui_requirement */
	0,						/**< flags          */
	NULL,						/**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,			/**< priority       */

	N_("core-pidgin_pp_"),				/**< id             */
	NULL,						/**< name           */
	PACKAGE_VERSION,				/**< version        */
	NULL,						/**< summary	    */
							/**  description    */
	NULL,
	"Stefan Ott <stefan@ott.net>",			/**< author         */
	"http://pidgin-privacy-please.googlecode.com/",	/**< homepage       */

	plugin_load,					/**< load           */
	plugin_unload,					/**< unload         */
	NULL,						/**< destroy        */

	&ui_info,					/**< ui_info        */
	NULL,						/**< extra_info     */
	NULL,                                           /**< prefs_info     */
	actions,

	// padding
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin * plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */
	info.name        = _("Privacy Please");
	info.summary     = _("Stops IM-spam");
	info.description = _("A simple plugin to stop unwanted messages and repeated authorization requests from spammers.");

	prefs_init();
}

PURPLE_INIT_PLUGIN(pidgin_pp, init_plugin, info)
