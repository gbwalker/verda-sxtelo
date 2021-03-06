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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>

#include "vsx-server.h"
#include "vsx-main-context.h"
#include "vsx-log.h"

static char *option_listen_address = "0.0.0.0";
static int option_listen_port = 5142;
static char *option_log_file = NULL;
static gboolean option_daemonize = FALSE;
static char *option_user = NULL;
static char *option_group = NULL;

static GOptionEntry
options[] =
  {
    {
      "address", 'a', 0, G_OPTION_ARG_STRING, &option_listen_address,
      "Address to listen on", "address"
    },
    {
      "port", 'p', 0, G_OPTION_ARG_INT, &option_listen_port,
      "Port to listen on", "port"
    },
    {
      "log", 'l', 0, G_OPTION_ARG_STRING, &option_log_file,
      "File to write log messages to", "file"
    },
    {
      "daemonize", 'd', 0, G_OPTION_ARG_NONE, &option_daemonize,
      "Launch the server in a separate detached process", NULL
    },
    {
      "user", 'u', 0, G_OPTION_ARG_STRING, &option_user,
      "Run the daemon as USER", "USER"
    },
    {
      "group", 'g', 0, G_OPTION_ARG_STRING, &option_group,
      "Run the daemon as GROUP", "GROUP"
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
  };

static gboolean
process_arguments (int *argc, char ***argv,
                   GError **error)
{
  GOptionContext *context;
  gboolean ret;
  GOptionGroup *group;

  group = g_option_group_new (NULL, /* name */
                              NULL, /* description */
                              NULL, /* help_description */
                              NULL, /* user_data */
                              NULL /* destroy notify */);
  g_option_group_add_entries (group, options);
  context = g_option_context_new ("- A server for practicing a "
                                  "foreign language");
  g_option_context_set_main_group (context, group);
  ret = g_option_context_parse (context, argc, argv, error);
  g_option_context_free (context);

  if (ret && *argc > 1)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION,
                   "Unknown option '%s'", (* argv)[1]);
      ret = FALSE;
    }

  return ret;
}

static VsxServer *
create_server (GError **error)
{
  GInetAddress *inet_address;
  VsxServer *server = NULL;

  inet_address = g_inet_address_new_from_string (option_listen_address);

  if (inet_address == NULL)
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                 "Failed to parse address '%s'",
                 option_listen_address);
  else
    {
      GSocketAddress *address = g_inet_socket_address_new (inet_address,
                                                           option_listen_port);

      server = vsx_server_new (address, error);

      g_object_unref (address);
      g_object_unref (inet_address);
    }

  return server;
}

static void
daemonize (void)
{
  pid_t pid, sid;

  pid = fork ();

  if (pid < 0)
    {
      g_warning ("fork failed: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }
  if (pid > 0)
    /* Parent process, we can just quit */
    exit (EXIT_SUCCESS);

  /* Reset the file mask (not really sure why we do this..) */
  umask (0);

  /* Create a new SID for the child process */
  sid = setsid ();
  if (sid < 0)
    {
      g_warning ("setsid failed: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  /* Change the working directory so we're resilient against it being
     removed */
  if (chdir ("/") < 0)
    {
      g_warning ("chdir failed: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  /* Redirect standard files to /dev/null */
  freopen ("/dev/null", "r", stdin);
  freopen ("/dev/null", "w", stdout);
  freopen ("/dev/null", "w", stderr);
}

static void
set_user (const char *user_name)
{
  struct passwd *user_info;

  user_info = getpwnam (user_name);

  if (user_info == NULL)
    {
      fprintf (stderr, "Unknown user \"%s\"\n", user_name);
      exit (EXIT_FAILURE);
    }

  if (setuid (user_info->pw_uid) == -1)
    {
      fprintf (stderr, "Error setting user privileges: %s\n",
               strerror (errno));
      exit (EXIT_FAILURE);
    }
}

static void
set_group (const char *group_name)
{
  struct group *group_info;

  group_info = getgrnam (group_name);

  if (group_info == NULL)
    {
      fprintf (stderr, "Unknown group \"%s\"\n", group_name);
      exit (EXIT_FAILURE);
    }

  if (setgid (group_info->gr_gid) == -1)
    {
      fprintf (stderr, "Error setting group privileges: %s\n",
               strerror (errno));
      exit (EXIT_FAILURE);
    }
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  VsxMainContext *mc;
  VsxServer *server;

  if (!process_arguments (&argc, &argv, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      return EXIT_FAILURE;
    }

  mc = vsx_main_context_get_default (&error);

  if (mc == NULL)
    {
      fprintf (stderr, "%s\n", error->message);
      g_clear_error (&error);
    }
  else
    {
      if (option_log_file
          && !vsx_log_set_file (option_log_file, &error))
        {
          fprintf (stderr, "Error setting log file: %s\n", error->message);
          g_clear_error (&error);
        }
      else
        {
          server = create_server (&error);

          if (server == NULL)
            {
              fprintf (stderr, "%s\n", error->message);
              g_clear_error (&error);
            }
          else
            {
              if (option_group)
                set_group (option_group);
              if (option_user)
                set_user (option_user);
              if (option_daemonize)
                daemonize ();

              if (!vsx_log_start (&error))
                {
                  /* This probably shouldn't happen. By the time we
                     get here may have daemonized so we can't really
                     print anything but let's do it anyway. */
                  g_warning ("Error starting log file: %s\n", error->message);
                  g_clear_error (&error);
                }
              else
                {
                  if (!vsx_server_run (server, &error))
                    vsx_log ("%s", error->message);

                  vsx_log ("Exiting...");
                }

              vsx_server_free (server);
            }

          vsx_log_close ();
        }

      vsx_main_context_free (mc);
    }

  return 0;
}
