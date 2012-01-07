/*
 * Gemelo - A server for chatting with strangers in a foreign language
 * Copyright (C) 2011  Neil Roberts
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

#ifndef __GML_WATCH_PERSON_HANDLER_H__
#define __GML_WATCH_PERSON_HANDLER_H__

#include <glib-object.h>
#include <gio/gio.h>
#include "gml-request-handler.h"

G_BEGIN_DECLS

#define GML_TYPE_WATCH_PERSON_HANDLER           \
  (gml_watch_person_handler_get_type())
#define GML_WATCH_PERSON_HANDLER(obj)                           \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               GML_TYPE_WATCH_PERSON_HANDLER,   \
                               GmlWatchPersonHandler))
#define GML_WATCH_PERSON_HANDLER_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                            \
                            GML_TYPE_WATCH_PERSON_HANDLER,      \
                            GmlWatchPersonHandlerClass))
#define GML_IS_WATCH_PERSON_HANDLER(obj)                        \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               GML_TYPE_WATCH_PERSON_HANDLER))
#define GML_IS_WATCH_PERSON_HANDLER_CLASS(klass)                \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            GML_TYPE_WATCH_PERSON_HANDLER))
#define GML_WATCH_PERSON_HANDLER_GET_CLASS(obj)                 \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              GML_WATCH_PERSON_HANDLER,         \
                              GmlWatchPersonHandlerClass))

typedef struct _GmlWatchPersonHandler      GmlWatchPersonHandler;
typedef struct _GmlWatchPersonHandlerClass GmlWatchPersonHandlerClass;

struct _GmlWatchPersonHandlerClass
{
  GmlRequestHandlerClass parent_class;
};

struct _GmlWatchPersonHandler
{
  GmlRequestHandler parent;

  GmlResponse *response;
};

GType
gml_watch_person_handler_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GML_WATCH_PERSON_HANDLER_H__ */