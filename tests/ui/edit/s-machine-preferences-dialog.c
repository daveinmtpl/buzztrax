/* Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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

#include "m-bt-edit.h"

//extern TCase *bt_machine_preferences_dialog_test_case(void);
extern TCase *bt_machine_preferences_dialog_example_case (void);

Suite *
bt_machine_preferences_dialog_suite (void)
{
  Suite *s = suite_create ("BtMachinePreferencesDialog");

  //suite_add_tcase(s,bt_machine_preferences_dialog_test_case());
  suite_add_tcase (s, bt_machine_preferences_dialog_example_case ());
  return (s);
}
