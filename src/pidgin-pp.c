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

// pidgin headers for most plugins
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
conf_msg_unknown_autoreply ()
{
	return purple_prefs_get_string
				("/plugins/core/pidgin_pp/unknown_message");
}

static gboolean
conf_block_unknown ()
{
	return purple_prefs_get_bool ("/plugins/core/pidgin_pp/unknown_block");
}

static gboolean
conf_reply_unknown ()
{
	return purple_prefs_get_bool ("/plugins/core/pidgin_pp/unknown_reply");
}

static const char*
conf_msg_blocked_autoreply ()
{
	return purple_prefs_get_string ("/plugins/core/pidgin_pp/message");
}

static gboolean
conf_reply_blocked ()
{
	return purple_prefs_get_bool ("/plugins/core/pidgin_pp/reply");
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
	purple_debug_info ("pidgin-pp", "Got message from %s\n", *sender);
	PurpleBuddy* buddy = purple_find_buddy(account, *sender);

	if (buddy == NULL) // No contact list entry
	{
		purple_debug_info ("pidgin-pp", "Got message from unknown "
						"source: %s\n", *sender);

		if (conf_block_unknown ())
		{
			purple_debug_info ("pidgin-pp", "Blocked\n");

			if (conf_reply_unknown ())
			{
				const char* msg = conf_msg_unknown_autoreply ();
				auto_reply (account, *sender, msg);
			}
			return TRUE; // block
		}
		else
		{
			purple_debug_info ("pidgin-pp", "Allowed\n");
		}
	}
	else // Contact list entry exists
	{
		const char* alias = purple_buddy_get_alias_only (buddy);
		purple_debug_info ("pidgin-pp", "Allowed %s\n", alias);
	}
	return FALSE; // default: accept
}

#if PURPLE_VERSION_CHECK (2, 3, 0)
static gboolean
request_authorization_cb (PurpleAccount* account, char *sender)
{
	int retval;

	if (!purple_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
	{
		return 0; // don't interfere, just prompt user
	}

	purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp", "Processing authorization"
						" request from %s\n", sender);

	// < 0: deny
	// = 0: prompt user
	// > 0: accept
	retval = -!purple_privacy_check (account, sender);

	if (!retval && purple_prefs_get_bool 
				("/plugins/core/pidgin_pp/auth_auto_info")) {
		// Show the info dialog
		PurpleConnection* con = purple_account_get_connection (account);
		pidgin_retrieve_user_info (con, sender);
	}

	return retval;
}

static void
authorization_deny_cb (PurpleAccount* account, char *sender)
{
	if (!purple_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return;

	purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp", "Processing rejected "
				"authorization request from %s\n", sender);
	if (purple_privacy_check (account, sender))
		purple_privacy_deny_add (account, sender, FALSE);
}

#else
// This is for compatibility with the old patches and will be removed soon

static gboolean
request_authorization_cb (PurpleAccount* account, char **sender)
{
	if (!purple_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return FALSE;

	purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp", "Processing authorization "
						"request from %s\n", *sender);
	// FALSE -> accept
	return !purple_privacy_check (account, *sender);
}

static void
authorization_deny_cb (PurpleAccount* account, char **sender)
{
	if (!purple_prefs_get_bool ("/plugins/core/pidgin_pp/block_denied"))
		return;

	purple_debug (PURPLE_DEBUG_INFO, "pidgin-pp", "Processing rejected "
				"authorization request from %s\n", *sender);
	if (purple_privacy_check (account, *sender))
		purple_privacy_deny_add (account, *sender, FALSE);
}
#endif

static void
msg_blocked_cb (PurpleAccount* account, char **sender)
{
	if (conf_reply_blocked ())
	{
		const char* msg = conf_msg_blocked_autoreply ();
		auto_reply (account, *sender, msg);
	}
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
		("/plugins/core/pidgin_pp/auth_auto_info", _("Automatically show user info on authorization requests"));
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static gboolean
plugin_load (PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle ();
	void *acct_handle = purple_accounts_get_handle ();

	purple_prefs_add_bool ("/plugins/core/pidgin_pp/reply", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/unknown_block", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/unknown_reply", FALSE);
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/auth_auto_info", FALSE);
	purple_prefs_add_string ("/plugins/core/pidgin_pp/message",
				_("Your message could not be delivered"));
	purple_prefs_add_string ("/plugins/core/pidgin_pp/unknown_message",
		_("I currently only accept messages from people on my contact"
				" list - please request my authorization."));
	purple_prefs_add_bool ("/plugins/core/pidgin_pp/block_denied", FALSE);

	purple_signal_connect (conv_handle, "receiving-im-msg",
			plugin, PURPLE_CALLBACK (receiving_im_msg_cb), NULL);
#if PURPLE_VERSION_CHECK (2, 3, 0)
	purple_signal_connect (acct_handle, "account-authorization-requested",
			plugin, PURPLE_CALLBACK (request_authorization_cb), NULL);
#else
// This is for compatibility with the old patches and will be removed soon
	purple_signal_connect (acct_handle, "account-request-authorization",
			plugin, PURPLE_CALLBACK (request_authorization_cb), NULL);
#endif
	purple_signal_connect (acct_handle, "account-authorization-denied",
			plugin, PURPLE_CALLBACK (authorization_deny_cb), NULL);
	purple_signal_connect (conv_handle, "blocked-im-msg",
			plugin, PURPLE_CALLBACK (msg_blocked_cb), NULL);
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
init_plugin (PurplePlugin * plugin)
{
	purple_prefs_add_none ("/plugins");
	purple_prefs_add_none ("/plugins/core");
	purple_prefs_add_none ("/plugins/core/pidgin_pp");
}

PURPLE_INIT_PLUGIN(pidgin_pp, init_plugin, info)
