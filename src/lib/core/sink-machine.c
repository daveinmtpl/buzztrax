// $Id: sink-machine.c,v 1.63 2006-03-10 17:18:27 ensonic Exp $
/**
 * SECTION:btsinkmachine
 * @short_description: class for signal processing machines with inputs only
 *
 * Sinks are machines that do playback or recording of the song. The
 * sink-machine utilizes the #BtSinkBin to transparently switch elements between
 * record (encoding) and playback.
 */ 
 
#define BT_CORE
#define BT_SINK_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/sink-machine.h>
#include <libbtcore/machine-private.h>

struct _BtSinkMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- helper methods

//-- constructor methods

/**
 * bt_sink_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 *
 * Create a new instance.
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 * The plugin used for this machine is taken from the #BtSettings.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkMachine *bt_sink_machine_new(const BtSong *song, const gchar *id) {
  BtSinkMachine *self=NULL;

  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);

  if(!(self=BT_SINK_MACHINE(g_object_new(BT_TYPE_SINK_MACHINE,"song",song,"id",id,"plugin-name","bt-sink-bin",NULL)))) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }

  GST_DEBUG("  %s this will be the master for the song",id);
  g_object_set(G_OBJECT(song),"master",G_OBJECT(self),NULL);

  //g_object_get(G_OBJECT (self), "machine", &elem, NULL);
  //g_object_set(G_OBJECT (elem), "sync", FALSE, NULL);
  //gst_object_unref(GST_OBJECT(elem));
  
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- io interface

static xmlNodePtr bt_sink_machine_persistence_save(BtPersistence *persistence, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
  //BtSinkMachine *self = BT_SINK_MACHINE(persistence);
  BtPersistenceInterface *parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlNodePtr node=NULL;

  GST_DEBUG("PERSISTENCE::sink-machine");

  // save parent class stuff
  if((node=parent_iface->save(persistence,doc,parent_node,selection))) {
    /* @todo: save own stuff */
    xmlNewProp(node,XML_CHAR_PTR("type"),XML_CHAR_PTR("sink"));
  }
  return(node);
}

static gboolean bt_sink_machine_persistence_load(BtPersistence *persistence, xmlDocPtr doc, xmlNodePtr node, BtPersistenceLocation *location) {
  BtSinkMachine *self = BT_SINK_MACHINE(persistence);
  BtPersistenceInterface *parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  
  g_object_set(G_OBJECT(self),"plugin-name","bt-sink-bin",NULL);
  
  // load parent class stuff
  return(parent_iface->load(persistence,doc,node,location));
}

static void bt_sink_machine_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface *iface = g_iface;
  
  iface->load = bt_sink_machine_persistence_load;
  iface->save = bt_sink_machine_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_sink_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSinkMachine *self = BT_SINK_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_sink_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSinkMachine *self = BT_SINK_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sink_machine_dispose(GObject *object) {
  BtSinkMachine *self = BT_SINK_MACHINE(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_sink_machine_finalize(GObject *object) {
  BtSinkMachine *self = BT_SINK_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sink_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtSinkMachine *self = BT_SINK_MACHINE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SINK_MACHINE, BtSinkMachinePrivate);
}

static void bt_sink_machine_class_init(BtSinkMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_MACHINE);
  g_type_class_add_private(klass,sizeof(BtSinkMachinePrivate));
  
  gobject_class->set_property = bt_sink_machine_set_property;
  gobject_class->get_property = bt_sink_machine_get_property;
  gobject_class->dispose      = bt_sink_machine_dispose;
  gobject_class->finalize     = bt_sink_machine_finalize;
}

GType bt_sink_machine_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSinkMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sink_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSinkMachine),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_sink_machine_init, // instance_init
      NULL // value_table
    };
    static const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_sink_machine_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(BT_TYPE_MACHINE,"BtSinkMachine",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
