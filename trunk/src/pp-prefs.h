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

#ifndef PIDGIN_PP_PREFS_H
#define PIDGIN_PP_PREFS_H

#include <gtk/gtk.h>

// auto-reply
gboolean prefs_autoreply_unknown();
gboolean prefs_autoreply_blocked();
const char* prefs_autoreply_unknown_msg();
const char* prefs_autoreply_blocked_msg();

// msg
const char* prefs_block_account_regex();
const char* prefs_block_message_regex();
const char* prefs_deny_auth_regex();
gboolean prefs_block_unknown();
gboolean prefs_block_account_using_regex();
gboolean prefs_block_message_using_regex();
gboolean prefs_deny_auth_using_regex();

// auth
gboolean prefs_auth_block_all();
gboolean prefs_auth_block_oscar();
gboolean prefs_auth_block_with_url();
gboolean prefs_auth_block_repeated();
gboolean prefs_auth_auto_info();

// botcheck
gboolean prefs_botcheck_enabled();
const char* prefs_botcheck_question();
const char* prefs_botcheck_answer();
const char* prefs_botcheck_ok();

// protocol-specific
gboolean prefs_block_jabber_headlines();
gboolean prefs_block_aol_sysmsg();
gboolean prefs_allow_all_irc();

void prefs_set_block_list(GList *blocklist);

void prefs_init();
void prefs_load();

GList* prefs_get_block_list();

GtkWidget* get_plugin_config_frame(PurplePlugin *plugin);
#endif
