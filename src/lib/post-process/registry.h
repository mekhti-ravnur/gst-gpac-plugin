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

#include "lib/memio.h"

#include <gpac/filters.h>
#include <gst/gst.h>

#define GPAC_FILTER_PP_IMPL_DECL(filter_name)                               \
  void filter_name##_ctx_init(void** process_ctx);                          \
  void filter_name##_ctx_free(void* process_ctx);                           \
  GF_Err filter_name##_configure_pid(GF_Filter* filter, GF_FilterPid* pid); \
  GF_Err filter_name##_post_process(                                        \
    GF_Filter* filter, GF_FilterPid* pid, GF_FilterPacket* pck);            \
  Bool filter_name##_process_event(GF_Filter* filter,                       \
                                   const GF_FilterEvent* evt);              \
  GPAC_FilterPPRet filter_name##_consume(                                   \
    GF_Filter* filter, GF_FilterPid* pid, void** outptr);

#define GPAC_FILTER_PP_IMPL_DEFINE(filter_name) \
  { #filter_name,                               \
    filter_name##_ctx_init,                     \
    filter_name##_ctx_free,                     \
    filter_name##_configure_pid,                \
    filter_name##_post_process,                 \
    filter_name##_process_event,                \
    filter_name##_consume }

// Forward declarations
GPAC_FILTER_PP_IMPL_DECL(generic);
GPAC_FILTER_PP_IMPL_DECL(mp4mx);
GPAC_FILTER_PP_IMPL_DECL(dasher);

typedef struct
{
  const gchar* filter_name;

  // Handlers
  void (*ctx_init)(void** process_ctx);
  void (*ctx_free)(void* process_ctx);
  GF_Err (*configure_pid)(GF_Filter* filter, GF_FilterPid* pid);
  GF_Err (*post_process)(GF_Filter* filter,
                         GF_FilterPid* pid,
                         GF_FilterPacket* pck);
  Bool (*process_event)(GF_Filter* filter, const GF_FilterEvent* evt);
  GPAC_FilterPPRet (*consume)(GF_Filter* filter,
                              GF_FilterPid* pid,
                              void** outptr);
} post_process_registry_entry;

static post_process_registry_entry pp_registry[] = {
  GPAC_FILTER_PP_IMPL_DEFINE(generic),
  GPAC_FILTER_PP_IMPL_DEFINE(mp4mx),
  GPAC_FILTER_PP_IMPL_DEFINE(dasher),
};

static inline u32
gpac_filter_get_num_supported_post_process()
{
  return G_N_ELEMENTS(pp_registry);
}

static inline post_process_registry_entry*
gpac_filter_get_post_process_registry_entry(const gchar* filter_name)
{
  for (u32 i = 0; i < gpac_filter_get_num_supported_post_process(); i++) {
    if (g_strcmp0(pp_registry[i].filter_name, filter_name) == 0) {
      return &pp_registry[i];
    }
  }

  // Not found, return the generic post-process handler
  return &pp_registry[0];
}
