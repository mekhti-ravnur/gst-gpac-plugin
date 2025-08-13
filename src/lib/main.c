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

#include "lib/main.h"
#include "gpacmessages.h"

static void
gpac_log_callback(void* cbck,
                  GF_LOG_Level log_level,
                  GF_LOG_Tool log_tool,
                  const char* fmt,
                  va_list vlist)
{
  GstElement* element = (GstElement*)cbck;

  char msg[1024];
  vsnprintf(msg, sizeof(msg), fmt, vlist);

  switch (log_level) {
    case GF_LOG_ERROR:
      GST_ERROR_OBJECT(element, "%s", msg);
      break;
    case GF_LOG_WARNING:
      GST_WARNING_OBJECT(element, "%s", msg);
      break;
    case GF_LOG_INFO:
      GST_INFO_OBJECT(element, "%s", msg);
      break;
    case GF_LOG_DEBUG:
      GST_DEBUG_OBJECT(element, "%s", msg);
      break;
    default:
      GST_LOG_OBJECT(element, "%s", msg);
      break;
  }
}

gboolean
gpac_init(GPAC_Context* ctx, GstElement* element)
{
  gpac_return_val_if_fail(gf_sys_init(GF_MemTrackerNone, NULL), FALSE);
  gf_log_set_callback(element, gpac_log_callback);
  gf_log_set_tools_levels("all@warning:strict", GF_TRUE);
  return TRUE;
}

void
gpac_destroy(GPAC_Context* ctx)
{
  gf_sys_close();
}
