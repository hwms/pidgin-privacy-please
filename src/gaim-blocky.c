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

// gaim headers for most plugins
#include "plugin.h"
#include "version.h"

// gaim headers for this plugin
#include "util.h"
#include "debug.h"
#include "account.h"
#include "privacy.h"
#include "blist.h"

// gaim header needed for gettext
#if GAIM_MAJOR_VERSION < 2
#include "internal.h"
#else
#define GETTEXT_PACKAGE "gtk20"
#include <glib/gi18n-lib.h>
#endif

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
blocky_get_message ()
{
	return gaim_prefs_get_string ("/plugins/core/gaim_blocky/message");
}

static gboolean
blocky_get_reply ()
{
	return gaim_prefs_get_bool ("/plugins/core/gaim_blocky/reply");
}

static const char*
blocky_get_unknown_msg ()
{
	return gaim_prefs_get_string
				("/plugins/core/gaim_blocky/unknown_message");
}

static gboolean
blocky_block_unknown ()
{
	return gaim_prefs_get_bool ("/plugins/core/gaim_blocky/unknown_block");
}

static gboolean
blocky_reply_unknown ()
{
	return gaim_prefs_get_bool ("/plugins/core/gaim_blocky/unknown_reply");
}

/**
 * This is our callback for the receiving-im-msg signal.
 *
 * We return TRUE to block the IM, FALSE to accept the IM
 */
static gboolean
receiving_im_msg_cb(GaimAccount* account, char **sender, char **buffer,
#if GAIM_MAJOR_VERSION >= 2
						GaimConversation * conv,
#endif
						int *flags, void *data)
{
	gaim_debug_info ("gaim-blocky", "Got message from %s\n", *sender);
	GaimBuddy* buddy = gaim_find_buddy(account, *sender);

	if (buddy == NULL) // No contact list entry
	{
		gaim_debug_info ("gaim-blocky", "Got message from unknown "
						"source: %s\n", *sender);

		if (blocky_block_unknown ())
		{
			gaim_debug_info ("gaim-blocky", "Blocked\n");

			if (blocky_reply_unknown ())
			{
				const char* msg = blocky_get_unknown_msg ();
				auto_reply (account, *sender, msg);
			}
			return TRUE; // block
		}
		else
		{
			gaim_debug_info ("gaim-blocky", "Allowed\n");
		}
	}
	else // Contact list entry exists
	{
        	const char* alias = gaim_buddy_get_alias_only (buddy);
		if (blocky_match (alias))
		{
			gaim_debug_info ("gaim-blocky", "Blocked %s\n", alias);

			if (blocky_get_reply ())
			{
				const char* msg = blocky_get_message ();
				auto_reply (account, *sender, msg);
			}
			
			return TRUE; // block
		}
		else
		{
			gaim_debug_info ("gaim-blocky", "Allowed %s\n", alias);
		}
	}
	return FALSE; // default: accept
}

static GaimPluginPrefFrame *
get_plugin_pref_frame (GaimPlugin* plugin)
{
	GaimPluginPrefFrame *frame;
	GaimPluginPref *ppref;

	frame = gaim_plugin_pref_frame_new();

	ppref = gaim_plugin_pref_new_with_label(_("Contact list entries"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/gaim_blocky/reply", _("Enable auto-reply"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/gaim_blocky/message", 
		_("Your auto-reply message:"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_label(_("Unknown people"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/gaim_blocky/unknown_block", _("Block messages from people not on your contact list"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label
		("/plugins/core/gaim_blocky/unknown_reply", _("Auto-reply on blocked messages with:"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name
				("/plugins/core/gaim_blocky/unknown_message");
	gaim_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static gboolean
plugin_load (GaimPlugin * plugin)
{
	void *conv_handle = gaim_conversations_get_handle ();

	gaim_prefs_add_none ("/plugins");
	gaim_prefs_add_none ("/plugins/core");
	gaim_prefs_add_none ("/plugins/core/gaim_blocky");

	gaim_prefs_add_bool ("/plugins/core/gaim_blocky/reply", FALSE);
	gaim_prefs_add_bool ("/plugins/core/gaim_blocky/unknown_block", FALSE);
	gaim_prefs_add_bool ("/plugins/core/gaim_blocky/unknown_reply", FALSE);
	gaim_prefs_add_string ("/plugins/core/gaim_blocky/message",
				_("Your message could not be delivered"));
	gaim_prefs_add_string ("/plugins/core/gaim_blocky/unknown_message",
		_("I currently only accept messages from people on my contact "
				" list - please request my authorization."));

	gaim_signal_connect (conv_handle, "receiving-im-msg",
			plugin, GAIM_CALLBACK (receiving_im_msg_cb), NULL);

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

static GaimPluginInfo info = {
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,				/**< type           */
	NULL,						/**< ui_requirement */
	0,						/**< flags          */
	NULL,						/**< dependencies   */
	GAIM_PRIORITY_DEFAULT,				/**< priority       */

	N_("core-gaim_blocky"),				/**< id             */
	N_("Blocky"),					/**< name           */
	VERSION,					/**< version        */
	N_("Block any contact protocol independently"), /**< summary	    */
							/**  description    */
	N_("A simple plugin which allows you to block messages from anyone, independently of protocol features.\n\nIn order to block a certain buddy, simple append \"   \" (three spaces) to his alias - any further messages from that buddy will be dropped silently or answered with an auto-reply message, depending on your configuration settings."),
	"Stefan Ott <stefan@desire.ch>",		/**< author         */
	"http://tools.desire.ch/gaim-blocky/",		/**< homepage       */

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
	return;
}

GAIM_INIT_PLUGIN(gaim_bs, init_plugin, info)
