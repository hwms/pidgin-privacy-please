/*
 * pidgin privacy please
 * Copyright (C) 2005-2010 Stefan Ott
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

#include <purple.h>
#include <gtk/gtk.h>
#include <pidgin/gtkprefs.h>

// config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include "win32dep.h"
#endif

#ifdef ENABLE_NLS
#include <glib/gi18n-lib.h>
#else
#define _(String) ((const char *) (String))
#define N_(String) ((const char *) (String))
#endif // ENABLE NLS

#define pref_boolean(name, key) \
gboolean prefs_##name() \
{ \
	return purple_prefs_get_bool("/plugins/core/pidgin_pp/##key"); \
}

#define pref_string(name, key) \
const char* prefs_##name() \
{ \
	return purple_prefs_get_string("/plugins/core/pidgin_pp/##key"); \
}

// Auto-reply prefs

pref_boolean(reply_blocked, "reply");
pref_boolean(reply_unknown, "unknown_reply");
pref_string(autoreply_blocked_msg, "message");
pref_string(autoreply_unknown_msg, "unknown_message");

// Message prefs

pref_boolean(block_unknown, "unknown_block");
pref_boolean(block_account_using_regex, "block_account_with_regex");
pref_boolean(block_message_using_regex, "block_message_with_regex");
pref_string(block_account_regex, "block_account_regex");
pref_string(block_message_regex, "block_message_regex");

// Authorization prefs

pref_boolean(auth_block_all, "block_auth_all");
pref_boolean(auth_block_oscar, "block_auth_oscar");
#if PURPLE_VERSION_CHECK(2, 8, 0)
pref_boolean(auth_block_with_url, "block_auth_with_url");
#endif // PURPLE_VERSION_CHECK
pref_boolean(auth_block_repeated, "block_denied");
pref_boolean(auth_auto_info, "auth_auto_info");

// Botcheck prefs

pref_boolean(botcheck_enabled, "botcheck_enable");
pref_string(botcheck_question, "botcheck_question");
pref_string(botcheck_answer, "botcheck_answer");
pref_string(botcheck_ok, "botcheck_ok");

// Protocol-specific prefs

pref_boolean(block_jabber_headlines, "block_jabber_headlines");
pref_boolean(block_aol_sysmsg, "block_aol_sys");
pref_boolean(allow_all_irc, "allow_all_irc");

// END

GList*
prefs_get_block_list()
{
	return purple_prefs_get_string_list("/plugins/core/pidgin_pp/block");
}

void
prefs_set_block_list(GList *blocklist)
{
	purple_prefs_set_string_list("/plugins/core/pidgin_pp/block",
			blocklist);
}

GtkWidget *
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
		_("Suppress repeated authorization requests"),
		"/plugins/core/pidgin_pp/block_denied", tab_vbox);
	pidgin_prefs_checkbox(
		_("Block all authorization requests"),
		"/plugins/core/pidgin_pp/block_auth_all", tab_vbox);
	pidgin_prefs_checkbox(
		_("Block authorization requests from OSCAR (ICQ/AIM)"),
		"/plugins/core/pidgin_pp/block_auth_oscar", tab_vbox);
#if PURPLE_VERSION_CHECK(2, 8, 0)
	pidgin_prefs_checkbox(
		_("Block authorization requests with hyperlinks"),
		"/plugins/core/pidgin_pp/block_auth_with_url", tab_vbox);
#endif // PURPLE_VERSION_CHECK
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

void
prefs_init()
{
	purple_prefs_add_none("/plugins");
	purple_prefs_add_none("/plugins/core");
	purple_prefs_add_none("/plugins/core/pidgin_pp");
}

void
prefs_load()
{
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
#if PURPLE_VERSION_CHECK(2, 8, 0)
	purple_prefs_add_bool("/plugins/core/pidgin_pp/block_auth_with_url", FALSE);
#endif // PURPLE_VERSION_CHECK
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
}
