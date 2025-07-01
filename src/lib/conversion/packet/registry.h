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

#include "lib/packet.h"

//
// Macros for declaring packet property handlers
//

#define GPAC_PROP_IMPL_DECL(prop_nickname)                   \
  gboolean prop_nickname##_handler(GPAC_PCK_PROP_IMPL_ARGS);

//
// Macros for declaring property handlers
//

#define GPAC_PROP_DEFINE(prop_4cc, prop_nickname) \
  { prop_4cc, NULL, prop_nickname##_handler }

#define GPAC_PROP_DEFINE_STR(prop_str, prop_nickname) \
  { 0, prop_str, prop_nickname##_handler }

//
// Property handler declarations
//
GPAC_PROP_IMPL_DECL(id3);

typedef struct
{
  u32 prop_4cc;
  const gchar* prop_str;

  gboolean (*handler)(GPAC_PCK_PROP_IMPL_ARGS);
} prop_registry_entry;

static prop_registry_entry prop_registry[] = { GPAC_PROP_DEFINE_STR("id3",
                                                                    id3) };

u32
gpac_pck_get_num_supported_props()
{
  return G_N_ELEMENTS(prop_registry);
}
