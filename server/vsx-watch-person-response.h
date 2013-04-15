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

#ifndef __VSX_WATCH_PERSON_RESPONSE_H__
#define __VSX_WATCH_PERSON_RESPONSE_H__

#include "vsx-response.h"
#include "vsx-person.h"

G_BEGIN_DECLS

typedef struct
{
  VsxResponse parent;

  VsxPerson *person;

  VsxListener person_changed_listener;

  enum
  {
    VSX_WATCH_PERSON_RESPONSE_WRITING_HTTP_HEADER,
    VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_START,
    VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_ID,
    VSX_WATCH_PERSON_RESPONSE_WRITING_HEADER_END,
    VSX_WATCH_PERSON_RESPONSE_AWAITING_START,
    VSX_WATCH_PERSON_RESPONSE_WRITING_START,
    VSX_WATCH_PERSON_RESPONSE_WRITING_MESSAGES,
    VSX_WATCH_PERSON_RESPONSE_WRITING_END,
    VSX_WATCH_PERSON_RESPONSE_DONE
  } state;

  unsigned int message_num;
  unsigned int message_pos;

  gboolean last_typing_state;
} VsxWatchPersonResponse;

VsxResponse *
vsx_watch_person_response_new (VsxPerson *person,
                               int last_message);

G_END_DECLS

#endif /* __VSX_WATCH_PERSON_RESPONSE_H__ */