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
#include "lib/memio.h"

typedef struct
{
  GQueue* output_queue;
} GenericCtx;

void
generic_ctx_init(void** process_ctx)
{
  *process_ctx = g_new0(GenericCtx, 1);
  GenericCtx* ctx = (GenericCtx*)*process_ctx;
  ctx->output_queue = g_queue_new();
}

void
generic_ctx_free(void* process_ctx)
{
  GenericCtx* ctx = (GenericCtx*)process_ctx;

  // Free the output queue
  while (!g_queue_is_empty(ctx->output_queue))
    gst_buffer_unref((GstBuffer*)g_queue_pop_head(ctx->output_queue));
  g_queue_free(ctx->output_queue);

  // Free the context
  g_free(ctx);
}

GF_Err
generic_configure_pid(GF_Filter* filter, GF_FilterPid* pid)
{
  return GF_OK;
}

Bool
generic_process_event(GF_Filter* filter, const GF_FilterEvent* evt)
{
  return GF_FALSE; // No event processing
}

GF_Err
generic_post_process(GF_Filter* filter, GF_FilterPid* pid, GF_FilterPacket* pck)
{
  GPAC_MemOutPIDContext* ctx =
    (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(pid);
  GenericCtx* generic_ctx = (GenericCtx*)ctx->private_ctx;

  if (!pck)
    return GF_OK;

  // Get the data
  u32 size;
  const u8* data = gf_filter_pck_get_data(pck, &size);
  gf_filter_pck_ref(&pck);

  // Create a new buffer
  GstBuffer* buffer =
    gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,           // flags
                                (u8*)data,                          // data
                                size,                               // maxsize
                                0,                                  // offset
                                size,                               // size
                                pck,                                // user_data
                                (GDestroyNotify)gf_filter_pck_unref // notify
    );

  // Enqueue the buffer
  g_queue_push_tail(generic_ctx->output_queue, buffer);
  return GF_OK;
}

GPAC_FilterPPRet
generic_consume(GF_Filter* filter, GF_FilterPid* pid, void** outptr)
{
  GPAC_MemOutPIDContext* ctx =
    (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(pid);
  GenericCtx* generic_ctx = (GenericCtx*)ctx->private_ctx;

  // Check if the queue is empty
  if (g_queue_is_empty(generic_ctx->output_queue))
    return GPAC_FILTER_PP_RET_EMPTY;

  // Assign the output
  if (outptr) {
    *outptr = g_queue_pop_head(generic_ctx->output_queue);
    return GPAC_FILTER_PP_RET_BUFFER;
  }
  return GPAC_FILTER_PP_RET_NULL;
}
