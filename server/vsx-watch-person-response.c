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

#include <string.h>
#include <stdio.h>

#include "vsx-watch-person-response.h"
#include "vsx-main-context.h"

/* Interval in microseconds between keep-alive messages */
#define VSX_WATCH_PERSON_RESPONSE_KEEP_ALIVE_INTERVAL 60000000 /* 1 minute */

static guint8
header_message[] =
  "HTTP/1.1 200 OK\r\n"
  VSX_RESPONSE_COMMON_HEADERS
  VSX_RESPONSE_DISABLE_CACHE_HEADERS
  "Content-Type: text/plain; charset=UTF-8\r\n"
  "Transfer-Encoding: chunked\r\n"
  "\r\n"
  "fb\r\n"
  "[\"padding\", \"This padding is here because it seems that for some reason "
  "some browsers don't notify Javascript that there is a new chunk of data "
  "until at least 1024 bytes of the response are received. Just think of all "
  "those wasted bytes! It's sad.\"]\r\n"
  "\r\n"
  "fe\r\n"
  "[\"padding\", \"Here's a joke to pass the while this padding is being "
  "downloaded. Why is a giraffe never alone? Because it has a long neck. "
  "It's not very funny. I apologise for that. Why are you reading this anyway? "
  "Don't you have anything better to do?\"]\r\n"
  "\r\n"
  "ee\r\n"
  "[\"padding\", \"Ĉi tiuj plenumiloj estas ĉi tie ĉar ŝajne ial iuj retumiloj "
  "ne informas na Javascript ke nova datumoj alvenis ĝis almenaŭ 1024 bajtoj "
  "da la respondo reciviĝas. Pensu pri tiu malŝparo de bajtoj! Tio estas "
  "tristiga.\"]\r\n"
  "\r\n"
  "112\r\n"
  "[\"padding\", \"Jen ŝerco por pasigi la tempon dum ĉi tiu malŝparo "
  "elŝutas. Kial girafo neniam solas? Ĉar ĝi havas kolegon. Ĝi estas bona "
  "ŝerco ĉu ne? Mi ŝatas ĝin ĉar ĝi ne havas sencon en la angla. Do jen la "
  "fino kaj nun povas komenci la veraj datumoj. Ĝuu!\"]\r\n"
  "\r\n";

static guint8
keep_alive_message[] =
  "10\r\n"
  "[\"keep-alive\"]\r\n"
  "\r\n";

static guint8
sync_message[] =
  "a\r\n"
  "[\"sync\"]\r\n"
  "\r\n";

static guint8
end_message[] =
  "9\r\n"
  "[\"end\"]\r\n"
  "\r\n"
  "0\r\n"
  "\r\n";

typedef struct
{
  guint8 *data;
  unsigned int length;
} WriteMessageData;

static gboolean
write_message (VsxWatchPersonResponse *self,
               WriteMessageData *message_data,
               const guint8 *message,
               unsigned int message_length)
{
  unsigned int to_write = MIN (message_data->length,
                               message_length - self->message_pos);

  memcpy (message_data->data, message + self->message_pos, to_write);
  message_data->data += to_write;
  message_data->length -= to_write;
  self->message_pos += to_write;

  /* Some time (half a second) is subtracted from the last write time
   * so that if we are woken up exactly at the right interval then it
   * won't skip until the next interval due to the small amount of
   * time time it takes to write the message */
  self->last_write_time =
    vsx_main_context_get_monotonic_clock (NULL) - 500000;

  return self->message_pos >= message_length;
}

#define write_static_message(self, message_data, message) \
  write_message (self, message_data, message, sizeof (message) - 1)

static gboolean
write_chunked_message (VsxWatchPersonResponse *self,
                       WriteMessageData *message_data,
                       const guint8 *message,
                       unsigned int message_length)
{
  char length_buf[8 + 2 + 1];
  unsigned int to_write;
  int length_length;

  length_length = sprintf (length_buf, "%x\r\n", message_length);

  if (self->message_pos < length_length)
    {
      to_write = MIN (message_data->length, length_length - self->message_pos);
      memcpy (message_data->data, length_buf + self->message_pos, to_write);
      message_data->length -= to_write;
      message_data->data += to_write;
      self->message_pos += to_write;

      if (message_data->length <= 0)
        return FALSE;
    }

  if (self->message_pos - length_length < message_length)
    {
      to_write = MIN (message_data->length,
                      message_length + length_length - self->message_pos);

      memcpy (message_data->data,
              message + self->message_pos - length_length,
              to_write);

      message_data->length -= to_write;
      message_data->data += to_write;
      self->message_pos += to_write;

      if (message_data->length <= 0)
        return FALSE;
    }

  to_write = MIN (message_data->length,
                  message_length + length_length + 2 - self->message_pos);

  memcpy (message_data->data,
          "\r\n" + self->message_pos - length_length - message_length,
          to_write);

  message_data->length -= to_write;
  message_data->data += to_write;
  self->message_pos += to_write;

  return self->message_pos >= length_length + message_length + 2;
}

static gboolean
has_pending_data (VsxWatchPersonResponse *self,
                  VsxWatchPersonResponseState *new_state)
{
  VsxConversation *conversation = self->person->conversation;
  int i;

  if (self->pending_n_tiles)
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_N_TILES;
      return TRUE;
    }

  if (self->named_players < conversation->n_players)
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_NAME;
      return TRUE;
    }

  for (i = 0; i < G_N_ELEMENTS (self->dirty_players); i++)
    if (self->dirty_players[i])
      {
        *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_PLAYER;
        return TRUE;
      }

  if (self->pending_shout != -1)
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_SHOUT;
      return TRUE;
    }

  for (i = 0; i < G_N_ELEMENTS (self->dirty_tiles); i++)
    if (self->dirty_tiles[i])
      {
        *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_TILE;
        return TRUE;
      }

  if (self->message_num < conversation->messages->len)
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES;
      return TRUE;
    }

  if (!vsx_player_is_connected (self->person->player))
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_END;
      return TRUE;
    }

  if (!self->sync_sent)
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_SYNC;
      return TRUE;
    }

  if (vsx_main_context_get_monotonic_clock (NULL) -
      self->last_write_time >= VSX_WATCH_PERSON_RESPONSE_KEEP_ALIVE_INTERVAL)
    {
      *new_state = VSX_WATCH_PERSON_RESPONSE_WRITING_KEEP_ALIVE;
      return TRUE;
    }

  return FALSE;
}

static unsigned int
vsx_watch_person_response_add_data (VsxResponse *response,
                                    guint8 *data_in,
                                    unsigned int length_in)
{
  VsxWatchPersonResponse *self = (VsxWatchPersonResponse *) response;
  WriteMessageData message_data;

  message_data.data = data_in;
  message_data.length = length_in;

  while (TRUE)
    switch (self->state)
      {
      case VSX_WATCH_PERSON_RESPONSE_WRITING_HTTP_HEADER:
        {
          if (write_static_message (self, &message_data, header_message))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER:
        {
          char buf[60];
          int length;

          length = sprintf (buf,
                            "[\"header\", {\"num\": %i, "
                            "\"id\": \"%016" G_GINT64_MODIFIER "X\"}]\r\n",
                            self->person->player->num,
                            self->person->id);

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) buf,
                                     length))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA:
        {
          VsxWatchPersonResponseState new_state;

          if (has_pending_data (self, &new_state))
            {
              self->message_pos = 0;
              self->state = new_state;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_N_TILES:
        {
          VsxConversation *conversation = self->person->conversation;
          char buf[15 + 10 + 1];
          int length;

          /* Decide how many tiles to write if we haven't started
           * writing yet */
          if (self->message_pos == 0)
            {
              self->dirty.n_tiles = conversation->total_n_tiles;

              /* We want to immediately reset the pending tiles flag
               * so that if it changes again while we are writing then
               * we will end up sending another message */
              if (message_data.length > 0)
                self->pending_n_tiles = FALSE;
            }

          length = sprintf (buf,
                            "[\"n-tiles\", %i]\r\n",
                            self->dirty.n_tiles);

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) buf,
                                     length))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_NAME:
        {
          VsxPlayer *player =
            self->person->conversation->players[self->named_players];

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) player->name_message,
                                     player->name_message_len))
            {
              self->message_pos = 0;
              if (++self->named_players
                  >= self->person->conversation->n_players)
                self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_PLAYER:
        {
          VsxPlayer *player;
          char buf[71];
          int length;

          /* Decide which player to update if we haven't started
           * writing yet */
          if (self->message_pos == 0)
            {
              self->dirty.player.num =
                vsx_flags_find_first_bit (self->dirty_players);

              player =
                self->person->conversation->players[self->dirty.player.num];
              self->dirty.player.flags = player->flags;

              /* We want to immediately mark the player as not dirty
               * so that it changes again while we are still in this
               * state then we will end up sending another message
               * with the new state */
              if (message_data.length > 0)
                  VSX_FLAGS_SET (self->dirty_players,
                                 self->dirty.player.num,
                                 FALSE);
            }
          else
            player =
              self->person->conversation->players[self->dirty.player.num];

          length = sprintf (buf,
                            "[\"player\", {\"num\": %u, \"flags\": %i}]\r\n",
                            self->dirty.player.num,
                            self->dirty.player.flags);

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) buf,
                                     length))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_SHOUT:
        {
          char buf[24];
          int length;

          length = sprintf (buf,
                            "[\"shout\", %u]\r\n",
                            self->pending_shout);

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) buf,
                                     length))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
              self->pending_shout = -1;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_TILE:
        {
          const VsxTile *tile;
          char buf[61 + 10 + 6 + 6 + VSX_TILE_MAX_LETTER_BYTES + 6 + 1];
          int length;

          /* Decide which tile to update if we haven't started
           * writing yet */
          if (self->message_pos == 0)
            {
              self->dirty.tile.num =
                vsx_flags_find_first_bit (self->dirty_tiles);

              tile =
                self->person->conversation->tiles + self->dirty.tile.num;
              self->dirty.tile.x = tile->x;
              self->dirty.tile.y = tile->y;
              self->dirty.tile.last_player = tile->last_player;

              /* We want to immediately mark the tile as not dirty
               * so that it changes again while we are still in this
               * state then we will end up sending another message
               * with the new state */
              if (message_data.length > 0)
                  VSX_FLAGS_SET (self->dirty_tiles,
                                 self->dirty.tile.num,
                                 FALSE);
            }
          else
            tile =
              self->person->conversation->tiles + self->dirty.tile.num;

          length = sprintf (buf,
                            "[\"tile\", {\"num\": %u, "
                            "\"x\": %i, \"y\": %i, "
                            "\"letter\": \"%s\", "
                            "\"player\": %i}]\r\n",
                            self->dirty.tile.num,
                            self->dirty.tile.x,
                            self->dirty.tile.y,
                            tile->letter,
                            self->dirty.tile.last_player);

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) buf,
                                     length))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES:
        {
          VsxConversation *conversation = self->person->conversation;
          VsxConversationMessage *message;

          message = &g_array_index (conversation->messages,
                                    VsxConversationMessage,
                                    self->message_num);

          if (write_chunked_message (self,
                                     &message_data,
                                     (const guint8 *) message->text,
                                     message->length))
            {
              self->message_pos = 0;
              self->message_num++;

              if (self->message_num >=
                  self->person->conversation->messages->len)
                self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_KEEP_ALIVE:
        {
          if (write_static_message (self, &message_data, keep_alive_message))
            self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_SYNC:
        {
          if (write_static_message (self, &message_data, sync_message))
            {
              self->sync_sent = TRUE;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_END:
        {
          if (write_static_message (self, &message_data, end_message))
            self->state = VSX_WATCH_PERSON_RESPONSE_DONE;
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_DONE:
        goto done;
      }

 done:
  return message_data.data - data_in;
}

static gboolean
vsx_watch_person_response_is_finished (VsxResponse *response)
{
  VsxWatchPersonResponse *self = (VsxWatchPersonResponse *) response;

  return self->state == VSX_WATCH_PERSON_RESPONSE_DONE;
}

static gboolean
vsx_watch_person_response_has_data (VsxResponse *response)
{
  VsxWatchPersonResponse *self = (VsxWatchPersonResponse *) response;

  switch (self->state)
    {
    case VSX_WATCH_PERSON_RESPONSE_WRITING_HTTP_HEADER:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER:
      return TRUE;

    case VSX_WATCH_PERSON_RESPONSE_AWAITING_DATA:
      {
        VsxWatchPersonResponseState new_state;

        return has_pending_data (self, &new_state);
      }

    case VSX_WATCH_PERSON_RESPONSE_WRITING_N_TILES:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_PLAYER:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_SHOUT:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_TILE:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_NAME:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_KEEP_ALIVE:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_SYNC:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_END:
      return TRUE;

    case VSX_WATCH_PERSON_RESPONSE_DONE:
      return FALSE;
    }

  g_warn_if_reached ();

  return FALSE;
}

static void
vsx_watch_person_response_free (void *object)
{
  VsxWatchPersonResponse *self = object;

  if (self->person)
    {
      vsx_list_remove (&self->conversation_changed_listener.link);
      vsx_object_unref (self->person);
    }

  vsx_main_context_remove_source (self->keep_alive_timer);

  vsx_response_get_class ()->parent_class.free (object);
}

static VsxResponseClass *
vsx_watch_person_response_get_class (void)
{
  static VsxResponseClass klass;

  if (klass.parent_class.free == NULL)
    {
      klass = *vsx_response_get_class ();

      klass.parent_class.instance_size = sizeof (VsxWatchPersonResponse);
      klass.parent_class.free = vsx_watch_person_response_free;

      klass.add_data = vsx_watch_person_response_add_data;
      klass.is_finished = vsx_watch_person_response_is_finished;
      klass.has_data = vsx_watch_person_response_has_data;
    }

  return &klass;
}

static void
conversation_changed_cb (VsxListener *listener,
                         void *user_data)
{
  VsxWatchPersonResponse *response =
    vsx_container_of (listener, response, conversation_changed_listener);
  VsxConversationChangedData *data = user_data;

  switch (data->type)
    {
    case VSX_CONVERSATION_N_TILES_CHANGED:
      response->pending_n_tiles = TRUE;
      break;

    case VSX_CONVERSATION_PLAYER_CHANGED:
      VSX_FLAGS_SET (response->dirty_players, data->num, TRUE);
      break;

    case VSX_CONVERSATION_TILE_CHANGED:
      VSX_FLAGS_SET (response->dirty_tiles, data->num, TRUE);
      break;

    case VSX_CONVERSATION_STATE_CHANGED:
    case VSX_CONVERSATION_MESSAGE_ADDED:
      break;

    case VSX_CONVERSATION_SHOUTED:
      if (response->state == VSX_WATCH_PERSON_RESPONSE_WRITING_SHOUT)
        return;
      response->pending_shout = data->num;
      break;
    }

  vsx_response_changed ((VsxResponse *) response);
}

static void
keep_alive_timer_cb (VsxMainContextSource *source,
                     void *user_data)
{
  VsxWatchPersonResponse *self = user_data;

  if (vsx_main_context_get_monotonic_clock (NULL) -
      self->last_write_time >= VSX_WATCH_PERSON_RESPONSE_KEEP_ALIVE_INTERVAL)
    vsx_response_changed ((VsxResponse *) self);
}

VsxResponse *
vsx_watch_person_response_new (VsxPerson *person,
                               int last_message)
{
  VsxWatchPersonResponse *self =
    vsx_object_allocate (vsx_watch_person_response_get_class ());

  vsx_response_init (self);

  self->person = vsx_object_ref (person);
  self->message_num = last_message;
  self->pending_shout = -1;
  self->pending_n_tiles = TRUE;

  self->last_write_time = vsx_main_context_get_monotonic_clock (NULL);
  self->keep_alive_timer =
    vsx_main_context_add_timer (NULL, /* default context */
                                VSX_WATCH_PERSON_RESPONSE_KEEP_ALIVE_INTERVAL /
                                60000000,
                                keep_alive_timer_cb,
                                self);

  vsx_flags_set_range (self->dirty_players,
                       person->conversation->n_players);
  vsx_flags_set_range (self->dirty_tiles,
                       person->conversation->n_tiles_in_play);

  self->conversation_changed_listener.notify = conversation_changed_cb;
  vsx_signal_add (&person->conversation->changed_signal,
                  &self->conversation_changed_listener);

  return (VsxResponse *) self;
}
