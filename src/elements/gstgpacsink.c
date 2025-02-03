/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Authors: Deniz Ugur, Romain Bouqueau, Sohaib Larbi
 *			Copyright (c) Motion Spell
 *				All rights reserved
 *
 *  This file is part of the GPAC/GStreamer wrapper
 *
 *  This GPAC/GStreamer wrapper is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Affero General Public License
 *  as published by the Free Software Foundation; either version 3, or (at
 *  your option) any later version.
 *
 *  This GPAC/GStreamer wrapper is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public
 *  License along with this library; see the file LICENSE.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "elements/gstgpacsink.h"

// Define the element properties
#define gst_gpac_sink_parent_class parent_class
G_DEFINE_TYPE(GstGpacSink, gst_gpac_sink, GST_TYPE_BIN);
GST_ELEMENT_REGISTER_DEFINE(gpac_sink,
                            "gpacsink",
                            GST_RANK_PRIMARY,
                            GST_TYPE_GPAC_SINK);

// #MARK: Properties
static void
gst_gpac_sink_set_property(GObject* object,
                           guint prop_id,
                           const GValue* value,
                           GParamSpec* pspec)
{
  GstGpacSink* gpac_sink = GST_GPAC_SINK(object);
  g_return_if_fail(GST_IS_GPAC_SINK(object));
  // Relay the property to the internal transform element
  g_object_set_property(G_OBJECT(gpac_sink->tf), pspec->name, value);
}

static void
gst_gpac_sink_get_property(GObject* object,
                           guint prop_id,
                           GValue* value,
                           GParamSpec* pspec)
{
  GstGpacSink* gpac_sink = GST_GPAC_SINK(object);
  g_return_if_fail(GST_IS_GPAC_SINK(object));
  // Relay the property to the internal transform element
  g_object_get_property(G_OBJECT(gpac_sink->tf), pspec->name, value);
}

// #MARK: Lifecycle
static void
gst_gpac_sink_reset(GstGpacSink* sink)
{
  // Remove the internal elements
  if (sink->tf) {
    gst_bin_remove(GST_BIN(sink), sink->tf);
    sink->tf = NULL;
  }

  if (sink->sink) {
    gst_bin_remove(GST_BIN(sink), sink->sink);
    sink->sink = NULL;
  }
}

// #MARK: Pad Management
static GstPad*
gst_gpac_sink_request_new_pad(GstElement* element,
                              GstPadTemplate* templ,
                              const gchar* pad_name,
                              const GstCaps* caps)
{
  GstGpacSink* gpac_sink = GST_GPAC_SINK(element);
  GstPad* pad = NULL;
  GstPad* peer = NULL;

  // Request a new pad from the aggregator
  peer = gst_element_request_pad(gpac_sink->tf, templ, pad_name, caps);
  if (!peer) {
    GST_ELEMENT_ERROR(gpac_sink,
                      STREAM,
                      FAILED,
                      ("Unable to request pad name from the aggregator"),
                      (NULL));
    return NULL;
  }

  // Decide the ghost pad name
  gchar* actual_name = gst_pad_get_name(peer);
  gchar* ghost_name = g_strdup_printf("ghost_%s", actual_name);
  g_free(actual_name);

  // Create a ghost pad from the aggregator
  pad = gst_ghost_pad_new_from_template(ghost_name, peer, templ);
  gst_pad_set_active(pad, TRUE);
  gst_element_add_pad(element, pad);
  gst_object_unref(peer);

  return pad;
}

static void
gst_gpac_sink_release_pad(GstElement* element, GstPad* pad)
{
  GstGpacSink* gpac_sink = GST_GPAC_SINK(element);
  GstPad* peer = gst_pad_get_peer(pad);

  if (peer) {
    gst_element_release_request_pad(gpac_sink->tf, peer);
    gst_object_unref(peer);
  }

  gst_object_ref(pad);
  gst_element_remove_pad(element, pad);
  gst_pad_set_active(pad, FALSE);
  gst_object_unref(pad);
}

// #MARK: Initialization
static void
gst_gpac_sink_init(GstGpacSink* sink)
{
  // Reset the sink
  gst_gpac_sink_reset(sink);

  // Treat the bin as a sink
  GST_OBJECT_FLAG_SET(sink, GST_ELEMENT_FLAG_SINK);

  // Create the internal elements
  sink->tf = gst_element_factory_make_full("gpactf", "no-output", TRUE, NULL);
  sink->sink = gst_element_factory_make("fakesink", NULL);

  // Add and link the elements
  gst_bin_add_many(GST_BIN(sink), sink->tf, sink->sink, NULL);
  gst_element_link(sink->tf, sink->sink);
}

static void
gst_gpac_sink_class_init(GstGpacSinkClass* klass)
{
  // Initialize the class
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass* gstelement_class = GST_ELEMENT_CLASS(klass);
  GstBinClass* gstbin_class = GST_BIN_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);

  // Install the pad templates
  gpac_install_sink_pad_templates(gstelement_class);

  // Set the pad management functions
  gstelement_class->request_new_pad =
    GST_DEBUG_FUNCPTR(gst_gpac_sink_request_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR(gst_gpac_sink_release_pad);

  // Set the property handlers
  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_gpac_sink_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_gpac_sink_get_property);

  // Configure the static properties
  gpac_install_local_properties(
    gobject_class, GPAC_PROP_GRAPH, GPAC_PROP_PRINT_STATS, GPAC_PROP_0);
  gpac_install_global_properties(gobject_class);

  // Set the metadata
  gst_element_class_set_static_metadata(
    gstelement_class,
    "gpac sink",
    "Aggregator/Sink",
    "Uses gpac filter session aggregate and process the incoming data",
    "Deniz Ugur <deniz.ugur@motionspell.com>");
}
