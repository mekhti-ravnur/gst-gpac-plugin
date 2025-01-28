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

#include "common.h"
#include "gpacmessages.h"
#include "lib/pid.h"
#include <gpac/iso639.h>

#define TAGS_HANDLER_SIGNATURE(prop_nickname)                \
  gboolean prop_nickname##_tags_handler(GPAC_PROP_IMPL_ARGS)

#define DEFAULT_HANDLER(prop_nickname)                       \
  gboolean prop_nickname##_tags_handler(GPAC_PROP_IMPL_ARGS) \
  {                                                          \
    return FALSE;                                            \
  }

//
// Default Tags handlers
//
DEFAULT_HANDLER(duration)

//
// Tags handlers
//
TAGS_HANDLER_SIGNATURE(id)
{
  // Get the id
  g_autofree gchar* id = NULL;
  if (!gst_tag_list_get_string(
        priv->tags, GST_TAG_CONTAINER_SPECIFIC_TRACK_ID, &id))
    return FALSE;

  // Try to get the id as an integer
  guint64 id_int = g_ascii_strtoull(id, NULL, 10);
  if (id_int > G_MAXUINT) {
    GST_ERROR("Container specific track id is too large: %s", id);
    return FALSE;
  }

  // Set the id
  SET_PROP(GF_PROP_PID_ID, PROP_UINT((uint)id_int));
  return TRUE;
}

TAGS_HANDLER_SIGNATURE(dbsize)
{
  // Get the dbsize
  guint dbsize = 0;
  // FIXME: Minimum bitrate is not the same as dbsize
  if (!gst_tag_list_get_uint(priv->tags, GST_TAG_MINIMUM_BITRATE, &dbsize))
    return FALSE;

  // Set the dbsize
  SET_PROP(GF_PROP_PID_DBSIZE, PROP_UINT(dbsize));
  return TRUE;
}

TAGS_HANDLER_SIGNATURE(bitrate)
{
  // Get the bitrate
  guint bitrate = 0;
  if (!gst_tag_list_get_uint(priv->tags, GST_TAG_BITRATE, &bitrate))
    if (!gst_tag_list_get_uint(priv->tags, GST_TAG_NOMINAL_BITRATE, &bitrate))
      return FALSE;

  // Set the bitrate
  SET_PROP(GF_PROP_PID_BITRATE, PROP_UINT(bitrate));
  return TRUE;
}

TAGS_HANDLER_SIGNATURE(max_bitrate)
{
  // Get the max bitrate
  guint max_bitrate = 0;
  if (!gst_tag_list_get_uint(priv->tags, GST_TAG_MAXIMUM_BITRATE, &max_bitrate))
    return FALSE;

  // Set the max bitrate
  SET_PROP(GF_PROP_PID_MAXRATE, PROP_UINT(max_bitrate));
  return TRUE;
}

TAGS_HANDLER_SIGNATURE(language)
{
  // Get the language code
  g_autofree gchar* language = NULL;
  if (!gst_tag_list_get_string(priv->tags, GST_TAG_LANGUAGE_CODE, &language))
    return FALSE;

  // Find the index of the language code
  gint32 lang_code = gf_lang_find(language);
  if (lang_code == -1) {
    GST_WARNING("Unknown language code: %s", language);
    return FALSE;
  }

  // Set the language code
  const gchar* lang_3cc = gf_lang_get_3cc(lang_code);
  SET_PROP(GF_PROP_PID_LANGUAGE, PROP_STRING(lang_3cc));
  return TRUE;
}
