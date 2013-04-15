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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "vsx-player.h"

void
vsx_player_free (VsxPlayer *player)
{
  g_free (player->name);
  g_free (player->escaped_name);

  g_slice_free (VsxPlayer, player);
}

VsxPlayer *
vsx_player_new (const char *player_name,
                unsigned int num)
{
  VsxPlayer *player = g_slice_new (VsxPlayer);
  GString *buf = g_string_new (NULL);
  const char *p;

  player->name = g_strdup (player_name);
  player->num = num;

  /* Make an escaped version of the string so we can easily send it in
   * a JSON object */
  for (p = player_name; *p; p++)
    if (*p == '"' || *p == '\\')
      {
        g_string_append_c (buf, '\\');
        g_string_append_c (buf, *p);
      }
    else
      g_string_append_c (buf, *p);

  player->escaped_name_len = buf->len;
  player->escaped_name = g_string_free (buf, FALSE);

  return player;
}
