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

// config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// system headers
#include <glib.h>
#include <string.h>

// pidgin headers
#include <purple.h>
#include <pidgin.h>
#include <pidgin/gtkutils.h>

// pidgin-pp headers
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

void
blocklist_add(const gchar *name)
{
	GList* blocklist;

	// abort if a category was clicked
	if (!name)
	{
		purple_debug_info("pidgin-pp", "Not blocking (null)\n");
		return;
	}

	purple_debug_info("pidgin-pp", "Adding %s to block list\n", name);

	blocklist = prefs_get_block_list();
	blocklist = g_list_append(blocklist, (gpointer) name);
	prefs_set_block_list(blocklist);
}

static void
remove_from_block_list(const gchar *name)
{
	GList *blocklist, *tmp;

	purple_debug_info("pidgin-pp", "Removing %s from block list\n", name);

	blocklist = prefs_get_block_list();
	tmp = blocklist;

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

gboolean
blocklist_contains(const gchar *name)
{
	GList* blocklist;
	gchar *clean_name;

	blocklist = prefs_get_block_list();

	// we don't care about what's after the slash
	clean_name = strtok((gchar *) name, "/");

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
block_contact_cb(PurpleBlistNode *node, gpointer data)
{
	const gchar* name = PURPLE_BLIST_NODE_NAME(node);
	blocklist_add(name);
}

static void
unblock_contact_cb(PurpleBlistNode *node, gpointer data)
{
	const gchar *name = PURPLE_BLIST_NODE_NAME(node);
	remove_from_block_list(name);
}

void
blocklist_mouse_action(PurpleBlistNode *node, GList **menu)
{
	const gchar *contact_name;
	PurpleMenuAction *action;

	if (purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	contact_name = PURPLE_BLIST_NODE_NAME(node);

	// don't show the contect menu on things other than contacts
	if (!contact_name)
		return;

	//action = NULL;
	//*menu = g_list_append(*menu, action);	// add a separator

	//purple_debug_info("pidgin-pp", "Clicked on %s\n", contact_name);

	if (blocklist_contains(contact_name))
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

void
blocklist_manage(PurplePluginAction *action)
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
