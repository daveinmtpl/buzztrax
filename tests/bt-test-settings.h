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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_TEST_SETTINGS_H
#define BT_TEST_SETTINGS_H

#include <glib.h>
#include <glib-object.h>
#include"settings-private.h"

#define BT_TYPE_TEST_SETTINGS            (bt_test_settings_get_type ())
#define BT_TEST_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_SETTINGS, BtTestSettings))
#define BT_TEST_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_SETTINGS, BtTestSettingsClass))
#define BT_IS_TEST_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_SETTINGS))
#define BT_IS_TEST_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_SETTINGS))
#define BT_TEST_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_SETTINGS, BtTestSettingsClass))

/* type macros */

typedef struct _BtTestSettings BtTestSettings;
typedef struct _BtTestSettingsClass BtTestSettingsClass;
typedef struct _BtTestSettingsPrivate BtTestSettingsPrivate;

/**
 * BtTestSettings:
 *
 * gconf based implementation object for a buzztard based settings
 */
struct _BtTestSettings {
  const BtSettings parent;
  
  /*< private >*/
  BtTestSettingsPrivate *priv;
};
/* structure of the gconf-settings class */
struct _BtTestSettingsClass {
  const BtSettingsClass parent;
  
};

BtTestSettings *bt_test_settings_new(void);

extern void bt_test_settings_set(BtTestSettings * const self, gchar *property_name, gpointer value);

GType bt_test_settings_get_type(void) G_GNUC_CONST;

#endif // BT_TEST_SETTINGS_H
