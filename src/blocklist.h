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
#include <glib.h>

#ifndef PIDGIN_PP_BLOCKLIST_H
#define PIDGIN_PP_BLOCKLIST_H

void blocklist_add(const char *name);
void blocklist_manage(PurplePluginAction *action);
void blocklist_mouse_action(PurpleBlistNode *node, GList **menu);
gboolean blocklist_contains(const char *name);

#endif // PIDGIN_PP_BLOCKLIST_H
