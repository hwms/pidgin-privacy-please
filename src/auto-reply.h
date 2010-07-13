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

#ifndef AUTO_REPLY_H
#define AUTO_REPLY_H

#define MAX_NAME_LENGTH 256
#define MSG_LIST_TIMEOUT 5000

typedef struct list_node {
	char *sender;
	guint timer;
	struct list_node *next;
} llnode;

void autoreply_cleanup();
void auto_reply(PurpleAccount*, const char *recipient, const char *message);
#endif
