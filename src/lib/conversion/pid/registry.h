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

#include "lib/pid.h"

//
// Macros for declaring PID property handlers
//

#define GPAC_PROP_IMPL_DECL_CAPS(prop_nickname)                   \
  gboolean prop_nickname##_caps_handler(GPAC_PID_PROP_IMPL_ARGS);

#define GPAC_PROP_IMPL_DECL_TAGS(prop_nickname)                   \
  gboolean prop_nickname##_tags_handler(GPAC_PID_PROP_IMPL_ARGS);

#define GPAC_PROP_IMPL_DECL_SEGMENT(prop_nickname)                   \
  gboolean prop_nickname##_segment_handler(GPAC_PID_PROP_IMPL_ARGS);

#define GPAC_PROP_IMPL_DECL_QUERY(prop_nickname)                   \
  gboolean prop_nickname##_query_handler(GPAC_PID_PROP_IMPL_ARGS);

#define GPAC_PROP_IMPL_DECL_DEFAULT(prop_nickname) \
  gboolean prop_nickname##_default_handler(GPAC_PID_PROP_IMPL_ARGS_NO_ELEMENT);

#define GPAC_PROP_IMPL_DECL_BUNDLE_ALL(prop_nickname) \
  GPAC_PROP_IMPL_DECL_CAPS(prop_nickname)             \
  GPAC_PROP_IMPL_DECL_TAGS(prop_nickname)             \
  GPAC_PROP_IMPL_DECL_SEGMENT(prop_nickname)          \
  GPAC_PROP_IMPL_DECL_QUERY(prop_nickname)            \
  GPAC_PROP_IMPL_DECL_DEFAULT(prop_nickname)

#define GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(prop_nickname) \
  GPAC_PROP_IMPL_DECL_CAPS(prop_nickname)              \
  GPAC_PROP_IMPL_DECL_DEFAULT(prop_nickname)

#define GPAC_PROP_IMPL_DECL_BUNDLE_TAGS(prop_nickname) \
  GPAC_PROP_IMPL_DECL_TAGS(prop_nickname)              \
  GPAC_PROP_IMPL_DECL_DEFAULT(prop_nickname)

#define GPAC_PROP_IMPL_DECL_BUNDLE_SEGMENT(prop_nickname) \
  GPAC_PROP_IMPL_DECL_SEGMENT(prop_nickname)              \
  GPAC_PROP_IMPL_DECL_DEFAULT(prop_nickname)

//
// Macros for declaring PID property handlers
//

#define GPAC_PROP_DEFINE_ALL(prop_4cc, prop_nickname) \
  { prop_4cc,                                         \
    prop_nickname##_caps_handler,                     \
    prop_nickname##_tags_handler,                     \
    prop_nickname##_segment_handler,                  \
    prop_nickname##_query_handler,                    \
    prop_nickname##_default_handler }

#define GPAC_PROP_DEFINE_CAPS(prop_4cc, prop_nickname)    \
  { prop_4cc, prop_nickname##_caps_handler,   NULL, NULL, \
    NULL,     prop_nickname##_default_handler }

#define GPAC_PROP_DEFINE_TAGS(prop_4cc, prop_nickname) \
  { prop_4cc, NULL, prop_nickname##_tags_handler,      \
    NULL,     NULL, prop_nickname##_default_handler }

#define GPAC_PROP_DEFINE_SEGMENT(prop_4cc, prop_nickname) \
  { prop_4cc, NULL,                                       \
    NULL,     prop_nickname##_segment_handler,            \
    NULL,     prop_nickname##_default_handler }

#define GPAC_PROP_DEFINE_DEFAULT(prop_4cc, prop_nickname)               \
  { prop_4cc, NULL, NULL, NULL, NULL, prop_nickname##_default_handler }

/*!
\brief PID Property Management and Reconfiguration

The value of a PID property can be set through four sources:
- Caps
- Tags
- Segment
- Query

If none of these sources set the property, the default handler is invoked. At
least one of the sources must return `TRUE` for the property to be set.

Properties can also be set using custom caps fields by prefixing the property
name with `"gpac-"`. This approach is particularly useful for setting properties
via tools like `gst-launch-1.0`. For a more structured approach, a nested
structure with `"gpac"` as the field name can be used. These custom fields will
override properties that would otherwise be set by GStreamer.

Property handlers have access to private data for each pad, which can be used to
store state information. Since the element's lock is acquired, the private data
can be safely modified, and the global context can also be utilized if required
by any property handler.

For example, the `id` property must remain consistent across subsequent calls
and must increment monotonically across all pads. This behavior is achieved by
storing the last `id` in a global context.
*/

//
// PID property handler declarations
//
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(stream_type)
GPAC_PROP_IMPL_DECL_BUNDLE_TAGS(id)
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(codec_id)
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(unframed)

GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(width)
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(height)

GPAC_PROP_IMPL_DECL_BUNDLE_TAGS(dbsize)
GPAC_PROP_IMPL_DECL_BUNDLE_TAGS(bitrate)
GPAC_PROP_IMPL_DECL_BUNDLE_TAGS(max_bitrate)

GPAC_PROP_IMPL_DECL_BUNDLE_ALL(duration)
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(timescale)
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(sample_rate)
GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(fps)

GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(num_channels)
GPAC_PROP_IMPL_DECL_BUNDLE_TAGS(language)

GPAC_PROP_IMPL_DECL_BUNDLE_CAPS(decoder_config)

typedef struct
{
  u32 prop_4cc;

  // Handlers
  gboolean (*caps_handler)(GPAC_PID_PROP_IMPL_ARGS);
  gboolean (*tags_handler)(GPAC_PID_PROP_IMPL_ARGS);
  gboolean (*segment_handler)(GPAC_PID_PROP_IMPL_ARGS);
  gboolean (*query_handler)(GPAC_PID_PROP_IMPL_ARGS);

  // Default fallback handler
  gboolean (*default_handler)(GPAC_PID_PROP_IMPL_ARGS_NO_ELEMENT);
} prop_registry_entry;

static prop_registry_entry prop_registry[] = {
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_STREAM_TYPE, stream_type),
  GPAC_PROP_DEFINE_TAGS(GF_PROP_PID_ID, id),
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_CODECID, codec_id),
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_UNFRAMED, unframed),

  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_WIDTH, width),
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_HEIGHT, height),

  GPAC_PROP_DEFINE_TAGS(GF_PROP_PID_DBSIZE, dbsize),
  GPAC_PROP_DEFINE_TAGS(GF_PROP_PID_BITRATE, bitrate),
  GPAC_PROP_DEFINE_TAGS(GF_PROP_PID_MAXRATE, max_bitrate),

  // DO NOT change the order of the next 4 properties
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_SAMPLE_RATE, sample_rate),
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_FPS, fps),
  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_TIMESCALE, timescale),
  GPAC_PROP_DEFINE_ALL(GF_PROP_PID_DURATION, duration),

  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_NUM_CHANNELS, num_channels),
  GPAC_PROP_DEFINE_TAGS(GF_PROP_PID_LANGUAGE, language),

  GPAC_PROP_DEFINE_CAPS(GF_PROP_PID_DECODER_CONFIG, decoder_config),
};

u32
gpac_pid_get_num_supported_props()
{
  return G_N_ELEMENTS(prop_registry);
}
