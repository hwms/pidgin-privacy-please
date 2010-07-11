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
#include <gtkprefs.h>

// our auto-reply functionality
#include "auto-reply.h"
#include "botcheck.h"

#ifdef WIN32
#include "win32dep.h"
#endif

#ifdef ENABLE_NLS
#include <glib/gi18n-lib.h>
#else
#define _(String) ((const char *) (String))
#define N_(String) ((const char *) (String))
#endif // ENABLE NLS

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
conf_block_account_using_regex()
{
	return purple_prefs_get_bool(
			"/plugins/core/pidgin_pp/block_account_with_regex");
}

static const gchar*
conf_get_account_regex()
{
	return purple_prefs_get_string(
			"/plugins/core/pidgin_pp/block_account_regex");
}

static gboolean
conf_block_message_using_regex()
{
	return purple_prefs_get_bool(
			"/plugins/core/pidgin_pp/block_message_with_regex");
}

static const gchar*
conf_get_message_regex()
{
	return purple_prefs_get_string(
			"/plugins/core/pidgin_pp/block_message_regex");
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

static gboolean
conf_botcheck_enabled()
{
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/botcheck_enable");
}

static GList*
conf_get_block_list()
{
	return purple_prefs_get_string_list("/plugins/core/pidgin_pp/block");
}

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

	GList* blocklist = conf_get_block_list();
	blocklist = g_list_append(blocklist, (gpointer) name);
	purple_prefs_set_string_list("/plugins/core/pidgin_pp/block",
			blocklist);
}

static void
remove_from_block_list(const gchar *name)
{
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

static gboolean
contact_is_blocked(const gchar *name)
{
	GList* blocklist = conf_get_block_list();

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

	if (conf_reply_blocked())
	{
		const char* msg = conf_msg_blocked_autoreply();
		auto_reply(account, sender, msg);
	}
}

#if GLIB_CHECK_VERSION(2,14,0)
static gboolean
pp_match_sender_regex(char *sender)
{
	purple_debug_info("pidgin-pp", "Block '%s' using regex?\n", sender);
	const gchar *pattern = conf_get_account_regex();

	return g_regex_match_simple(pattern, sender, 0, 0);
}

static gboolean
pp_match_msg_regex(char *message)
{
	purple_debug_info("pidgin-pp", "Block '%s' using regex?\n", message);
	const gchar *pattern = conf_get_message_regex();

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
//						int *flags, void *data)
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
	if ((!strcmp(account->protocol_id, "prpl-irc")) && conf_allow_all_irc())
		return FALSE;

	// block AOL system messages
	if (conf_block_aol_sysmsg() && !strcmp(*sender, "AOL System Msg"))
	{
		purple_debug_info("pidgin-pp", "Blocking AOL system message\n");
		return TRUE; // block
	}

#if GLIB_CHECK_VERSION(2,14,0)
	// block using account regex
	if (conf_block_account_using_regex() && pp_match_sender_regex(*sender))
	{
		purple_debug_info(
			"pidgin-pp", "Blocking account using regex\n");

		// TODO: pidgin should actually emit a signal when we block a
		// message (but it doesn't). remember to file a bug report.
		msg_blocked_cb(account, *sender);
		return TRUE; // block
	}

	// block using message regex
	if (conf_block_message_using_regex() && pp_match_msg_regex(*message))
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
	if (conf_botcheck_enabled())
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

static int
request_authorization_cb (PurpleAccount* account, char *sender, char *msg)
{
	// < 0: deny
	// = 0: prompt user
	// > 0: accept

	if (msg != NULL)
	{
		purple_debug_info("pidgin-pp", "message: %s\n", msg);
		const gchar *pattern = "test pattern";

		gboolean match = g_regex_match_simple(pattern, msg, 0, 0);
		purple_debug_info("pidgin-pp", "match: %d\n", match);
	}
	else
	{
		purple_debug_info("pidgin-pp", "NO MESSAGE\n");
	}

	purple_debug_info("pidgin-pp", "request_authorization_cb\n");

	if (purple_prefs_get_bool("/plugins/core/pidgin_pp/block_auth_all"))
	{
		purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp", "Blocking "
				"authorization request from %s\n", sender);
		return -1;
	}

	if (purple_prefs_get_bool("/plugins/core/pidgin_pp/block_auth_oscar") &&
		(g_str_equal(account->protocol_id, "prpl-aim") ||
		g_str_equal(account->protocol_id, "prpl-icq")))
	{
		purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp", "Blocking "
				"OSCAR authorization request from %s\n", sender);
		return -1;
	}

	if (!purple_prefs_get_bool("/plugins/core/pidgin_pp/block_denied"))
	{
		return 0; // don't interfere, just prompt user
	}

	purple_debug(PURPLE_DEBUG_INFO, "pidgin-pp", "Processing authorization"
						" request from %s\n", sender);
	if (contact_is_blocked(sender))
		return -1;

	if (purple_prefs_get_bool("/plugins/core/pidgin_pp/auth_auto_info"))
	{
		// Show the info dialog
		PurpleConnection* con = purple_account_get_connection(account);
		pidgin_retrieve_user_info(con, sender);
	}
	return 0;
}

static void
authorization_deny_cb(PurpleAccount* account, char *sender)
{
	if (!purple_prefs_get_bool("/plugins/core/pidgin_pp/block_denied"))
		return;

	purple_debug_info("pidgin-pp", "Processing rejected "
				"authorization request from %s\n", sender);

	if (!contact_is_blocked(sender))
		add_to_block_list(sender);
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

	purple_debug_info("pidgin-pp", "CONTACT NAME IS %s\n", contact_name);

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

	blocklist = conf_get_block_list();

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

static GtkWidget *
get_plugin_config_frame(PurplePlugin *plugin)
{
	GtkWidget *notebook;
	GtkWidget *config_vbox;
	GtkWidget *tab_vbox;
	GtkSizeGroup *sg;

	config_vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(config_vbox), 12);
	gtk_widget_show(config_vbox);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(config_vbox), notebook, 0, 0, 0);
	gtk_widget_show(notebook);

	// Notebook page 1: Auto-reply

	tab_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 5);
	gtk_widget_show(tab_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab_vbox,
			gtk_label_new(_("Auto-reply")));

	pidgin_prefs_checkbox(_("Auto-reply on blocked messages with:"),
			"/plugins/core/pidgin_pp/reply", tab_vbox);
	pidgin_prefs_labeled_entry(tab_vbox, "    ",
			"/plugins/core/pidgin_pp/message", 0);
	pidgin_prefs_checkbox(
		_("Auto-reply on blocked messages from unknown people with:"),
		"/plugins/core/pidgin_pp/unknown_reply", tab_vbox);
	pidgin_prefs_labeled_entry(tab_vbox, "    ",
			"/plugins/core/pidgin_pp/unknown_message", 0);

	// Notebook page 2: Messages

	tab_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 5);
	gtk_widget_show(tab_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab_vbox,
			gtk_label_new(_("Messages")));

	pidgin_prefs_checkbox(
		_("Block messages from people not on your contact list"),
		"/plugins/core/pidgin_pp/unknown_block", tab_vbox);

#if GLIB_CHECK_VERSION(2,14,0)
	pidgin_prefs_checkbox(
		_("Block messages that match a regular expression:"),
		"/plugins/core/pidgin_pp/block_message_with_regex", tab_vbox);
	pidgin_prefs_labeled_entry(tab_vbox, "    ",
		"/plugins/core/pidgin_pp/block_message_regex", 0);

	pidgin_prefs_checkbox(_(
		"Block messages from senders that match a regular expression:"),
		"/plugins/core/pidgin_pp/block_account_with_regex", tab_vbox);
	pidgin_prefs_labeled_entry(tab_vbox, "    ",
		"/plugins/core/pidgin_pp/block_account_regex", 0);
#endif // GLIB_CHECK_VERSION

	// Notebook page 3: Authorization

	tab_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 5);
	gtk_widget_show(tab_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab_vbox,
			gtk_label_new(_("Authorization")));

	pidgin_prefs_checkbox(
		_("Suppress repeated authorization requests\n(requires privacy settings to block individual users)"),
		"/plugins/core/pidgin_pp/block_denied", tab_vbox);
	pidgin_prefs_checkbox(
		_("Block all authorization requests"),
		"/plugins/core/pidgin_pp/block_auth_all", tab_vbox);
	pidgin_prefs_checkbox(
		_("Block authorization requests from OSCAR (ICQ/AIM)"),
		"/plugins/core/pidgin_pp/block_auth_oscar", tab_vbox);
	pidgin_prefs_checkbox(
		_("Automatically show user info on authorization requests"),
		"/plugins/core/pidgin_pp/auth_auto_info", tab_vbox);

	// Notebook page 4: Bot check

	tab_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 5);
	gtk_widget_show(tab_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab_vbox,
			gtk_label_new(_("Bot check")));

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	pidgin_prefs_checkbox(
		_("Verify message sender by asking a question"),
		"/plugins/core/pidgin_pp/botcheck_enable", tab_vbox);
	pidgin_prefs_labeled_entry(tab_vbox, _("Question:"),
		"/plugins/core/pidgin_pp/botcheck_question", sg);
	pidgin_prefs_labeled_entry(tab_vbox, _("Answer:"),
		"/plugins/core/pidgin_pp/botcheck_answer", sg);
	pidgin_prefs_labeled_entry(tab_vbox, _("OK message:"),
		"/plugins/core/pidgin_pp/botcheck_ok", sg);

	// Notebook page 5: Protocol specific

	tab_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 5);
	gtk_widget_show(tab_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab_vbox,
			gtk_label_new(_("Protocol specific")));


	pidgin_prefs_checkbox(
		_("Block jabber headline messages (MSN alerts, announcements etc.)"),
		"/plugins/core/pidgin_pp/block_jabber_headlines", tab_vbox);
	pidgin_prefs_checkbox(
		_("Allow all messages on IRC"),
		"/plugins/core/pidgin_pp/allow_all_irc", tab_vbox);
	pidgin_prefs_checkbox(
		_("Block AOL system messages"),
		"/plugins/core/pidgin_pp/block_aol_sys", tab_vbox);

	return config_vbox;
}

static gboolean
plugin_load (PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle ();
	void *acct_handle = purple_accounts_get_handle ();

	PurplePlugin *jabber = purple_find_prpl("prpl-jabber");

	purple_prefs_add_bool("/plugins/core/pidgin_pp/reply", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/unknown_block", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/unknown_reply", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/auth_auto_info", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_jabber_headlines",
			FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/allow_all_irc", TRUE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_aol_sys", FALSE);
	purple_prefs_add_string("/plugins/core/pidgin_pp/message",
				_("Your message could not be delivered"));
	purple_prefs_add_string("/plugins/core/pidgin_pp/unknown_message",
		_("I currently only accept messages from people on my contact"
				" list - please request my authorization."));
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_denied", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_auth_all", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_auth_oscar", FALSE);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_account_with_regex", FALSE);
	purple_prefs_add_string("/plugins/core/pidgin_pp/block_account_regex",
			"spam.*bot");
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_message_with_regex", FALSE);
	purple_prefs_add_string("/plugins/core/pidgin_pp/block_message_regex",
			"(leather jackets?|gold watch)");
	purple_prefs_add_string_list("/plugins/core/pidgin_pp/block", NULL);
	purple_prefs_add_bool("/plugins/core/pidgin_pp/botcheck_enable", FALSE);
	purple_prefs_add_string("/plugins/core/pidgin_pp/botcheck_question",
		_("To prove that you are human, please enter the result of 8+3"));
	purple_prefs_add_string("/plugins/core/pidgin_pp/botcheck_answer",
		_("11"));
	purple_prefs_add_string("/plugins/core/pidgin_pp/botcheck_ok",
		_("Very well then, you may speak"));

	purple_signal_connect(conv_handle, "receiving-im-msg",
			plugin, PURPLE_CALLBACK (receiving_im_msg_cb), NULL);
	purple_signal_connect(acct_handle, "account-authorization-requested",
			plugin, PURPLE_CALLBACK (request_authorization_cb),
			NULL);
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

	purple_prefs_add_none("/plugins");
	purple_prefs_add_none("/plugins/core");
	purple_prefs_add_none("/plugins/core/pidgin_pp");
}

PURPLE_INIT_PLUGIN(pidgin_pp, init_plugin, info)
