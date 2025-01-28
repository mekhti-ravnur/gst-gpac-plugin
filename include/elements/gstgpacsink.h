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

#pragma once

#include "gpacmessages.h"
#include "lib/caps.h"
#include "lib/properties.h"

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_GPAC_SINK (gst_gpac_sink_get_type())
G_DECLARE_FINAL_TYPE(GstGpacSink, gst_gpac_sink, GST, GPAC_SINK, GstBin)

/**
 * GstGpacSink: Opaque data structure.
 */
struct _GstGpacSink
{
  GstBin parent;

  /* Internal elements */
  GstElement* tf;
  GstElement* sink;
};

GST_ELEMENT_REGISTER_DECLARE(gpac_sink);

G_END_DECLS
