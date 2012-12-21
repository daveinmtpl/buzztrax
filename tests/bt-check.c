/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * testing helpers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION::btcheck:
 * @short_description: testing helpers
 */

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
//-- glib
#include <glib/gstdio.h>

#include "bt-check.h"

#ifdef HAVE_SETRLIMIT
#include <sys/time.h>
#include <sys/resource.h>
#endif

/* this is needed for a hack to make glib log lines gst-debug log alike
 * Using gst_debug_log would require use to set debug categories for each GLib
 * log domain.
 */
static GstClockTime _priv_gst_info_start_time;

void
bt_check_init (void)
{
  _priv_gst_info_start_time = gst_util_get_timestamp ();
  extern gboolean bt_test_plugin_init (GstPlugin * plugin);
  gst_plugin_register_static (GST_VERSION_MAJOR,
      GST_VERSION_MINOR,
      "bt-test",
      "buzztard test plugin - several unit test support elements",
      bt_test_plugin_init,
      VERSION, "LGPL", PACKAGE, PACKAGE_NAME, "http://www.buzztard.org");

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-check", 0,
      "music production environment / unit tests");
  // disable logging from gstreamer itself
  gst_debug_remove_log_function (gst_debug_log_default);
  // no ansi color codes in logfiles please
  gst_debug_set_colored (FALSE);
  // use our dummy settings
  bt_settings_set_factory ((BtSettingsFactory) bt_test_settings_new);

#ifdef HAVE_SETRLIMIT
  // only fork mode limit cpu/mem usage
  const gchar *mode = g_getenv ("CK_FORK");
  if (!mode || strcmp (mode, "no")) {
    struct rlimit rl;

    rl.rlim_max = RLIM_INFINITY;
    // limit cpu in seconds
    rl.rlim_cur = 20;
    if (setrlimit (RLIMIT_CPU, &rl) < 0)
      perror ("setrlimit(RLIMIT_CPU) failed");
    // limit process’s data size in bytes
    // if we get failing tests and "mmap() failed: Cannot allocate memory"
    // this limit needs to be increased
    rl.rlim_cur = 515 * 1024 * 1024;    // 0.5GB
    if (setrlimit (RLIMIT_DATA, &rl) < 0)
      perror ("setrlimit(RLIMIT_DATA) failed");
  }
#endif
}


/*
 * error trapping:
 * Check for certain log-output.
 */

static gboolean __check_error_trapped = FALSE;
static gchar *__check_method = NULL;
static gchar *__check_test = NULL;
static GLogLevelFlags __fatal_mask = 0;

#define FALLBACK_LOG_FILE_NAME \
    G_DIR_SEPARATOR_S "tmp" G_DIR_SEPARATOR_S "" PACKAGE_NAME ".log"

// set during setup_log_*() and leaked :/
static const gchar *__log_root = NULL;  // /tmp
static gchar *__log_base = NULL;        // the test binary
static gchar *__log_case = NULL;        // the test case
static const gchar *__log_test = NULL;  // the actual test
static gchar *__log_file_name = NULL;

/*
 * Install a new error trap for function and output.
 */
void
check_init_error_trapp (gchar * method, gchar * test)
{
  __check_method = method;
  __check_test = test;
  __check_error_trapped = FALSE;
  // in case the test suite made warnings and criticals fatal
  __fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
}

/*
 * Check if the monitored log-output has been emitted.
 * If Gstreamer has not been compiled using --gst-enable-debug, this returns
 * %TRUE as there is no logoutput at all.
 */
gboolean
check_has_error_trapped (void)
{
  g_log_set_always_fatal (__fatal_mask);
#ifndef GST_DISABLE_GST_DEBUG
  return (__check_error_trapped);
#else
  return (TRUE);
#endif
}

gboolean
_check_log_contains (gchar * text)
{
  FILE *logfile;
  gchar line[2000];
  gboolean res = FALSE;

  if (!(logfile = fopen (__log_file_name, "r")))
    return FALSE;

  while (!feof (logfile)) {
    if (fgets (line, 1999, logfile)) {
      if (strstr (line, text)) {
        res = TRUE;
        break;
      }
    }
  }

  fclose (logfile);
  return res;
}

/*
 * log helper:
 * Helpers for handling glib log-messages.
 */
static void
check_print_handler (const gchar * const message)
{
  if (message) {
    FILE *logfile;
    gboolean add_nl = FALSE;
    guint sl = strlen (message);
    gboolean use_stdout = FALSE;

    //-- check if messages has no newline
    if ((sl > 1) && (message[sl - 1] != '\n'))
      add_nl = TRUE;

    //-- check message contents
    if (__check_method && (strstr (message, __check_method) != NULL)
        && __check_test && (strstr (message, __check_test) != NULL))
      __check_error_trapped = TRUE;
    else if (__check_method && (strstr (message, __check_method) != NULL)
        && !__check_test)
      __check_error_trapped = TRUE;
    else if (__check_test && (strstr (message, __check_test) != NULL)
        && !__check_method)
      __check_error_trapped = TRUE;

    if ((logfile = fopen (__log_file_name, "a"))
        || (logfile = fopen (__log_file_name, "w"))) {
      use_stdout |= (fwrite (message, sl, 1, logfile) != 1);
      if (add_nl)
        use_stdout |= (fwrite ("\n", 1, 1, logfile) != 1);
      use_stdout |= (fclose (logfile) < 0);
    } else {
      use_stdout = TRUE;
    }

    if (use_stdout) {           /* Fall back to console output if unable to open file */
      printf ("%s", message);
      if (add_nl)
        putchar ('\n');
    }
  }
}

/*
static void check_critical_log_handler(const gchar * const log_domain, const GLogLevelFlags log_level, const gchar * const message, gpointer const user_data) {
  printf(">>> CRITICAL: %s\n",message);
  //-- check message contents
  if(__check_method  && (strstr(message,__check_method)!=NULL) && __check_test && (strstr(message,__check_test)!=NULL)) __check_error_trapped=TRUE;
  else if(__check_method && (strstr(message,__check_method)!=NULL) && !__check_test) __check_error_trapped=TRUE;
  else if(__check_test && (strstr(message,__check_test)!=NULL) && !__check_method) __check_error_trapped=TRUE;
}
*/

#if defined (GLIB_SIZEOF_VOID_P) && GLIB_SIZEOF_VOID_P == 8
#define PTR_FMT "14p"
#else
#define PTR_FMT "10p"
#endif
#define PID_FMT "5d"

static void
check_log_handler (const gchar * const log_domain,
    const GLogLevelFlags log_level, const gchar * const message,
    gpointer const user_data)
{
  gchar *msg, *level;
  GstClockTime elapsed;

  //-- check message contents
  if (__check_method && (strstr (message, __check_method) != NULL)
      && __check_test && (strstr (message, __check_test) != NULL))
    __check_error_trapped = TRUE;
  else if (__check_method && (strstr (message, __check_method) != NULL)
      && !__check_test)
    __check_error_trapped = TRUE;
  else if (__check_test && (strstr (message, __check_test) != NULL)
      && !__check_method)
    __check_error_trapped = TRUE;

  //-- format  
  switch (log_level & G_LOG_LEVEL_MASK) {
    case G_LOG_LEVEL_ERROR:
      level = "ERROR";
      break;
    case G_LOG_LEVEL_CRITICAL:
      level = "CRITICAL";
      break;
    case G_LOG_LEVEL_WARNING:
      level = "WARNING";
      break;
    case G_LOG_LEVEL_MESSAGE:
      level = "MESSAGE";
      break;
    case G_LOG_LEVEL_INFO:
      level = "INFO";
      break;
    case G_LOG_LEVEL_DEBUG:
      level = "DEBUG";
      break;
    default:
      level = "???";
      break;
  }

  elapsed =
      GST_CLOCK_DIFF (_priv_gst_info_start_time, gst_util_get_timestamp ());

  msg = g_alloca (85 + strlen (log_domain) + strlen (level) + strlen (message));
  g_sprintf (msg,
      "%" GST_TIME_FORMAT " %" PID_FMT " %" PTR_FMT " %-7s %20s ::: %s",
      GST_TIME_ARGS (elapsed), getpid (), g_thread_self (), level, log_domain,
      message);
  check_print_handler (msg);
}

#ifndef GST_DISABLE_GST_DEBUG
/* *INDENT-OFF* */
static void
check_gst_log_handler (GstDebugCategory * category,
    GstDebugLevel level, const gchar * file, const gchar * function, gint line,
    GObject * object, GstDebugMessage * _message, gpointer data)
    G_GNUC_NO_INSTRUMENT;
/* *INDENT-ON* */

static void
check_gst_log_handler (GstDebugCategory * category,
    GstDebugLevel level, const gchar * file, const gchar * function, gint line,
    GObject * object, GstDebugMessage * _message, gpointer data)
{
  const gchar *message = gst_debug_message_get (_message);
  gchar *msg, *obj_str;
  const gchar *level_str, *cat_str;
  GstClockTime elapsed;

  //-- check message contents
  if (__check_method && (strstr (function, __check_method) != NULL)
      && __check_test && (strstr (message, __check_test) != NULL))
    __check_error_trapped = TRUE;
  else if (__check_method && (strstr (function, __check_method) != NULL)
      && !__check_test)
    __check_error_trapped = TRUE;
  else if (__check_test && (strstr (message, __check_test) != NULL)
      && !__check_method)
    __check_error_trapped = TRUE;

  elapsed =
      GST_CLOCK_DIFF (_priv_gst_info_start_time, gst_util_get_timestamp ());
  level_str = gst_debug_level_get_name (level);
  cat_str = gst_debug_category_get_name (category);
  if (object) {
    if (GST_IS_OBJECT (object)) {
      obj_str = g_strdup_printf ("<%s,%" G_OBJECT_REF_COUNT_FMT ">",
          GST_OBJECT_NAME (object), G_OBJECT_LOG_REF_COUNT (object));
    } else if (GST_IS_OBJECT (object)) {
      obj_str = g_strdup_printf ("<%s,%" G_OBJECT_REF_COUNT_FMT ">",
          G_OBJECT_TYPE_NAME (object), G_OBJECT_LOG_REF_COUNT (object));
    } else {
      obj_str = g_strdup_printf ("%p", object);
    }
  } else {
    obj_str = g_strdup ("");
  }

  msg = g_alloca (95 + strlen (cat_str) + strlen (level_str) + strlen (message)
      + strlen (file) + strlen (function) + strlen (obj_str));
  g_sprintf (msg,
      "%" GST_TIME_FORMAT " %" PID_FMT " %" PTR_FMT " %-7s %20s %s:%d:%s:%s %s",
      GST_TIME_ARGS (elapsed), getpid (), g_thread_self (),
      level_str, cat_str, file, line, function, obj_str, message);
  g_free (obj_str);
  check_print_handler (msg);
}
#endif

static void
reset_log (gchar * path, gchar * file)
{
  if (path) {
    g_mkdir_with_parents (path, 0755);
  }
  g_unlink (file);
}

/*
 * setup_log_base:
 * @argc: command line argument count received in main()
 * @argv: command line arguments received in main()
 *
 * Initializes the logoutput channel.
 */
void
setup_log_base (gint argc, gchar ** argv)
{
  gchar *log, *path;

  __log_root = g_get_tmp_dir ();

  // get basename from argv[0]; -> lt-bt_edit
  if ((log = g_path_get_basename (argv[0]))) {
    // cut libtool prefix
    __log_base = g_strdup (&log[3]);
    g_free (log);
  } else {
    fprintf (stderr, "can't get basename from: '%s'\n", argv[0]);
    fflush (stderr);
    __log_base = PACKAGE_NAME;
  }

  log = g_strdup_printf ("%s.log", __log_base);
  //fprintf(stderr,"logname : '%s'\n",log);fflush(stderr);
  g_free (__log_file_name);
  if (!(__log_file_name = g_build_filename (__log_root, log, NULL))) {
    fprintf (stderr, "can't build logname from '%s','%s'\n", __log_root, log);
    fflush (stderr);
    __log_file_name = g_strdup (FALLBACK_LOG_FILE_NAME);
  }
  g_free (log);

  // ensure directory and reset log file
  path = g_build_filename (__log_root, __log_base, NULL);
  reset_log (path, __log_file_name);
  g_free (path);
}

void
setup_log_case (const gchar * file_name)
{
  gchar *log, *path;

  g_free (__log_case);
  if ((log = g_path_get_basename (file_name))) {
    // cut ".c" extension
    log[strlen (log) - 2] = '\0';
    __log_case = log;
  } else {
    fprintf (stderr, "can't get basename from: '%s'\n", file_name);
    fflush (stderr);
    __log_case = g_strdup ("case");
  }

  log = g_strdup_printf ("%s.log", __log_case);
  //fprintf(stderr,"logname : '%s'\n",log);fflush(stderr);
  g_free (__log_file_name);
  if (!(__log_file_name = g_build_filename (__log_root, __log_base, log, NULL))) {
    fprintf (stderr, "can't build logname from '%s','%s,'%s'\n",
        __log_root, __log_base, log);
    fflush (stderr);
    __log_file_name = g_strdup (FALLBACK_LOG_FILE_NAME);
  }
  g_free (log);

  // ensure directory and reset log file
  path = g_build_filename (__log_root, __log_base, __log_case, NULL);
  reset_log (path, __log_file_name);
  g_free (path);
}

void
setup_log_test (const gchar * func_name, gint i)
{
  static gchar *case_log_file_name = NULL;
  gchar *log;

  __log_test = func_name;
  if (func_name == NULL) {
    g_free (__log_file_name);
    __log_file_name = case_log_file_name;
    return;
  }

  log = g_strdup_printf ("%s.%d.log", __log_test, i);
  //fprintf(stderr,"logname : '%s'\n",log);fflush(stderr);
  case_log_file_name = __log_file_name;
  if (!(__log_file_name = g_build_filename (__log_root, __log_base, __log_case,
              log, NULL))) {
    fprintf (stderr, "can't build logname from '%s','%s,'%s,'%s'\n",
        __log_root, __log_base, __log_case, log);
    fflush (stderr);
    __log_file_name = g_strdup (FALLBACK_LOG_FILE_NAME);
  }
  g_free (log);

  // reset log file
  reset_log (NULL, __log_file_name);
}

/*
 * setup_log_capture:
 *
 * Installs own logging handlers to capture and channelize all diagnostic output
 * during testing.
 */
void
setup_log_capture (void)
{
  (void) g_log_set_default_handler (check_log_handler, NULL);
  (void) g_log_set_handler (G_LOG_DOMAIN,
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      check_log_handler, NULL);
  (void) g_log_set_handler ("buzztard",
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      check_log_handler, NULL);
  (void) g_log_set_handler ("GStreamer",
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      check_log_handler, NULL);
  (void) g_log_set_handler ("GLib",
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      check_log_handler, NULL);
  (void) g_log_set_handler ("GLib-GObject",
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      check_log_handler, NULL);
  (void) g_log_set_handler (NULL,
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      check_log_handler, NULL);
  (void) g_set_printerr_handler (check_print_handler);

#ifndef GST_DISABLE_GST_DEBUG
  gst_debug_add_log_function (check_gst_log_handler, NULL);
#endif
}


/*
 * file output tests:
 *
 * Check if certain text has been outputted to a (text)file. Pass either a
 * file-handle or a file_name.
 */

gboolean
check_file_contains_str (FILE * input_file, gchar * input_file_name,
    gchar * str)
{
  gchar read_str[1024];
  gboolean need_close = FALSE;
  gboolean ret = FALSE;

  g_assert (input_file || input_file_name);
  g_assert (str);

  if (!input_file) {
    input_file = fopen (input_file_name, "rb");
    need_close = TRUE;
  } else {
    fseek (input_file, 0, SEEK_SET);
  }
  if (!input_file) {
    return ret;
  }
  while (!feof (input_file)) {
    if (!fgets (read_str, 1023, input_file)) {
      break;
    }
    //GST_LOG("[%s]",read_str);
    read_str[1023] = '\0';
    if (strstr (read_str, str)) {
      ret = TRUE;
      break;
    }
  }
  if (need_close) {
    fclose (input_file);
  }
  return ret;
}


// runtime test selection via env-var BT_CHECKS
// injected via out tcase_add_test override
// we could do this also for suite_add_tcase and srunner_add_suite
// maybe by treating the env-var as <suite>:<tcase>:<test>
gboolean
_bt_check_run_test_func (const gchar * func_name)
{
  const gchar *checks;
  gboolean res = FALSE;
  gchar **funcs, **f;

  checks = g_getenv ("BT_CHECKS");

  /* no filter specified => run all checks */
  if (checks == NULL || *checks == '\0')
    return TRUE;

  /* only run specified functions, regexps would be nice */
  funcs = g_strsplit (checks, ",", -1);
  for (f = funcs; f != NULL && *f != NULL; ++f) {
    if (g_pattern_match_simple (*f, func_name)) {
      res = TRUE;
      break;
    }
  }
  g_strfreev (funcs);
  return res;
}

// main loop

static gboolean
_check_end_main_loop (gpointer user_data)
{
  g_main_loop_quit ((GMainLoop *) user_data);
  return FALSE;
}

void
check_run_main_loop_for_usec (gulong usec)
{
  GMainLoop *loop = g_main_loop_new (g_main_context_default (), FALSE);

  g_timeout_add_full (G_PRIORITY_HIGH, usec / 1000, _check_end_main_loop, loop,
      NULL);
  g_main_loop_run (loop);
}

// test file access

const gchar *
check_get_test_song_path (const gchar * name)
{
  static gchar path[2048];

  // TESTSONGDIR gets defined in Makefile.am and is an absolute path
  g_snprintf (path, 2047, TESTSONGDIR "" G_DIR_SEPARATOR_S "%s", name);
  GST_INFO ("build path: '%s'", path);
  return (path);
}

// test plugins

void
check_register_plugins (void)
{
  // this is a hack to persuade the linker to not optimize out these :(
  if (!bt_test_mono_source_get_type ())
    g_print ("registering mono-src faild");
}

// property tests

static gboolean
check_read_int_param (GParamSpec * paramspec, GObject * toCheck)
{
  gint check;
  gboolean ret = FALSE;

  g_object_get (toCheck, paramspec->name, &check, NULL);
  if ((check >= G_PARAM_SPEC_INT (paramspec)->minimum) &&
      (check <= G_PARAM_SPEC_INT (paramspec)->maximum)) {
    ret = TRUE;
  } else {
    GST_WARNING ("property read range check failed for : %s : %d <= %d <= %d",
        paramspec->name,
        G_PARAM_SPEC_INT (paramspec)->minimum,
        check, G_PARAM_SPEC_INT (paramspec)->maximum);
  }
  return ret;
}

static gboolean
check_read_uint_param (GParamSpec * paramspec, GObject * toCheck)
{
  guint check;
  gboolean ret = FALSE;

  g_object_get (toCheck, paramspec->name, &check, NULL);
  if ((check >= G_PARAM_SPEC_UINT (paramspec)->minimum) &&
      (check <= G_PARAM_SPEC_UINT (paramspec)->maximum)) {
    ret = TRUE;
  } else {
    GST_WARNING ("property read range check failed for : %s : %u <= %u <= %u",
        paramspec->name,
        G_PARAM_SPEC_UINT (paramspec)->minimum,
        check, G_PARAM_SPEC_UINT (paramspec)->maximum);
  }
  return ret;
}

static gboolean
check_read_int64_param (GParamSpec * paramspec, GObject * toCheck)
{
  gint64 check;
  gboolean ret = FALSE;

  g_object_get (toCheck, paramspec->name, &check, NULL);
  if ((check >= G_PARAM_SPEC_INT64 (paramspec)->minimum) &&
      (check <= G_PARAM_SPEC_INT64 (paramspec)->maximum)) {
    ret = TRUE;
  } else {
    GST_WARNING ("property read range check failed for : %s", paramspec->name);
  }
  return ret;
}

static gboolean
check_read_long_param (GParamSpec * paramspec, GObject * toCheck)
{
  glong check;
  gboolean ret = FALSE;

  g_object_get (toCheck, paramspec->name, &check, NULL);
  if ((check >= G_PARAM_SPEC_LONG (paramspec)->minimum) &&
      (check <= G_PARAM_SPEC_LONG (paramspec)->maximum)) {
    ret = TRUE;
  } else {
    GST_WARNING
        ("property read range check failed for : %s : %ld <= %ld <= %ld",
        paramspec->name, G_PARAM_SPEC_LONG (paramspec)->minimum, check,
        G_PARAM_SPEC_LONG (paramspec)->maximum);
  }
  return ret;
}

static gboolean
check_read_ulong_param (GParamSpec * paramspec, GObject * toCheck)
{
  gulong check;
  gboolean ret = FALSE;

  g_object_get (toCheck, paramspec->name, &check, NULL);
  if ((check >= G_PARAM_SPEC_ULONG (paramspec)->minimum) &&
      (check <= G_PARAM_SPEC_ULONG (paramspec)->maximum)) {
    ret = TRUE;
  } else {
    GST_WARNING
        ("property read range check failed for : %s : %lu <= %lu <= %lu",
        paramspec->name, G_PARAM_SPEC_ULONG (paramspec)->minimum, check,
        G_PARAM_SPEC_ULONG (paramspec)->maximum);
  }
  return ret;
}

static gboolean
check_read_property (GParamSpec * paramspec, GObject * toCheck)
{
  GType param_type;
  gboolean ret = FALSE;

  param_type = G_PARAM_SPEC_TYPE (paramspec);
  if (param_type == G_TYPE_PARAM_INT) {
    ret = check_read_int_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_UINT) {
    ret = check_read_uint_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_INT64) {
    ret = check_read_int64_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_LONG) {
    ret = check_read_long_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_ULONG) {
    ret = check_read_ulong_param (paramspec, toCheck);
  } else {                      // no check performed
    ret = TRUE;
  }
  return ret;
}

static gboolean
check_write_int_param (GParamSpec * paramspec, GObject * toCheck)
{
  gint check1, check2;

  check1 = G_PARAM_SPEC_INT (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, check1, NULL);

  check2 = G_PARAM_SPEC_INT (paramspec)->maximum;
  g_object_set (toCheck, paramspec->name, check2, NULL);

  return TRUE;
}

static gboolean
check_write_uint_param (GParamSpec * paramspec, GObject * toCheck)
{
  guint check1, check2;

  check1 = G_PARAM_SPEC_UINT (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, check1, NULL);

  check2 = G_PARAM_SPEC_UINT (paramspec)->maximum;
  g_object_set (toCheck, paramspec->name, check2, NULL);

  return TRUE;
}

static gboolean
check_write_int64_param (GParamSpec * paramspec, GObject * toCheck)
{
  gint64 check1, check2;

  check1 = G_PARAM_SPEC_INT64 (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, check1, NULL);

  check2 = G_PARAM_SPEC_INT64 (paramspec)->maximum;
  g_object_set (toCheck, paramspec->name, check2, NULL);

  return TRUE;
}

static gboolean
check_write_long_param (GParamSpec * paramspec, GObject * toCheck)
{
  glong check1, check2;

  check1 = G_PARAM_SPEC_LONG (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, check1, NULL);

  check2 = G_PARAM_SPEC_LONG (paramspec)->maximum;
  g_object_set (toCheck, paramspec->name, check2, NULL);

  return TRUE;
}

static gboolean
check_write_ulong_param (GParamSpec * paramspec, GObject * toCheck)
{
  gulong check1, check2;

  check1 = G_PARAM_SPEC_ULONG (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, check1, NULL);

  check2 = G_PARAM_SPEC_ULONG (paramspec)->maximum;
  g_object_set (toCheck, paramspec->name, check2, NULL);

  return TRUE;
}

static gboolean
check_write_property (GParamSpec * paramspec, GObject * toCheck)
{
  GType param_type;
  gboolean ret = FALSE;

  param_type = G_PARAM_SPEC_TYPE (paramspec);
  if (param_type == G_TYPE_PARAM_INT) {
    ret = check_write_int_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_UINT) {
    ret = check_write_uint_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_INT64) {
    ret = check_write_int64_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_LONG) {
    ret = check_write_long_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_ULONG) {
    ret = check_write_ulong_param (paramspec, toCheck);
  } else {                      // no check performed
    ret = TRUE;
  }
  return ret;
}

static gboolean
check_readwrite_int_param (GParamSpec * paramspec, GObject * toCheck)
{
  gint ival, oval;
  gboolean ret = TRUE;

  ival = G_PARAM_SPEC_INT (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, ival, NULL);
  g_object_get (toCheck, paramspec->name, &oval, NULL);
  if (ival != oval) {
    GST_WARNING ("property read/write minimum check failed for : %s : %d != %d",
        paramspec->name, ival, oval);
    ret = FALSE;
  }
  /*
     ival=G_PARAM_SPEC_INT(paramspec)->maximum;
     g_object_set(toCheck,paramspec->name, ival,NULL);
     g_object_get(toCheck,paramspec->name,&oval,NULL);
     if(ival!=oval) {
     GST_WARNING("property read/write maximum check failed for : %s : %d != %d",
     paramspec->name,
     ival,oval
     );
     ret=FALSE;
     }
   */
  return ret;
}

static gboolean
check_readwrite_uint_param (GParamSpec * paramspec, GObject * toCheck)
{
  guint ival, oval;
  gboolean ret = TRUE;

  ival = G_PARAM_SPEC_UINT (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, ival, NULL);
  g_object_get (toCheck, paramspec->name, &oval, NULL);
  if (ival != oval) {
    GST_WARNING ("property read/write minimum check failed for : %s : %u != %u",
        paramspec->name, ival, oval);
    ret = FALSE;
  }
  /*
     ival=G_PARAM_SPEC_UINT(paramspec)->maximum;
     g_object_set(toCheck,paramspec->name, ival,NULL);
     g_object_get(toCheck,paramspec->name,&oval,NULL);
     if(ival!=oval) {
     GST_WARNING("property read/write maximum check failed for : %s : %u != %u",
     paramspec->name,
     ival,oval
     );
     ret=FALSE;
     }
   */
  return ret;
}

static gboolean
check_readwrite_int64_param (GParamSpec * paramspec, GObject * toCheck)
{
  gint64 ival, oval;
  gboolean ret = TRUE;

  ival = G_PARAM_SPEC_INT64 (paramspec)->minimum;
  g_object_set (toCheck, paramspec->name, ival, NULL);
  g_object_get (toCheck, paramspec->name, &oval, NULL);
  if (ival != oval) {
    GST_WARNING ("property read/write minimum check failed for : %s",
        paramspec->name);
    ret = FALSE;
  }
  /*
     ival=G_PARAM_SPEC_INT64(paramspec)->maximum;
     g_object_set(toCheck,paramspec->name, ival,NULL);
     g_object_get(toCheck,paramspec->name,&oval,NULL);
     if(ival!=oval) {
     GST_WARNING("property read/write maxmum check failed for : %s",paramspec->name);
     ret=FALSE;
     }
   */
  return ret;
}

static gboolean
check_readwrite_long_param (GParamSpec * paramspec, GObject * toCheck)
{
  glong ival, oval;
  gboolean ret = TRUE;

  ival = G_PARAM_SPEC_LONG (paramspec)->default_value;
  g_object_set (toCheck, paramspec->name, ival, NULL);
  g_object_get (toCheck, paramspec->name, &oval, NULL);
  if (ival != oval) {
    GST_WARNING
        ("property read/write default_value check failed for : %s : %ld != %ld",
        paramspec->name, ival, oval);
    ret = FALSE;
  }
  /*
     ival=G_PARAM_SPEC_LONG(paramspec)->maximum;
     g_object_set(toCheck,paramspec->name, ival,NULL);
     g_object_get(toCheck,paramspec->name,&oval,NULL);
     if(ival!=oval) {
     GST_WARNING("property read/write maxmum check failed for : %s : %ld != %ld",
     paramspec->name,
     ival,oval
     );
     ret=FALSE;
     }
   */
  return ret;
}

static gboolean
check_readwrite_ulong_param (GParamSpec * paramspec, GObject * toCheck)
{
  gulong ival, oval;
  gboolean ret = TRUE;

  ival = G_PARAM_SPEC_ULONG (paramspec)->default_value;
  g_object_set (toCheck, paramspec->name, ival, NULL);
  g_object_get (toCheck, paramspec->name, &oval, NULL);
  if (ival != oval) {
    GST_WARNING
        ("property read/write default_value check failed for : %s : %lu != %lu",
        paramspec->name, ival, oval);
    ret = FALSE;
  }
  /*
     ival=G_PARAM_SPEC_ULONG(paramspec)->maximum;
     g_object_set(toCheck,paramspec->name, ival,NULL);
     g_object_get(toCheck,paramspec->name,&oval,NULL);
     if(ival!=oval) {
     GST_WARNING("property read/write maxmum check failed for : %s : %lu != %lu",
     paramspec->name,
     ival,oval
     );
     ret=FALSE;
     }
   */
  return ret;
}

static gboolean
check_readwrite_property (GParamSpec * paramspec, GObject * toCheck)
{
  GType param_type;
  gboolean ret = FALSE;

  param_type = G_PARAM_SPEC_TYPE (paramspec);
  if (param_type == G_TYPE_PARAM_INT) {
    ret = check_readwrite_int_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_UINT) {
    ret = check_readwrite_uint_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_INT64) {
    ret = check_readwrite_int64_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_LONG) {
    ret = check_readwrite_long_param (paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_ULONG) {
    ret = check_readwrite_ulong_param (paramspec, toCheck);
  } else {                      // no check performed
    ret = TRUE;
  }
  return ret;
}

/* test gobject properties
 * @toCheck: the object to examine
 *
 * The function runs tests against all registered properties of the given
 * object. Depending on the flags and the type of the properties, tests include
 * read/write and boundary checks.
 *
 * TODO(ensonic):
 * - tests limmits for integer/float props
 * - write twice to gobject props (to check if they unref the old one)
 * - test more type
 * - use test more often :)
 */

gboolean
check_gobject_properties (GObject * toCheck)
{
  gboolean ret = FALSE;
  gboolean check_read = TRUE;
  gboolean check_write = TRUE;
  gboolean check_readwrite = TRUE;
  guint n_properties;
  guint loop;
  GParamSpec **return_params;

  return_params =
      g_object_class_list_properties (G_OBJECT_GET_CLASS (toCheck),
      &n_properties);
  // iterate over properties
  for (loop = 0; loop < n_properties; loop++) {
    GParamSpec *paramspec = return_params[loop];

    GST_DEBUG ("property check for : %s", paramspec->name);
    if (paramspec->flags & G_PARAM_READABLE) {
      if (!(check_read = check_read_property (paramspec, toCheck))) {
        GST_WARNING ("property read check failed for : %s", paramspec->name);
      }
    }
    if (paramspec->flags & G_PARAM_WRITABLE) {
      if (!(check_write = check_write_property (paramspec, toCheck))) {
        GST_WARNING ("property write check failed for : %s", paramspec->name);
      }
    }
    if (paramspec->flags & G_PARAM_READWRITE) {
      if (!(check_readwrite = check_readwrite_property (paramspec, toCheck))) {
        GST_WARNING ("property read/write check failed for : %s",
            paramspec->name);
      }
    }
  }
  g_free (return_params);
  if (check_read && check_write && check_readwrite) {
    ret = TRUE;
  }
  return ret;
}

/* helpers to get single gobject properties
 * allows to write 
 *   fail_unless(check_get_gulong_property(obj,"voice")==1, NULL);
 * instead of
 *   gulong voices;
 *   g_object_get(pattern,"voices",&voices,NULL);
 *   fail_unless(voices==1, NULL);
 */
gboolean
check_gobject_get_boolean_property (gpointer obj, const gchar * prop)
{
  gboolean val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

gint
check_gobject_get_int_property (gpointer obj, const gchar * prop)
{
  gint val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

guint
check_gobject_get_uint_property (gpointer obj, const gchar * prop)
{
  guint val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

glong
check_gobject_get_long_property (gpointer obj, const gchar * prop)
{
  glong val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

gulong
check_gobject_get_ulong_property (gpointer obj, const gchar * prop)
{
  gulong val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

guint64
check_gobject_get_uint64_property (gpointer obj, const gchar * prop)
{
  guint64 val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

GObject *
check_gobject_get_object_property (gpointer obj, const gchar * prop)
{
  GObject *val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

gchar *
check_gobject_get_str_property (gpointer obj, const gchar * prop)
{
  gchar *val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}

gpointer
check_gobject_get_ptr_property (gpointer obj, const gchar * prop)
{
  gpointer val;

  g_object_get (obj, prop, &val, NULL);
  return val;
}
