/* $Id: wire.c,v 1.27 2004-09-24 22:42:15 ensonic Exp $
 * class for a machine to machine connection
 */
 
#define BT_CORE
#define BT_WIRE_C

#include <libbtcore/core.h>

enum {
  WIRE_SONG=1,
	WIRE_SRC,
	WIRE_DST
};

struct _BtWirePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the wire belongs to */
	BtSong *song;
	/* which machines are linked */
	BtMachine *src,*dst;
	/* wire type adapter elements */
	GstElement *convert,*scale;
	/* convinience pointer */
	GstElement *dst_elem,*src_elem;
  
  /* the element to control the gain of a connection */
  GstElement *gain;
  
  /* the element to analyse the output level of the wire (Filter/Analyzer/Audio/Level)
   * \@todo which volume does one wants to know about? (see machine.c)
   *   a.) the machine output (before the volume control on the wire)
   *   b.) the machine input (after the volume control on the wire)
   */
  GstElement *level;
  /* the element to analyse the frequence spectrum of the wire (Filter/Analyzer/Audio/Spectrum) */
  GstElement *spectrum;
  
};

//-- helper methods

/**
 * bt_wire_link_machines:
 * @self: the wire that should be used for this connection
 *
 * Links the gst-element of this wire and tries a couple for conversion elements
 * if necessary.
 *
 * Returns: true for success
 */
static gboolean bt_wire_link_machines(const BtWire *self) {
  gboolean res=TRUE;
  BtMachine *src, *dst;
  BtSong *song=self->private->song;
  GstBin *bin;

  g_object_get(G_OBJECT(song),"bin",&bin,NULL);
  src=self->private->src;
  dst=self->private->dst;

	GST_DEBUG("trying to link machines directly : %p -> %p",src->src_elem,dst->dst_elem);
	// try link src to dst {directly, with convert, with scale, with ...}
	if(!gst_element_link(src->src_elem, dst->dst_elem)) {
		if(!self->private->convert) {
			self->private->convert=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",self));
			g_assert(self->private->convert!=NULL);
		}
		gst_bin_add(bin, self->private->convert);
    GST_DEBUG("trying to link machines with convert");
		if(!gst_element_link_many(src->src_elem, self->private->convert, dst->dst_elem, NULL)) {
			if(!self->private->scale) {
				self->private->scale=gst_element_factory_make("audioscale",g_strdup_printf("audioscale_%p",self));
				g_assert(self->private->scale!=NULL);
			}
			gst_bin_add(bin, self->private->scale);
      GST_DEBUG("trying to link machines with scale");
			if(!gst_element_link_many(src->src_elem, self->private->scale, dst->dst_elem, NULL)) {
        GST_DEBUG("trying to link machines with convert and scale");
				if(!gst_element_link_many(src->src_elem, self->private->convert, self->private->scale, dst->dst_elem, NULL)) {
					// try harder (scale, convert)
					GST_DEBUG("failed to link the machines");
          res=FALSE;
				}
				else {
					self->private->src_elem=self->private->scale;
					self->private->dst_elem=self->private->convert;
					GST_DEBUG("  wire okay with convert and scale");
				}
			}
			else {
				self->private->src_elem=self->private->scale;
				self->private->dst_elem=self->private->scale;
        GST_DEBUG("  wire okay with scale");
			}
		}
		else {
			self->private->src_elem=self->private->convert;
			self->private->dst_elem=self->private->convert;
			GST_DEBUG("  wire okay with convert");
		}
	}
	else {
		self->private->src_elem=src->src_elem;
		self->private->dst_elem=dst->dst_elem;
		GST_DEBUG("  wire okay");
	}
  g_object_try_unref(bin);
	return(res);
}

/**
 * bt_wire_unlink_machines:
 * @self: the wire that should be used for this connection
 *
 * Unlinks gst-element of this wire and removes the conversion elemnts from the
 * song.
 *
 * Returns: true for success
 */
static gboolean bt_wire_unlink_machines(const BtWire *self) {
  BtSong *song=self->private->song;
  GstBin *bin;

	GST_DEBUG("trying to unlink machines");
	gst_element_unlink(self->private->src->src_elem, self->private->dst->dst_elem);
	if(self->private->convert) {
		gst_element_unlink_many(self->private->src->src_elem, self->private->convert, self->private->dst->dst_elem, NULL);
	}
	if(self->private->scale) {
		gst_element_unlink_many(self->private->src->src_elem, self->private->scale, self->private->dst->dst_elem, NULL);
	}
	if(self->private->convert && self->private->scale) {
		gst_element_unlink_many(self->private->src->src_elem, self->private->convert, self->private->scale, self->private->dst->dst_elem, NULL);
	}
  g_object_get(G_OBJECT(song),"bin",&bin,NULL);
	if(self->private->convert) {
		gst_bin_remove(bin, self->private->convert);
	}
	if(self->private->scale) {
		gst_bin_remove(bin, self->private->scale);
	}
  g_object_try_unref(bin);
}

/**
 * bt_wire_connect:
 * @self: the wire that should be used for this connection
 *
 * Connect two machines with a wire. The source and dsetination machine must be
 * passed upon construction. 
 * Each machine can be involved in multiple connections. The method
 * transparently add spreader or adder elements in such cases. It further
 * inserts elemnts for data-type conversion if neccesary.
 *
 * The machines must have been previously added to the setup using #bt_setup_add_machine().
 * Same way the resulting wire should be added to the setup using #bt_setup_add_wire().
 *
 * Returns: true for success
 */
static gboolean bt_wire_connect(BtWire *self) {
  gboolean res=FALSE;
  BtSong *song=self->private->song;
  BtSetup *setup=NULL;
  GstBin *bin=NULL;
  BtWire *other_wire;
  BtMachine *src, *dst;

  if((!self->private->src) || (!self->private->dst)) goto Error;
  g_object_get(G_OBJECT(song),"bin",&bin,"setup",&setup,NULL);
  src=self->private->src;
  dst=self->private->dst;

	GST_DEBUG("trying to link machines : %p (%p) -> %p (%p)",src,src->src_elem,dst,dst->dst_elem);

	// if there is already a connection from src && src has not yet an spreader (in use)
	if((other_wire=bt_setup_get_wire_by_src_machine(setup,src)) && (src->src_elem!=src->spreader)) {
		GST_DEBUG("  other wire from src found");
		// unlink the elements from the other wire
		bt_wire_unlink_machines(other_wire);
		// create spreader (if needed)
		if(!src->spreader) {
			src->spreader=gst_element_factory_make("oneton",g_strdup_printf("oneton_%p",src));
			g_assert(src->spreader!=NULL);
			gst_bin_add(bin, src->spreader);
			if(!gst_element_link(src->machine, src->spreader)) {
				GST_ERROR("failed to link the machines internal spreader");goto Error;
			}
		}
		if(other_wire->private->src_elem==src->src_elem) {
			// we need to fix the src_elem of the other connect, as we have inserted the spreader
			other_wire->private->src_elem=src->spreader;
		}
		// activate spreader
		src->src_elem=src->spreader;
		GST_DEBUG("  spreader activated for \"%s\"",gst_element_get_name(src->machine));
		// correct the link for the other wire
		if(!bt_wire_link_machines(other_wire)) {
		//if(!gst_element_link(other_wire->src->src_elem, other_wire->dst_elem)) {
			GST_ERROR("failed to re-link the machines after inserting internal spreaker");goto Error;
		}
	}
	
	// if there is already a wire to dst and dst has not yet an adder (in use)
	if((other_wire=bt_setup_get_wire_by_dst_machine(setup,dst)) && (dst->dst_elem!=dst->adder)) {
		GST_DEBUG("  other wire to dst found");
		// unlink the elements from the other wire
		bt_wire_unlink_machines(other_wire);
		//gst_element_unlink(other_wire->src_elem, other_wire->dst->dst_elem);
		// create adder (if needed)
		if(!dst->adder) {
			GstElement *convert;
			
			dst->adder=gst_element_factory_make("adder",g_strdup_printf("adder_%p",dst));
			g_assert(dst->adder!=NULL);
			gst_bin_add(bin, dst->adder);
			// @todo this is a workaround for gst not making full caps-nego
			convert=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",dst));
			g_assert(convert!=NULL);
			gst_bin_add(bin, convert);
			if(!gst_element_link_many(dst->adder, convert, dst->machine, NULL)) {
				GST_ERROR("failed to link the machines internal adder");goto Error;
			}
		}
		if(other_wire->private->dst_elem==dst->dst_elem) {
			// we need to fix the dst_elem of the other connect, as we have inserted the adder
			other_wire->private->dst_elem=dst->adder;
		}
		// activate adder
		dst->dst_elem=dst->adder;
		GST_DEBUG("  adder activated for \"%s\"",gst_element_get_name(dst->machine)); 
		// correct the link for the other wire
		if(!bt_wire_link_machines(other_wire)) {
		//if(!gst_element_link(other_wire->src_elem, other_wire->dst->dst_elem)) {
			GST_ERROR("failed to re-link the machines after inserting internal adder");goto Error;
		}
	}
	
	if(!bt_wire_link_machines(self)) {
    GST_ERROR("linking machines failed");goto Error;
	}
  bt_setup_add_wire(setup,self);
  res=TRUE;
  GST_DEBUG("linking machines succeded");
Error:
  g_object_try_unref(bin);
  g_object_try_unref(setup);
  if(!res) g_object_unref(self);
	return(res);
}

//-- constructor methods

/**
 * bt_wire_new:
 * @song: the song the new instance belongs to
 * @src_machine: the data source
 * @dst_machine: the data sink
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine) {
  BtWire *self;
  self=BT_WIRE(g_object_new(BT_TYPE_WIRE,"song",song,"src",src_machine,"dst",dst_machine,NULL));

  bt_wire_connect(self);
  return(self);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wire_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWire *self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      g_value_set_object(value, self->private->song);
    } break;
    case WIRE_SRC: {
      g_value_set_object(value, self->private->src);
    } break;
    case WIRE_DST: {
      g_value_set_object(value, self->private->dst);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wire_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWire *self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      g_object_try_weak_unref(self->private->song);
      self->private->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->private->song);
      //GST_DEBUG("set the song for wire: %p",self->private->song);
    } break;
		case WIRE_SRC: {
      g_object_try_unref(self->private->src);
			self->private->src = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the source element for the wire: %p",self->private->src);
		} break;
		case WIRE_DST: {
      g_object_try_unref(self->private->dst);
			self->private->dst = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the target element for the wire: %p",self->private->dst);
		} break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_dispose(GObject *object) {
  BtWire *self = BT_WIRE(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

	g_object_try_weak_unref(self->private->song);
	g_object_try_unref(self->private->dst);
	g_object_try_unref(self->private->src);
}

static void bt_wire_finalize(GObject *object) {
  BtWire *self = BT_WIRE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->private);
}

static void bt_wire_init(GTypeInstance *instance, gpointer g_class) {
  BtWire *self = BT_WIRE(instance);
  self->private = g_new0(BtWirePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_wire_class_init(BtWireClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  gobject_class->set_property = bt_wire_set_property;
  gobject_class->get_property = bt_wire_get_property;
  gobject_class->dispose      = bt_wire_dispose;
  gobject_class->finalize     = bt_wire_finalize;

  g_object_class_install_property(gobject_class,WIRE_SONG,
																	g_param_spec_object("song",
                                     "song construct prop",
                                     "the song object, the wire belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,WIRE_SRC,
																	g_param_spec_object("src",
                                     "src ro prop",
                                     "src machine object, the wire links to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,WIRE_DST,
																	g_param_spec_object("dst",
                                     "dst ro prop",
                                     "dst machine object, the wire links to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
}

GType bt_wire_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtWireClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtWire),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wire_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtWire",&info,0);
  }
  return type;
}

