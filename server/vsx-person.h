/*
 * Verda Ŝtelo - An anagram game in Esperanto for the web
 * Copyright (C) 2011, 2013  Neil Roberts
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

#ifndef __VSX_PERSON_H__
#define __VSX_PERSON_H__

#include <glib.h>
#include <gio/gio.h>

#include "vsx-conversation.h"
#include "vsx-player.h"
#include "vsx-signal.h"

G_BEGIN_DECLS

typedef guint64 VsxPersonId;

typedef struct
{
  VsxObject parent;

  VsxSignal changed_signal;

  VsxPersonId id;

  VsxConversation *conversation;
  VsxListener conversation_changed_listener;

  VsxPlayer *player;

  gint64 last_noise_time;
} VsxPerson;

gboolean
vsx_person_id_equal (gconstpointer v1,
                     gconstpointer v2);

guint
vsx_person_id_hash (gconstpointer v);

VsxPersonId
vsx_person_generate_id (GSocketAddress *address);

gboolean
vsx_person_parse_id (const char *string,
                     VsxPersonId *id);

VsxPerson *
vsx_person_new (VsxPersonId id,
                const char *player_name,
                VsxConversation *conversation);

void
vsx_person_make_noise (VsxPerson *person);

gboolean
vsx_person_is_silent (VsxPerson *person);

G_END_DECLS

#endif /* __VSX_PERSON_H__ */