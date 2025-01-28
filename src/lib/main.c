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

gboolean
gpac_init(GPAC_Context* ctx)
{
  gpac_return_val_if_fail(gf_sys_init(GF_MemTrackerNone, NULL), FALSE);
  return TRUE;
}

void
gpac_destroy(GPAC_Context* ctx)
{
  gf_sys_close();

  // Free the properties
  while (gf_list_count(ctx->prop.properties)) {
    void* item = gf_list_pop_front(ctx->prop.properties);
    g_free(item);
  }
  gf_list_del(ctx->prop.properties);
  g_free(ctx->prop.props_as_argv);
}
