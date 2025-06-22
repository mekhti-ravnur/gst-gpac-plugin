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
#include "lib/signals.h"

#include <gio/gio.h>
#include <gpac/network.h>

GST_DEBUG_CATEGORY_STATIC(gpac_dasher);
#define GST_CAT_DEFAULT gpac_dasher

typedef struct
{
  gchar* name;        // Name of the file
  GOutputStream* out; // Output stream for the file
} FileAbstract;

typedef struct
{
  // current file being processed
  FileAbstract* current_file;

  gboolean is_manifest;
  guint32 dash_state;
  const gchar* dst; // destination file path
} DasherCtx;

void
dasher_ctx_init(void** process_ctx)
{
  *process_ctx = g_new0(DasherCtx, 1);
  DasherCtx* ctx = (DasherCtx*)*process_ctx;

  GST_DEBUG_CATEGORY_INIT(
    gpac_dasher, "gpacdasherpp", 0, "GPAC dasher post-processor");
}

void
dasher_ctx_free(void* process_ctx)
{
  DasherCtx* ctx = (DasherCtx*)process_ctx;

  // Free the current file if it exists
  if (ctx->current_file) {
    if (ctx->current_file->out) {
      g_output_stream_close(ctx->current_file->out, NULL, NULL);
      g_object_unref(ctx->current_file->out);
    }
    g_free(ctx->current_file->name);
    g_free(ctx->current_file);
  }

  // Free the context
  g_free(ctx);
}

GF_Err
dasher_configure_pid(GF_Filter* filter, GF_FilterPid* pid)
{
  GPAC_MemOutPIDContext* ctx =
    (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(pid);
  DasherCtx* dasher_ctx = (DasherCtx*)ctx->private_ctx;
  GPAC_MemOutPIDFlags udta_flags = gf_filter_pid_get_udta_flags(pid);

  // We'll transfer the data over signals
  udta_flags |= GPAC_MEMOUT_PID_FLAG_DONT_CONSUME;
  gf_filter_pid_set_udta_flags(pid, udta_flags);

  const GF_PropertyValue* p =
    gf_filter_pid_get_property(pid, GF_PROP_PID_IS_MANIFEST);
  if (p && p->value.uint)
    dasher_ctx->is_manifest = TRUE;
  else
    dasher_ctx->dash_state = 1;

  // Get the destination path
  GF_PropertyValue dst;
  Bool found = gf_filter_get_arg(filter, "dst", &dst);
  if (found)
    dasher_ctx->dst = g_strdup(dst.value.string);

  GPAC_MemOutPrivateContext* fctx =
    (GPAC_MemOutPrivateContext*)gf_filter_pid_get_alias_udta(pid);
  if (fctx) {
    dasher_ctx->dst = fctx->dst;
  } else {
    fctx = (GPAC_MemOutPrivateContext*)gf_filter_get_udta(filter);
    dasher_ctx->dst = fctx->dst;
  }

  return GF_OK;
}

Bool
dasher_process_event(GF_Filter* filter, const GF_FilterEvent* evt)
{
  if (evt->base.type == GF_FEVT_FILE_DELETE) {
    GPAC_MemIoContext* io_ctx =
      (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
    GPAC_MemOutPIDContext* ctx =
      (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(evt->base.on_pid);
    DasherCtx* dasher_ctx = (DasherCtx*)ctx->private_ctx;

    GST_TRACE_OBJECT(io_ctx->sess->element,
                     "Received file delete event for PID %s: %s",
                     gf_filter_pid_get_name(evt->base.on_pid),
                     evt->file_del.url);

    gpac_signal_try_emit(io_ctx->sess->element,
                         GPAC_SIGNAL_DASHER_DELETE_SEGMENT,
                         evt->file_del.url,
                         NULL);

    return GF_TRUE;
  }
  return GF_FALSE;
}

void
dasher_open_close_file(GF_Filter* filter, GF_FilterPid* pid, const char* name)
{
  GPAC_MemIoContext* io_ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  GPAC_MemOutPIDContext* ctx =
    (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(pid);
  DasherCtx* dasher_ctx = (DasherCtx*)ctx->private_ctx;

  // If the file is already open, close it
  if (dasher_ctx->current_file) {
    GST_TRACE_OBJECT(io_ctx->sess->element,
                     "Closing file for PID %s: %s",
                     gf_filter_pid_get_name(pid),
                     dasher_ctx->current_file->name);
    if (dasher_ctx->current_file->out)
      g_output_stream_close(dasher_ctx->current_file->out, NULL, NULL);
    g_free(dasher_ctx->current_file->name);
    g_free(dasher_ctx->current_file);
    dasher_ctx->current_file = NULL;
  }

  if (!name)
    return; // No file to open

  GST_TRACE_OBJECT(io_ctx->sess->element,
                   "Opening new file for PID %s: %s",
                   gf_filter_pid_get_name(pid),
                   name);

  // Create a new file
  FileAbstract* file = dasher_ctx->current_file = g_new0(FileAbstract, 1);
  file->name = g_strdup(name);

  // Decide on the file flags
  if (dasher_ctx->is_manifest) {
    if (g_strcmp0(name, dasher_ctx->dst) == 0) {
      gpac_signal_try_emit(io_ctx->sess->element,
                           GPAC_SIGNAL_DASHER_MANIFEST,
                           file->name,
                           &file->out);
    } else {
      gpac_signal_try_emit(io_ctx->sess->element,
                           GPAC_SIGNAL_DASHER_MANIFEST_VARIANT,
                           file->name,
                           &file->out);
    }
  } else {
    if (g_strcmp0(name, dasher_ctx->dst) == 0) {
      gpac_signal_try_emit(io_ctx->sess->element,
                           GPAC_SIGNAL_DASHER_SEGMENT_INIT,
                           file->name,
                           &file->out);
    } else {
      gpac_signal_try_emit(io_ctx->sess->element,
                           GPAC_SIGNAL_DASHER_SEGMENT,
                           file->name,
                           &file->out);
    }
  }
}

void
dasher_setup_file(GF_Filter* filter, GF_FilterPid* pid)
{
  GPAC_MemOutPIDContext* ctx =
    (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(pid);
  DasherCtx* dasher_ctx = (DasherCtx*)ctx->private_ctx;
  const char* dst = dasher_ctx->dst;

  const GF_PropertyValue* p =
    gf_filter_pid_get_property(pid, GF_PROP_PID_OUTPATH);
  if (p && p->value.string) {
    dasher_open_close_file(filter, pid, p->value.string);
    return;
  }

  if (dasher_ctx->dst) {
    dasher_open_close_file(filter, pid, dasher_ctx->dst);
  } else {
    p = gf_filter_pid_get_property(pid, GF_PROP_PID_FILEPATH);
    if (!p)
      p = gf_filter_pid_get_property(pid, GF_PROP_PID_URL);
    if (p && p->value.string)
      dasher_open_close_file(filter, pid, p->value.string);
  }
}

GF_Err
dasher_post_process(GF_Filter* filter, GF_FilterPid* pid, GF_FilterPacket* pck)
{
  GPAC_MemIoContext* io_ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  GPAC_MemOutPIDContext* ctx =
    (GPAC_MemOutPIDContext*)gf_filter_pid_get_udta(pid);
  DasherCtx* dasher_ctx = (DasherCtx*)ctx->private_ctx;
  const GF_PropertyValue *fname, *p;

  if (!pck) {
    if (gf_filter_pid_is_eos(pid) && !gf_filter_pid_is_flush_eos(pid))
      dasher_open_close_file(filter, pid, NULL);
    return GF_OK; // No packet to process
  }

  // Check the packet framing
  Bool start, end;
  gf_filter_pck_get_framing(pck, &start, &end);
  if (dasher_ctx->dash_state) {
    p = gf_filter_pck_get_property(pck, GF_PROP_PCK_FILENUM);
    if (p) {
      p = gf_filter_pck_get_property(pck, GF_PROP_PCK_FILENAME);
      if (p && p->value.string)
        start = GF_TRUE;
    }

    p = gf_filter_pck_get_property(pck, GF_PROP_PCK_EODS);
    if (p && p->value.boolean)
      end = GF_TRUE;
  }

  if (start) {
    // Previous file has ended, move to the next file
    if (dasher_ctx->current_file)
      dasher_open_close_file(filter, pid, NULL);

    const GF_PropertyValue *ext, *fnum, *rel;
    Bool explicit_overwrite = GF_FALSE;
    const char* name = NULL;
    fname = ext = NULL;
    // file num increased per packet, open new file
    fnum = gf_filter_pck_get_property(pck, GF_PROP_PCK_FILENUM);
    if (fnum) {
      fname = gf_filter_pid_get_property(pid, GF_PROP_PID_OUTPATH);
      ext = gf_filter_pid_get_property(pid, GF_PROP_PID_FILE_EXT);
      if (!fname)
        name = dasher_ctx->dst;
    }
    // filename change at packet start, open new file
    if (!fname)
      fname = gf_filter_pck_get_property(pck, GF_PROP_PCK_FILENAME);
    if (fname)
      name = fname->value.string;

    if (name) {
      dasher_open_close_file(filter, pid, name);
    } else if (!dasher_ctx->current_file) {
      dasher_setup_file(filter, pid);
    }
  }

  // We must be actively working on a file by now
  if (G_UNLIKELY(!dasher_ctx->current_file)) {
    GST_ELEMENT_ERROR(io_ctx->sess->element,
                      STREAM,
                      FAILED,
                      (NULL),
                      ("No current file to write to for PID %s\n",
                       gf_filter_pid_get_name(pid)));
    gf_filter_abort(filter);
    return GF_IO_ERR;
  }

  // Get the data
  u32 size;
  const u8* data = gf_filter_pck_get_data(pck, &size);
  if (G_UNLIKELY(!data || !size)) {
    GST_ELEMENT_WARNING(
      io_ctx->sess->element,
      STREAM,
      FAILED,
      (NULL),
      ("Received empty packet for PID %s\n", gf_filter_pid_get_name(pid)));
    return GF_OK;
  }

  // Get the current file
  FileAbstract* file = dasher_ctx->current_file;
  if (G_UNLIKELY(!file->out)) {
    GST_ELEMENT_ERROR(io_ctx->sess->element,
                      STREAM,
                      FAILED,
                      (NULL),
                      ("No output stream for file %s",
                       dasher_ctx->current_file->name
                         ? dasher_ctx->current_file->name
                         : "unknown"));
    gf_filter_abort(filter);
    return GF_IO_ERR;
  }

  // Write the data to the output stream
  gssize bytes_written =
    g_output_stream_write(file->out, data, size, NULL, NULL);
  if (bytes_written < 0) {
    GST_ELEMENT_ERROR(io_ctx->sess->element,
                      STREAM,
                      FAILED,
                      (NULL),
                      ("Failed to write data to output stream for file %s",
                       file->name ? file->name : "unknown"));
  } else if (bytes_written < (gssize)size) {
    GST_ELEMENT_WARNING(io_ctx->sess->element,
                        STREAM,
                        FAILED,
                        (NULL),
                        ("Partial write to output stream for file %s, "
                         "expected: %" G_GSIZE_FORMAT
                         ", written: %" G_GSSIZE_FORMAT,
                         file->name ? file->name : "unknown",
                         (gssize)size,
                         bytes_written));
  }

  GST_TRACE_OBJECT(io_ctx->sess->element,
                   "Sent %s over a signal, size: %" G_GSIZE_FORMAT,
                   file->name ? file->name : "unknown",
                   (gssize)size);

  // Close the output stream
  if (end)
    dasher_open_close_file(filter, pid, NULL);

  return GF_OK;
}

GPAC_FilterPPRet
dasher_consume(GF_Filter* filter, GF_FilterPid* pid, void** outptr)
{
  // We don't output any buffers directly
  return GPAC_FILTER_PP_RET_NULL;
}
