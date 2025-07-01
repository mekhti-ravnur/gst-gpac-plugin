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

#include "lib/caps.h"
#include "lib/properties.h"
#include "lib/signals.h"
#include "utils.h"

#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * Custom options to expose for the GPAC filters
 */
#define GPAC_TF_FILTER_OPTIONS(name, ...)                           \
  { .filter_name = name, .options = (guint32[]){ __VA_ARGS__, 0 } }

typedef struct
{
  const gchar* filter_name;
  guint32* options;
} filter_option_overrides;

static filter_option_overrides filter_options[] = {
  GPAC_TF_FILTER_OPTIONS("mp4mx", GPAC_PROP_SEGDUR),
};

/**
 * Subelements
 */
typedef struct
{
  const gchar* name;
  const gchar* value;
  // If not forced, user can override this option
  gboolean forced;
} filter_option;

typedef enum
{
  // Sink only subelement
  GPAC_SE_SINK_ONLY = 1 << 0,
  // Subelement requires memory output. Normally filters on sink don't require
  // one.
  GPAC_SE_REQUIRES_MEMOUT = 1 << 1,
} subelement_flags;

#define GPAC_SE_IS_REQUIRES_MEMOUT(flags)                          \
  (((flags) & GPAC_SE_REQUIRES_MEMOUT) == GPAC_SE_REQUIRES_MEMOUT)

typedef struct
{
  const gchar* alias_name;
  const gchar* filter_name;
  subelement_flags flags;
  // The pad template to constrain the element with
  GstStaticPadTemplate src_template;
  // The default options to apply on the filter
  filter_option* default_options;
  // Requested output destination
  const gchar* destination;
  // Signal presets (comma-separated)
  const gchar* signal_presets;
} subelement_info;

#define GPAC_TF_SUBELEMENT_COMMON(name, caps)                   \
  .filter_name = (name),                                        \
  .src_template = GST_STATIC_PAD_TEMPLATE(                      \
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(caps)), \
  .flags = 0

#define GPAC_TF_SUBELEMENT_COMMON_FLAGS(name, caps, _flags)    \
  .filter_name = (name), .flags = (_flags),                    \
  .src_template = GST_STATIC_PAD_TEMPLATE(                     \
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(caps))

#define GPAC_TF_SUBELEMENT(name, caps, opts) \
  { .alias_name = (name),                    \
    GPAC_TF_SUBELEMENT_COMMON(name, caps),   \
    .default_options = (opts) }
#define GPAC_TF_SUBELEMENT_FLAGS(name, caps, flags, opts) \
  { .alias_name = (name),                                 \
    GPAC_TF_SUBELEMENT_COMMON_FLAGS(name, caps, flags),   \
    .default_options = (opts) }

#define GPAC_TF_SUBELEMENT_AS(alias, name, caps, opts) \
  { .alias_name = (alias),                             \
    GPAC_TF_SUBELEMENT_COMMON(name, caps),             \
    .default_options = (opts) }
#define GPAC_TF_SUBELEMENT_AS_FLAGS(alias, name, caps, flags, opts) \
  { .alias_name = (alias),                                          \
    GPAC_TF_SUBELEMENT_COMMON_FLAGS(name, caps, flags),             \
    .default_options = (opts) }

#define GPAC_TF_SUBELEMENT_CUSTOM(                      \
  alias, name, caps, flags, opts, dest, spresets)       \
  { .alias_name = (alias),                              \
    GPAC_TF_SUBELEMENT_COMMON_FLAGS(name, caps, flags), \
    .default_options = (opts),                          \
    .destination = (dest),                              \
    .signal_presets = (spresets) }

#define GPAC_TF_FILTER_OPTION(name, value, forced) { name, value, forced }
#define GPAC_TF_FILTER_OPTION_ARRAY(...) \
  (filter_option[])                      \
  {                                      \
    __VA_ARGS__,                         \
    {                                    \
      NULL, NULL, FALSE                  \
    }                                    \
  }

//
// Subelement definitions
//

static subelement_info subelements[] = {
  GPAC_TF_SUBELEMENT_AS(
    "cmafmux",
    "mp4mx",
    QT_CMAF_CAPS,
    GPAC_TF_FILTER_OPTION_ARRAY(GPAC_TF_FILTER_OPTION("cmaf", "cmf2", TRUE),
                                GPAC_TF_FILTER_OPTION("store", "frag", FALSE))),
  GPAC_TF_SUBELEMENT("mp4mx", QT_CAPS, NULL),
  GPAC_TF_SUBELEMENT_CUSTOM(
    "hls",
    "dasher",
    NULL, // No caps, this is a sink only element
    GPAC_SE_SINK_ONLY | GPAC_SE_REQUIRES_MEMOUT,
    GPAC_TF_FILTER_OPTION_ARRAY(
      GPAC_TF_FILTER_OPTION("mname", "master.m3u8", TRUE),
      GPAC_TF_FILTER_OPTION("cmaf", "cmf2", TRUE),
      GPAC_TF_FILTER_OPTION("dmode", "dynamic", FALSE)),
    "master.m3u8",
    "dasher_all"),
  GPAC_TF_SUBELEMENT_AS("tsmx", "m2tsmx", MPEG_TS_CAPS, NULL),
};

/**
 * Quark
 */
#define GST_GPAC_PARAMS_QDATA g_quark_from_static_string("gpac-params")
#define GST_GPAC_GET_PARAMS(klass)                                    \
  g_type_get_qdata(G_OBJECT_CLASS_TYPE(klass), GST_GPAC_PARAMS_QDATA)

typedef struct _GstGpacParams
{
  GType private_type;
  gboolean is_single;
  gboolean is_inside_sink;
  subelement_info* info;
  guint registered_signals[GPAC_SIGNAL_LAST];
} GstGpacParams;

/**
 * Register a private subelement with the GPAC/GStreamer wrapper.
 *
 * @param se_info Information about the subelement to register
 * @param is_inside_sink Whether the subelement is inside a sink bin
 * @return GType of the registered subelement
 * @note This element won't be registered with GStreamer, but it will be
 * available to instantiate
 */
GType
gst_gpac_tf_register_custom(subelement_info* se_info, gboolean is_inside_sink);

G_END_DECLS
