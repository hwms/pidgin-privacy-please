/*
 * pidgin privacy please
 * Copyright (C) 2005-2008 Stefan Ott
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

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

// system headers
#include <glib.h>
#include <string.h>

// gaim headers for most plugins
#include "plugin.h"
#include "version.h"

// gaim headers for this plugin
#include "util.h"
#include "debug.h"
#include "account.h"
#include "privacy.h"
#include "blist.h"

// gaim <-> pidgin compatibility
#include "gaim-compat.h"

// gaim header needed for gettext
#define GETTEXT_PACKAGE "gtk20"
#include <glib/gi18n-lib.h>

// our auto-reply functionality
#include "auto-reply.h"

static gboolean
blocky_match (const char* s)
{
	if (s == NULL) return FALSE;
	char* tag = "   ";
	char* offset = strstr (s, tag);

	return (offset != NULL && offset - s == strlen (s) - strlen (tag));
}

static const char*
blocky_get_unknown_msg ()
{
	return gaim_prefs_get_string
				("/plugins/core/pidgin_pp/unknown_message");
}

static gboolean
blocky_block_unknown ()
{
	return gaim_prefs_get_bool ("/plugins/core/pidgin_pp/unknown_block");
}

static gboolean
blocky_reply_unknown ()
{
	return gaim_prefs_get_bool ("/plugins/core/pidgin_pp/unknown_reply");
}

static const char*
blocky_get_message ()
{
	return gaim_prefs_get_string ("/plugins/core/pidgin_pp/message");
}

static gboolean
blocky_get_reply ()
{
	return gaim_prefs_get_bool ("/plugins/core/pidgin_pp/reply");
}

/**
 * This is our callback for the receiving-im-msg signal.
 *
 * We return TRUE to block the IM, FALSE to accept the IM
 */
static gboolean
receiving_im_msg_cb(GaimAccount* account, char **sender, char **buffer,
						int *flags, void *data)
{
	gaim_debug_info ("pidgin-pp", "Got message from %s\n", *sender);
	GaimBuddy* buddy = gaim_find_buddy(account, *sender);

	if (buddy == NULL) // No contact list entry
	{
		gaim_debug_info ("pidgin-pp", "Got message from unknown "
						"source: %s\n", *sender);

		if (blocky_block_unknown ())
		{
			gaim_debug_info ("pidgin-pp", "Blocked\n");

			if (blocky_reply_unknown ())
			{
				const char* msg = blocky_get_unknown_msg ();
				auto_reply (account, *sender, msg);
			}
			return TRUE; // block
		}
		else
		{
			gaim_debug_info ("pidgin-pp", "Allowed\n");
		}
	}
	else // Contact list entry exists
	{
		const char* alias = gaim_buddy_get_alias_only (buddy);
		if (blocky_match (alias))
		{
			gaim_debug_info ("pidgin-pp", "Blocked %s\n", alias);

			if (blocky_get_reply ())
			{
				const char* msg = blocky_get_message ();
				auto_reply (account, *sender, msg);
			}
			return TRUE; // block
		}
		else
		{
			gaim_debug_info ("pidgin-pp", "Allowed %s\n", alias);
		}
	}
	return FALSE; // default: accept
}

#if GAIM_VERSION_CHECK (2, 3, 0)
static gboolean
request_authorization_cb (GaimAccount* account, char *sender)
{
	if (!gaim_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return 0; // prompt user

	gaim_debug (GAIM_DEBUG_INFO, "pidgin-pp", "Processing authorization "
						"request from %s\n", sender);
	// < 0: deny
	// = 0: prompt user
	// > 0: accept
	return -!gaim_privacy_check (account, sender);
}

static void
authorization_deny_cb (GaimAccount* account, char *sender)
{
	if (!gaim_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return;

	gaim_debug (GAIM_DEBUG_INFO, "pidgin-pp", "Processing rejected "
				"authorization request from %s\n", sender);
	if (gaim_privacy_check (account, sender))
		gaim_privacy_deny_add (account, sender, FALSE);
}

#else
// This is for compatibility with the old patches and will be removed soon

static gboolean
request_authorization_cb (GaimAccount* account, char **sender)
{
	if (!gaim_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return FALSE;

	gaim_debug (GAIM_DEBUG_INFO, "pidgin-pp", "Processing authorization "
						"request from %s\n", *sender);
	// FALSE -> accept
	return !gaim_privacy_check (account, *sender);
}

static void
authorization_deny_cb (GaimAccount* account, char **sender)
{
	if (!gaim_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return;

	gaim_debug (GAIM_DEBUG_INFO, "pidgin-pp", "Processing rejected "
				"authorization request from %s\n", *sender);
	if (gaim_privacy_check (account, *sender))
		gaim_privacy_deny_add (account, *sender, FALSE);
}
#endif

static void
msg_blocked_cb (GaimAccount* account, char **sender)
{
	if (blocky_get_reply ())
	{
		const char* msg = blocky_get_message ();
		auto_reply (account, *sender, msg);
	}
}

static GaimPluginPrefFrame *
get_plugin_pref_frame (GaimPlugin* plugin)
{
	GaimPluginPrefFrame *frame;
	GaimPluginPref *ppref;

	frame = gaim_plugin_pref_frame_new();

	ppref = gaim_plugin_pref_new_with_label(_("Blocked messages"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/reply", _("Auto-reply on blocked messages with:"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name
				("/plugins/core/pidgin_pp/message");
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_label(_("Unknown people"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/unknown_block", _("Block messages from people not on your contact list"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/unknown_reply", _("Auto-reply on blocked messages with:"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name
				("/plugins/core/pidgin_pp/unknown_message");
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_label(_("Authorization"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/pidgin_pp/block_denied", _("Suppress repeated authorization requests\n(requires privacy settings to block individual users)"));
	gaim_plugin_pref_frame_add(frame, ppref);
	return frame;
}

static gboolean
plugin_load (GaimPlugin * plugin)
{
	void *conv_handle = gaim_conversations_get_handle ();
	void *acct_handle = gaim_accounts_get_handle ();

	gaim_prefs_add_bool ("/plugins/core/pidgin_pp/reply", FALSE);
	gaim_prefs_add_bool ("/plugins/core/pidgin_pp/unknown_block", FALSE);
	gaim_prefs_add_bool ("/plugins/core/pidgin_pp/unknown_reply", FALSE);
	gaim_prefs_add_string ("/plugins/core/pidgin_pp/message",
				_("Your message could not be delivered"));
	gaim_prefs_add_string ("/plugins/core/pidgin_pp/unknown_message",
		_("I currently only accept messages from people on my contact"
				" list - please request my authorization."));
	gaim_prefs_add_bool ("/plugins/core/pidgin_pp/block_denied", FALSE);

	gaim_signal_connect (conv_handle, "receiving-im-msg",
			plugin, GAIM_CALLBACK (receiving_im_msg_cb), NULL);
#if GAIM_VERSION_CHECK (2, 3, 0)
	gaim_signal_connect (acct_handle, "account-authorization-requested",
			plugin, GAIM_CALLBACK (request_authorization_cb), NULL);
#else
// This is for compatibility with the old patches and will be removed soon
	gaim_signal_connect (acct_handle, "account-request-authorization",
			plugin, GAIM_CALLBACK (request_authorization_cb), NULL);
#endif
	gaim_signal_connect (acct_handle, "account-authorization-denied",
			plugin, GAIM_CALLBACK (authorization_deny_cb), NULL);
	gaim_signal_connect (conv_handle, "blocked-im-msg",
			plugin, GAIM_CALLBACK (msg_blocked_cb), NULL);
	return TRUE;
}

static gboolean
plugin_unload (GaimPlugin * plugin)
{
	gaim_signals_disconnect_by_handle (plugin);
	destroy_msg_list ();

	return TRUE;
}

static GaimPluginUiInfo prefs_info = { get_plugin_pref_frame };

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,				/**< type           */
	NULL,						/**< ui_requirement */
	0,						/**< flags          */
	NULL,						/**< dependencies   */
	GAIM_PRIORITY_DEFAULT,				/**< priority       */

	N_("core-pidgin_pp_"),				/**< id             */
	N_("Privacy Please"),				/**< name           */
	VERSION,					/**< version        */
	N_("Stops IM-spam"),				/**< summary	    */
							/**  description    */
	N_("A simple plugin to stop unwanted messages and repeated authorization requests from spammers."),
	"Stefan Ott <stefan@ott.net>",			/**< author         */
	"http://tools.desire.ch/pidgin-pp/",		/**< homepage       */

	plugin_load,					/**< load           */
	plugin_unload,					/**< unload         */
	NULL,						/**< destroy        */

	NULL,						/**< ui_info        */
	NULL,						/**< extra_info     */
	&prefs_info,					/**< prefs_info     */
	NULL
};

static void
init_plugin (GaimPlugin * plugin)
{
	gaim_prefs_add_none ("/plugins");
	gaim_prefs_add_none ("/plugins/core");
	gaim_prefs_add_none ("/plugins/core/pidgin_pp");
}

GAIM_INIT_PLUGIN(pidgin_pp, init_plugin, info)
