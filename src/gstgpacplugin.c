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

#include "config.h"

#include "elements/gstgpacsink.h"
#include "elements/gstgpactf.h"

static gboolean
plugin_init(GstPlugin* plugin)
{
  gboolean ret = TRUE;
  ret |= GST_ELEMENT_REGISTER(gpac_tf, plugin);
  ret |= GST_ELEMENT_REGISTER(gpac_sink, plugin);
  return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MIN_REQUIRED_MAJOR,
                  GST_VERSION_MIN_REQUIRED_MINOR,
                  gpac_plugin,
                  PACKAGE_DESC,
                  plugin_init,
                  VERSION,
                  LICENSE,
                  PACKAGE_NAME,
                  PACKAGE_ORIGIN)
