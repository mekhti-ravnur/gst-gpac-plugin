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

#include "lib/filters/filters.h"
#include "lib/memio.h"
#include <gpac/internal/isomedia_dev.h>

static guint32 INIT_BOXES[] = { GF_ISOM_BOX_TYPE_FTYP,
                                GF_ISOM_BOX_TYPE_FREE,
                                GF_ISOM_BOX_TYPE_MOOV,
                                0 };
static guint32 HEADER_BOXES[] = { GF_ISOM_BOX_TYPE_STYP,
                                  GF_ISOM_BOX_TYPE_MOOF,
                                  GF_ISOM_BOX_TYPE_MDAT,
                                  0 };

typedef enum _BufferType
{
  INIT = 0,
  HEADER,
  DATA,

  LAST
} BufferType;

struct BoxMapping
{
  BufferType type;
  guint32* boxes;
} box_mapping[LAST] = {
  { INIT, INIT_BOXES },
  { HEADER, HEADER_BOXES },
  { DATA, NULL },
};

typedef struct
{
  GstBuffer* buffer;
  gboolean is_complete;
  guint32 expected_size;
} BufferContents;

typedef struct
{
  // Output queue for complete GOPs
  GQueue* output_queue;

  // State
  BufferType current_type;

  // Buffer contents for the init, header, and data
  BufferContents* contents[3];

  // Input context
  guint32 timescale;

  // Reader context
  guint64 offset;
  guint64 size;
  guint32 leftover;
} Mp4mxCtx;

void
mp4mx_ctx_init(void** process_ctx)
{
  *process_ctx = g_new0(Mp4mxCtx, 1);
  Mp4mxCtx* ctx = (Mp4mxCtx*)*process_ctx;

  // Initialize the context
  ctx->output_queue = g_queue_new();
  ctx->current_type = INIT;

  // Initialize the buffer contents
  for (guint i = 0; i < LAST; i++) {
    ctx->contents[i] = g_new0(BufferContents, 1);
    ctx->contents[i]->is_complete = FALSE;
  }
}

void
mp4mx_ctx_free(void* process_ctx)
{
  Mp4mxCtx* ctx = (Mp4mxCtx*)process_ctx;

  // Free the output queue
  while (!g_queue_is_empty(ctx->output_queue))
    gst_buffer_unref((GstBuffer*)g_queue_pop_head(ctx->output_queue));
  g_queue_free(ctx->output_queue);

  // Free the buffer contents
  for (guint i = 0; i < LAST; i++) {
    if (ctx->contents[i]->buffer)
      gst_buffer_unref(ctx->contents[i]->buffer);
    g_free(ctx->contents[i]);
  }

  // Free the context
  g_free(ctx);
}

GF_Err
mp4mx_configure_pid(GF_Filter* filter, GF_FilterPid* pid)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  Mp4mxCtx* mp4mx_ctx = (Mp4mxCtx*)ctx->process_ctx;

  // Get the timescale from the PID
  mp4mx_ctx->timescale = GST_SECOND;
  const GF_PropertyValue* p =
    gf_filter_pid_get_property(pid, GF_PROP_PID_TIMESCALE);
  if (p)
    mp4mx_ctx->timescale = p->value.uint;

  return GF_OK;
}

GF_Err
mp4mx_post_process(GF_Filter* filter, GF_FilterPacket* pck)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  Mp4mxCtx* mp4mx_ctx = (Mp4mxCtx*)ctx->process_ctx;
  if (!pck)
    return GF_OK;

  // Declare variables
  GstMemory* mem;

  // Get the data
  u32 size;
  const u8* data = gf_filter_pck_get_data(pck, &size);

  mp4mx_ctx->size += size;
  guint32 offset = 0;

  // If we have leftover data, append it to the current buffer
  if (mp4mx_ctx->leftover) {
    guint32 leftover = MIN(mp4mx_ctx->leftover, size);
    mem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                 (gpointer)data,
                                 leftover,
                                 0,
                                 leftover,
                                 pck,
                                 (GDestroyNotify)gf_filter_pck_unref);
    gf_filter_pck_ref(&pck);

    // Append the memory to the buffer
    g_assert(mp4mx_ctx->contents[mp4mx_ctx->current_type]->buffer);
    gst_buffer_append_memory(
      mp4mx_ctx->contents[mp4mx_ctx->current_type]->buffer, mem);

    // Update the leftover and data
    mp4mx_ctx->leftover -= leftover;
    offset += leftover;
  }

  // Iterate over the boxes
  while (offset < size) {
    guint32 box_size =
      GUINT32_FROM_LE((data[offset] << 24) | (data[offset + 1] << 16) |
                      (data[offset + 2] << 8) | data[offset + 3]);
    guint32 box_type =
      GUINT32_FROM_LE((data[offset + 4] << 24) | (data[offset + 5] << 16) |
                      (data[offset + 6] << 8) | data[offset + 7]);

    // Special handling for mdat
    if (box_type == GF_ISOM_BOX_TYPE_MDAT) {
      // Create a new memory for the mdat header
      mem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                   (gpointer)data + offset,
                                   8,
                                   0,
                                   8,
                                   pck,
                                   (GDestroyNotify)gf_filter_pck_unref);
      gf_filter_pck_ref(&pck);

      // Append the memory to the buffer
      g_assert(mp4mx_ctx->contents[HEADER]->buffer);
      gst_buffer_append_memory(mp4mx_ctx->contents[HEADER]->buffer, mem);

      // Complete the header
      mp4mx_ctx->contents[HEADER]->is_complete = TRUE;
      mp4mx_ctx->current_type = DATA;

      // Set the expected size
      mp4mx_ctx->contents[DATA]->expected_size = box_size - 8;

      // Move the offset
      offset += 8;
      box_size -= 8;
      goto append_memory;
    }

    guint32 last_type;
    for (last_type = mp4mx_ctx->current_type; last_type < LAST; last_type++) {
      // Check if the current box is related to current type
      gboolean found = FALSE;
      for (guint i = 0; box_mapping[last_type].boxes[i]; i++) {
        if (box_mapping[last_type].boxes[i] == box_type) {
          found = TRUE;
          break;
        }
      }
      if (found)
        break;

      // If not found, current type is completed
      mp4mx_ctx->contents[last_type]->is_complete = TRUE;
    }

    // Check if the box is unknown
    if (last_type == LAST) {
      GST_ERROR("Unknown box type: %s", gf_4cc_to_str(box_type));
      g_assert_not_reached();
    }

    // Set the current type
    mp4mx_ctx->current_type = last_type;

  append_memory:
    // Create a new memory
    mem = gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                 (gpointer)data + offset,
                                 box_size,
                                 0,
                                 box_size,
                                 pck,
                                 (GDestroyNotify)gf_filter_pck_unref);
    gf_filter_pck_ref(&pck);

    // Append the memory to the buffer
    BufferType type = mp4mx_ctx->current_type;
    if (!mp4mx_ctx->contents[type]->buffer) {
      GstBuffer* buf = mp4mx_ctx->contents[type]->buffer = gst_buffer_new();

      // Set the flags
      switch (type) {
        case INIT:
          if (!ctx->is_continuous) {
            GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_DISCONT);
            ctx->is_continuous = TRUE;
          }
          // fallthrough
        case HEADER:
          GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_HEADER);
          break;
        case DATA:
          GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_MARKER);
          GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT);
          break;

        default:
          break;
      }

      // Calculate the PTS
      guint64 pts = gf_timestamp_rescale(
        gf_filter_pck_get_cts(pck), mp4mx_ctx->timescale, GST_SECOND);
      pts += ctx->global_offset;
      GST_BUFFER_PTS(buf) = pts;

      // Calculate the DTS
      guint64 dts = gf_timestamp_rescale(
        gf_filter_pck_get_dts(pck), mp4mx_ctx->timescale, GST_SECOND);
      dts += ctx->global_offset;
      GST_BUFFER_DTS(buf) = dts;

      // Calculate the duration
      guint64 duration = gf_timestamp_rescale(
        gf_filter_pck_get_duration(pck), mp4mx_ctx->timescale, GST_SECOND);
      GST_BUFFER_DURATION(buf) = type == INIT ? GST_CLOCK_TIME_NONE : duration;
    }

    // Append the memory to the buffer
    gst_buffer_append_memory(mp4mx_ctx->contents[type]->buffer, mem);

    // Move to the next box
    offset += box_size;
  }

  // Update leftover and global offset
  if (mp4mx_ctx->offset + offset > mp4mx_ctx->size) {
    mp4mx_ctx->leftover = mp4mx_ctx->offset + offset - mp4mx_ctx->size;
  } else {
    g_assert(mp4mx_ctx->leftover == 0);
    mp4mx_ctx->offset = MIN(mp4mx_ctx->offset + offset, mp4mx_ctx->size);
  }

  // Check if the data is complete
  if (mp4mx_ctx->contents[DATA]->buffer) {
    guint32 expected_size = mp4mx_ctx->contents[DATA]->expected_size;
    guint32 buffer_size =
      gst_buffer_get_size(mp4mx_ctx->contents[DATA]->buffer);

    // Check if the buffer is complete
    if (buffer_size >= expected_size) {
      g_assert(buffer_size == expected_size);

      // Update the state
      mp4mx_ctx->current_type = HEADER;
      mp4mx_ctx->contents[DATA]->is_complete = TRUE;
    }
  }

  // Check if the GOP is complete
  if (mp4mx_ctx->contents[HEADER]->is_complete &&
      mp4mx_ctx->contents[DATA]->is_complete) {
    // Create a buffer list
    GstBufferList* buffer_list = gst_buffer_list_new();

    // Init only if it's present
    if (mp4mx_ctx->contents[INIT]->is_complete) {
      gst_buffer_list_add(buffer_list, mp4mx_ctx->contents[INIT]->buffer);

      // Init won't have timings set, set it using header
      GST_BUFFER_PTS(mp4mx_ctx->contents[INIT]->buffer) =
        GST_BUFFER_PTS(mp4mx_ctx->contents[HEADER]->buffer);
      GST_BUFFER_DTS(mp4mx_ctx->contents[INIT]->buffer) =
        GST_BUFFER_DTS(mp4mx_ctx->contents[HEADER]->buffer) -
        GST_BUFFER_DURATION(mp4mx_ctx->contents[HEADER]->buffer);
    }

    gst_buffer_list_add(buffer_list, mp4mx_ctx->contents[HEADER]->buffer);
    gst_buffer_list_add(buffer_list, mp4mx_ctx->contents[DATA]->buffer);

    // Enqueue the buffer
    g_queue_push_tail(mp4mx_ctx->output_queue, buffer_list);

    // Reset the buffer contents
    for (guint i = 0; i < LAST; i++) {
      mp4mx_ctx->contents[i]->buffer = NULL;
      mp4mx_ctx->contents[i]->is_complete = FALSE;
      mp4mx_ctx->contents[i]->expected_size = 0;
    }
  }

  return GF_OK;
}

GPAC_FilterPPRet
mp4mx_consume(GF_Filter* filter, void** outptr)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  Mp4mxCtx* mp4mx_ctx = (Mp4mxCtx*)ctx->process_ctx;
  *outptr = NULL;

  // Check if the queue is empty
  if (g_queue_is_empty(mp4mx_ctx->output_queue))
    return GPAC_FILTER_PP_RET_EMPTY;

  // Assign the output
  *outptr = g_queue_pop_head(mp4mx_ctx->output_queue);
  return GPAC_FILTER_PP_RET_BUFFER_LIST;
}
