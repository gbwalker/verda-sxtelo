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

static guint8
header[] =
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
start[] =
  "1a\r\n"
  "[\"state\", \"in-progress\"]\r\n"
  "\r\n";

static guint8
end[] =
  "13\r\n"
  "[\"state\", \"done\"]\r\n"
  "\r\n"
  "0\r\n"
  "\r\n";

static guint8
header_first_person_start[] =
  "32\r\n"
  "[\"header\", {\"num\": 0, \"id\": \"";
static guint8
header_second_person_start[] =
  "32\r\n"
  "[\"header\", {\"num\": 1, \"id\": \"";
static guint8
header_end[] =
  "\"}]\r\n\r\n";

/* We need at least this much space before we'll consider adding a
   chunk to the buffer. The 8 is the length of 2³²-1 in hexadecimal,
   the first 2 is for the chunk length terminator and the second two
   is for the data terminator */
#define CHUNK_LENGTH_SIZE (8 + 2 + 2)

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

  return self->message_pos >= message_length;
}

#define write_static_message(self, message_data, message) \
  write_message (self, message_data, message, sizeof (message) - 1)

static gboolean
vsx_watch_person_response_get_typing_state (VsxWatchPersonResponse *self)
{
  VsxPerson *person = self->person;
  VsxConversation *conversation = person->conversation;

  return !!(conversation->typing_mask & (1 << (person->player->num ^ 1)));
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
          if (write_static_message (self, &message_data, header))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_START;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_START:
        {
          if (self->person->player->num == 0
              ? write_static_message (self, &message_data,
                                      header_first_person_start)
              : write_static_message (self, &message_data,
                                      header_second_person_start))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_ID;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_ID:
        {
          char id_buf[17];

          G_STATIC_ASSERT (sizeof (VsxPersonId) == sizeof (guint64));

          g_snprintf (id_buf, sizeof (id_buf),
                      "%016" G_GINT64_MODIFIER "X",
                      self->person->id);

          if (write_message (self, &message_data, (guint8 *) id_buf, 16))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_END;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_END:
        {
          if (write_static_message (self, &message_data, header_end))
            {
              self->message_pos = 0;
              self->state = VSX_WATCH_PERSON_RESPONSE_AWAITING_START;
            }
          else
            goto done;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_AWAITING_START:
        {
          VsxConversation *conversation = self->person->conversation;

          if (conversation->state == VSX_CONVERSATION_AWAITING_PARTNER)
            goto done;

          self->message_pos = 0;
          self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_START;
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_START:
        if (write_static_message (self, &message_data, start))
          {
            self->message_pos = 0;
            self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES;
          }
        else
          goto done;
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES:
        {
          VsxConversation *conversation = self->person->conversation;
          VsxConversationMessage *message;
          unsigned int to_write;
          int length_length;

          if (self->message_num >= conversation->messages->len)
            {
              gboolean typing_state;

              if (conversation->state == VSX_CONVERSATION_FINISHED)
                {
                  self->message_pos = 0;
                  self->state = VSX_WATCH_PERSON_RESPONSE_WRITING_END;
                }
              else if ((typing_state =
                        vsx_watch_person_response_get_typing_state (self))
                       != self->last_typing_state)
                {
                  static const guint8 typing_message[] =
                    "c\r\n"
                    "[\"typing\"]\r\n"
                    "\r\n";
                  static const guint8 not_typing_message[] =
                    "10\r\n"
                    "[\"not-typing\"]\r\n"
                    "\r\n";

                  if (typing_state)
                    {
                      /* Only add the typing mask if there's enough
                         space in the buffer for the entire message so
                         that we can't accidentally add it in the
                         middle of sending a message */
                      if (sizeof (typing_message) - 1 <= message_data.length)
                        {
                          write_static_message (self,
                                                &message_data,
                                                typing_message);
                          self->message_pos = 0;
                          self->last_typing_state = TRUE;
                        }
                      else
                        goto done;
                    }
                  else if (sizeof (not_typing_message) - 1
                           <= message_data.length)
                    {
                      write_static_message (self,
                                            &message_data,
                                            not_typing_message);
                      self->message_pos = 0;
                      self->last_typing_state = FALSE;
                    }
                  else
                    goto done;
                }
              else
                goto done;
            }
          else
            {
              /* If there's not enough space left in the buffer to
                 write a large chunk length then we'll wait until the
                 next call to add any data */
              if (message_data.length <= CHUNK_LENGTH_SIZE)
                goto done;

              message = &g_array_index (conversation->messages,
                                        VsxConversationMessage,
                                        self->message_num);

              to_write = MIN (message_data.length - CHUNK_LENGTH_SIZE,
                              message->length - self->message_pos);

              length_length = sprintf ((char *) message_data.data,
                                       "%x\r\n", to_write);

              message_data.length -= length_length;
              message_data.data += length_length;

              memcpy (message_data.data,
                      message->text + self->message_pos, to_write);
              memcpy (message_data.data + to_write, "\r\n", 2);

              message_data.length -= to_write + 2;
              message_data.data += to_write + 2;

              self->message_pos += to_write;

              if (self->message_pos >= message->length)
                {
                  self->message_pos = 0;
                  self->message_num++;
                }
            }
        }
        break;

      case VSX_WATCH_PERSON_RESPONSE_WRITING_END:
        {
          if (write_static_message (self, &message_data, end))
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
    case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_START:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_ID:
    case VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_END:
      return TRUE;

    case VSX_WATCH_PERSON_RESPONSE_AWAITING_START:
      return (self->person->conversation->state
              != VSX_CONVERSATION_AWAITING_PARTNER);

    case VSX_WATCH_PERSON_RESPONSE_WRITING_START:
      return TRUE;

    case VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES:
      if (self->person->conversation->state == VSX_CONVERSATION_FINISHED)
        return TRUE;

      return (self->message_num < self->person->conversation->messages->len
              || (self->last_typing_state
                  != vsx_watch_person_response_get_typing_state (self)));

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
      vsx_list_remove (&self->person_changed_listener.link);
      vsx_object_unref (self->person);
    }

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
person_changed_cb (VsxListener *listener,
                   void *data)
{
  VsxWatchPersonResponse *response =
    vsx_container_of (listener, response, person_changed_listener);

  vsx_response_changed ((VsxResponse *) response);
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

  self->person_changed_listener.notify = person_changed_cb;
  vsx_signal_add (&person->changed_signal,
                  &self->person_changed_listener);

  return (VsxResponse *) self;
}