/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-core.h"

//-- globals

static BtApplication *app;
static BtSong *song;

//-- fixtures

static void
case_setup (void)
{
  GST_INFO
      ("================================================================================");
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  song = bt_song_new (app);
}

static void
test_teardown (void)
{
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

BT_START_TEST (test_bt_cmd_pattern_properties)
{
  /* arrange */
  BtMachine *machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  BtCmdPattern *pattern =
      bt_cmd_pattern_new (song, machine, BT_PATTERN_CMD_MUTE);

  /* act & assert */
  fail_unless (check_gobject_properties ((GObject *) pattern), NULL);

  /* cleanup */
  g_object_unref (pattern);
  g_object_unref (machine);
}

BT_END_TEST
BT_START_TEST (test_bt_cmd_pattern_new_null_machine)
{
  /* arrange */
  check_init_error_trapp ("bt_cmd_pattern_",
      "BT_IS_MACHINE (self->priv->machine)");

  /* act */
  BtCmdPattern *pattern = bt_cmd_pattern_new (song, NULL, BT_PATTERN_CMD_MUTE);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (pattern != NULL, NULL);

  /* cleanup */
  g_object_unref (pattern);
}

BT_END_TEST TCase * bt_cmd_pattern_test_case (void)
{
  TCase *tc = tcase_create ("BtCmdPatternTests");

  tcase_add_test (tc, test_bt_cmd_pattern_properties);
  tcase_add_test (tc, test_bt_cmd_pattern_new_null_machine);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
