/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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

//extern TCase *bt_main_page_sequence_test_case(void);
extern TCase *bt_main_page_sequence_example_case (void);

Suite *
bt_main_page_sequence_suite (void)
{
  Suite *s = suite_create ("BtMainPageSequence");

  //suite_add_tcase(s,bt_main_page_sequence_test_case());
  suite_add_tcase (s, bt_main_page_sequence_example_case ());
  return (s);
}
