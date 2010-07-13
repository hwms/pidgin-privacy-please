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
#include <gtkplugin.h>
//#include <gtkprefs.h>

// our auto-reply functionality
#include "auto-reply.h"
#include "botcheck.h"
#include "pp-prefs.h"

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
add_to_block_list(const gchar *name)
{
	// abort when a category was clicked
	if (!name)
	{
		purple_debug_info("pidgin-pp", "Not blocking (null)\n");
		return;
	}

	purple_debug_info("pidgin-pp", "Adding %s to block list\n", name);

	GList* blocklist = prefs_get_block_list();
	blocklist = g_list_append(blocklist, (gpointer) name);
	prefs_set_block_list(blocklist);
}

static void
remove_from_block_list(const gchar *name)
{
	purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp",
			"Removing %s from block list\n", name);

	GList* blocklist = prefs_get_block_list();
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
	prefs_set_block_list(blocklist);
}

static gboolean
contact_is_blocked(const gchar *name)
{
	GList* blocklist = prefs_get_block_list();

	// we don't care about what's after the slash
	gchar *clean_name = strtok((gchar *) name, "/");

	// abort when a category was clicked
	if (!clean_name)
		return FALSE;

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

/**
 * This is our callback for the receiving-im-msg signal.
 *
 * We return TRUE to block the IM, FALSE to accept the IM
 */
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
		return FALSE;
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
		purple_debug_info(
			"pidgin-pp", "Blocking account using regex\n");

		// TODO: pidgin should actually emit a signal when we block a
		// message (but it doesn't). remember to file a bug report.
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}

	// block using message regex
	if (prefs_block_message_using_regex() && pp_match_msg_regex(*message))
	{
		purple_debug_info(
			"pidgin-pp", "Blocking message using regex\n");

		// TODO: pidgin should actually emit a signal when we block a
		// message (but it doesn't). remember to file a bug report.
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}
#endif // GLIB_CHECK_VERSION

	// block blocked buddies
	if (contact_is_blocked(*sender))
	{
		purple_debug_info("pidgin-pp", "Blocking %s\n", *sender);

		// TODO: pidgin should actually emit a signal when we block a
		// message (but it doesn't). remember to file a bug report.
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

	//purple_debug_info("pidgin-pp", "request_authorization_cb\n");
	purple_debug_info("pidgin-pp",
			"Processing authorization request from %s\n", sender);

	if (prefs_auth_block_all())
	{
		purple_debug_info("pidgin-pp",
			"Blocking authorization request from %s\n", sender);
		return -1; // deny
	}

	if (prefs_auth_block_oscar() &&
		(
			g_str_equal(account->protocol_id, "prpl-aim") ||
			g_str_equal(account->protocol_id, "prpl-icq")
		))
	{
		purple_debug_info("pidgin-pp",
			"Blocking OSCAR authorization request from %s\n",
			sender);
		return -1; // deny
	}

#if PURPLE_VERSION_CHECK(2, 8, 0)
	if (prefs_auth_block_with_url() && (msg != NULL))
	{
		const gchar *pattern = "http:\\/\\/";
		gboolean match = g_regex_match_simple(pattern, msg, 0, 0);

		if (match)
		{
			purple_debug_info("pidgin-pp",
				"Blocking auth request with url from %s\n"
				sender);
			return -1; // deny
		}
	}
#endif // PURPLE_VERSION_CHECK

	if (prefs_auth_block_repeated() && contact_is_blocked(sender))
	{
		return -1; // deny
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

	if (!contact_is_blocked(sender))
		add_to_block_list(sender);
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

	node_name = g_markup_escape_text (node->name, -1);

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

static void
block_contact_cb(PurpleBlistNode *node, gpointer data)
{
	const gchar* name = PURPLE_BLIST_NODE_NAME(node);
	add_to_block_list(name);
}

static void
unblock_contact_cb(PurpleBlistNode *node, gpointer data)
{
	const gchar *name = PURPLE_BLIST_NODE_NAME(node);
	remove_from_block_list(name);
}

static void
mouse_menu_cb(PurpleBlistNode *node, GList **menu)
{
	if (purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	const gchar *contact_name = PURPLE_BLIST_NODE_NAME(node);

	// don't show the contect menu on things other than contacts
	if (!contact_name)
		return;

	PurpleMenuAction *action = NULL;

	*menu = g_list_append(*menu, action);

	//purple_debug_info("pidgin-pp", "Clicked on %s\n", contact_name);

	if (contact_is_blocked(contact_name))
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

static void
del_button_clicked_cb(GtkWidget *widget, GtkTreeSelection *selection)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GValue value;

	memset(&value, 0, sizeof(GValue));

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get_value(model, &iter, 0, &value);
		gchar *name = (gchar *) g_value_get_string (&value);
		remove_from_block_list(name);

		if (gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
		{
			gtk_tree_selection_select_iter(selection, &iter);
		}
		g_value_unset(&value);
	}
}

static void
manage_blocked_users_cb(PurplePluginAction *action)
{
	GtkWidget *vbox, *window, *treeview;
	GtkWidget *del_button;
	GtkWidget *buttons;
	GtkWidget *scrolled_window;
	GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
	GtkTreeIter iter;
	GtkCellRenderer *cellrenderer;
	GtkTreeViewColumn *list_column;
	GtkTreeSelection *selection;
	GList *blocklist;

	blocklist = prefs_get_block_list();

	while (blocklist)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, blocklist->data, -1);
		blocklist = g_list_next(blocklist);
	}

	window = pidgin_create_window(_("Privacy Please"),
			PIDGIN_HIG_BORDER, NULL, TRUE);
	gtk_window_set_default_size(GTK_WINDOW(window), 380, 200);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	gtk_widget_show(vbox);

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_widget_set_size_request(treeview, 200, 150);

	cellrenderer = gtk_cell_renderer_text_new();
	list_column = gtk_tree_view_column_new_with_attributes
			(_("Blocked users"), cellrenderer, "text", 0, NULL);
	gtk_tree_view_column_set_min_width(list_column, 300);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), list_column);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(scrolled_window),
			GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
			GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);

	buttons = gtk_hbox_new(FALSE, 0);

	del_button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_box_pack_end(GTK_BOX(buttons), del_button, FALSE, FALSE, 0);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);
	gtk_box_pack_start(GTK_BOX(vbox), buttons, FALSE, FALSE, 0);

	gtk_widget_show_all(window);

	g_signal_connect(GTK_OBJECT(del_button), "clicked",
			GTK_SIGNAL_FUNC(del_button_clicked_cb), selection);
}

static GList *
actions(PurplePlugin *plugin, gpointer context)
{
	GList *l = NULL;
	PurplePluginAction *act = NULL;

	act = purple_plugin_action_new(_("Manage blocked users"),
			manage_blocked_users_cb);
	l = g_list_append(l, act);

	return l;
}

static gboolean
plugin_load (PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle ();
	void *acct_handle = purple_accounts_get_handle ();

	PurplePlugin *jabber = purple_find_prpl("prpl-jabber");

	prefs_load();

	purple_signal_connect(conv_handle, "receiving-im-msg",
			plugin, PURPLE_CALLBACK (receiving_im_msg_cb), NULL);
#if PURPLE_VERSION_CHECK(2, 8, 0)
	purple_signal_connect(acct_handle,
			"account-authorization-requested-with-message",
			plugin, PURPLE_CALLBACK (request_authorization_cb),
			NULL);
#else
	purple_signal_connect(acct_handle, "account-authorization-requested",
			plugin, PURPLE_CALLBACK (request_authorization_cb),
			NULL);
#endif // PURPLE_VERSION_CHECK
	purple_signal_connect(acct_handle, "account-authorization-denied",
			plugin, PURPLE_CALLBACK (authorization_deny_cb), NULL);
	purple_signal_connect(conv_handle, "blocked-im-msg",
			plugin, PURPLE_CALLBACK (msg_blocked_cb), NULL);

	if (jabber)
	{
		purple_signal_connect(jabber, "jabber-receiving-xmlnode",
			plugin, PURPLE_CALLBACK (jabber_xmlnode_cb), NULL);
	}
	else
	{
		purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp", "Jabber support "
					"missing - disabled headline blocking");
	}

	purple_signal_connect(purple_blist_get_handle(),
			"blist-node-extended-menu", plugin,
			PURPLE_CALLBACK(mouse_menu_cb), NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin * plugin)
{
	purple_signals_disconnect_by_handle (plugin);
	destroy_msg_list ();

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
