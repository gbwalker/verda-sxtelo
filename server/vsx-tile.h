/*
 * Verda Ŝtelo - An anagram game in Esperanto for the web
 * Copyright (C) 2013  Neil Roberts
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

#ifndef __VSX_TILE_H__
#define __VSX_TILE_H__

#include <glib.h>

G_BEGIN_DECLS

/* Enough space for any unicode character in UTF-8 */
#define VSX_TILE_MAX_LETTER_BYTES 6

/* Size of a tile in tenths of an em */
#define VSX_TILE_SIZE 20
/* Gap to leave around each tile */
#define VSX_TILE_GAP 1

typedef struct
{
  gint16 x, y;
  gint16 last_player;
  char letter[VSX_TILE_MAX_LETTER_BYTES + 1];
} VsxTile;

G_END_DECLS

#endif /* __VSX_TILE_H__ */
