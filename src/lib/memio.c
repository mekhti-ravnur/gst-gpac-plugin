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
#include "lib/memio.h"
#include "gpacmessages.h"
#include "lib/caps.h"
#include "lib/pid.h"
#include <gst/video/video-event.h>

static GF_Err
gpac_default_memin_process_cb(GF_Filter* filter);

static Bool
gpac_default_memin_process_event(GF_Filter* filter, const GF_FilterEvent* evt);

static GF_Err
gpac_default_memout_process_cb(GF_Filter* filter);

static GF_Err
gpac_default_memout_configure_pid_cb(GF_Filter* filter,
                                     GF_FilterPid* PID,
                                     Bool is_remove);

static const GF_FilterCapability DefaultMemOutCaps[] = {
  CAP_UINT(GF_CAPS_INPUT, GF_PROP_PID_STREAM_TYPE, GF_STREAM_FILE),
  CAP_STRING(GF_CAPS_INPUT, GF_PROP_PID_FILE_EXT, "*"),
};

GF_FilterRegister MemOutRegister = {
  .name = "memout",
  // Set to (u32) -1 once we add support for multiple "src" pads
  .max_extra_pids = 0,
  .priority = -1,
  .flags = GF_FS_REG_FORCE_REMUX,
  .caps = DefaultMemOutCaps,
  .nb_caps = G_N_ELEMENTS(DefaultMemOutCaps),
  .process = gpac_default_memout_process_cb,
  .configure_pid = gpac_default_memout_configure_pid_cb,
};

GF_Err
gpac_memio_new(GPAC_SessionContext* sess, GPAC_MemIoDirection dir)
{
  GF_Err e = GF_OK;
  GF_Filter* memio = NULL;

  if (dir == GPAC_MEMIO_DIR_IN) {
    memio = sess->memin = gf_fs_new_filter(sess->session, "memin", 0, &e);
    if (!sess->memin) {
      GST_ELEMENT_ERROR(sess->element,
                        LIBRARY,
                        INIT,
                        ("Failed to create memin filter"),
                        (NULL));
      return e;
    }
    gf_filter_set_process_ckb(memio, gpac_default_memin_process_cb);
    gf_filter_set_process_event_ckb(memio, gpac_default_memin_process_event);
  } else {
    gf_fs_add_filter_register(sess->session, &MemOutRegister);
    memio = sess->memout = gf_fs_load_filter(sess->session, "memout", &e);
    if (!sess->memout) {
      GST_ELEMENT_ERROR(
        sess->element, LIBRARY, INIT, ("Failed to load memout filter"), (NULL));
      return e;
    }
  }

  // Set the runtime user data
  GPAC_MemIoContext* rt_udta = g_new0(GPAC_MemIoContext, 1);
  if (!rt_udta) {
    GST_ELEMENT_ERROR(sess->element,
                      LIBRARY,
                      INIT,
                      ("Failed to allocate memory for runtime user data"),
                      (NULL));
    return GF_OUT_OF_MEM;
  }
  gpac_return_if_fail(gf_filter_set_rt_udta(memio, rt_udta));
  rt_udta->dir = dir;
  rt_udta->global_offset = GST_CLOCK_TIME_NONE;

  return e;
}

void
gpac_memio_free(GPAC_SessionContext* sess)
{
  if (sess->memin)
    gf_free(gf_filter_get_rt_udta(sess->memin));

  if (sess->memout)
    gf_free(gf_filter_get_rt_udta(sess->memout));
}

void
gpac_memio_assign_queue(GPAC_SessionContext* sess,
                        GPAC_MemIoDirection dir,
                        GQueue* queue)
{
  GPAC_MemIoContext* rt_udta = gf_filter_get_rt_udta(
    dir == GPAC_MEMIO_DIR_IN ? sess->memin : sess->memout);
  if (!rt_udta) {
    GST_ELEMENT_ERROR(sess->element,
                      LIBRARY,
                      FAILED,
                      ("Failed to get runtime user data"),
                      (NULL));
    return;
  }
  rt_udta->queue = queue;
}

void
gpac_memio_set_eos(GPAC_SessionContext* sess, gboolean eos)
{
  if (!sess->memin)
    return;

  GPAC_MemIoContext* rt_udta = gf_filter_get_rt_udta(sess->memin);
  if (!rt_udta) {
    GST_ELEMENT_ERROR(sess->element,
                      LIBRARY,
                      FAILED,
                      ("Failed to get runtime user data"),
                      (NULL));
    return;
  }
  rt_udta->eos = eos;
}

gboolean
gpac_memio_set_caps(GPAC_SessionContext* sess, GstCaps* caps)
{
  if (!sess->memout)
    return TRUE;

  // Save the current caps
  guint cur_nb_caps = 0;
  const GF_FilterCapability* current_caps =
    gf_filter_get_caps(sess->memout, &cur_nb_caps);

  // Convert the caps to GF_FilterCapability
  guint new_nb_caps = 0;
  GF_FilterCapability* gf_caps = gpac_gstcaps_to_gfcaps(caps, &new_nb_caps);
  if (!gf_caps) {
    GST_ELEMENT_ERROR(sess->element,
                      STREAM,
                      FAILED,
                      ("Failed to convert the caps to GF_FilterCapability"),
                      (NULL));
    return FALSE;
  }

  // Set the capabilities
  if (gf_filter_override_caps(sess->memout, gf_caps, new_nb_caps) != GF_OK) {
    GST_ELEMENT_ERROR(sess->element,
                      STREAM,
                      FAILED,
                      ("Failed to set the caps on the memory output filter, "
                       "reverting to the previous caps"),
                      (NULL));
    gf_free(gf_caps);
    gf_filter_override_caps(sess->memout, current_caps, cur_nb_caps);
    return FALSE;
  }

  // Reconnect the pipeline
  u32 count = gf_fs_get_filters_count(sess->session);
  for (u32 i = 0; i < count; i++) {
    GF_Filter* filter = gf_fs_get_filter(sess->session, i);
    gf_filter_reconnect_output(filter, NULL);
  }

  return TRUE;
}

GPAC_FilterPPRet
gpac_memio_consume(GPAC_SessionContext* sess, void** outptr)
{
  if (!sess->memout)
    return GPAC_FILTER_PP_RET_NULL;

  GPAC_MemIoContext* ctx = gf_filter_get_rt_udta(sess->memout);
  if (!ctx)
    return GPAC_FILTER_PP_RET_ERROR;

  // Check if we have a input PID yet
  if (!ctx->ipid)
    return GPAC_FILTER_PP_RET_EMPTY;

  // Get the post-process context
  const gchar* source_name = gf_filter_pid_get_filter_name(ctx->ipid);
  post_process_registry_entry* pp_entry =
    gpac_filter_get_post_process_registry_entry(source_name);

  // Consume the packet
  return pp_entry->consume(sess->memout, outptr);
}

void
gpac_memio_set_global_offset(GPAC_SessionContext* sess,
                             const GstSegment* segment)
{
  if (!sess->memout)
    return;

  GPAC_MemIoContext* ctx = gf_filter_get_rt_udta(sess->memout);
  if (!ctx) {
    GST_ELEMENT_ERROR(sess->element,
                      LIBRARY,
                      FAILED,
                      ("Failed to get runtime user data"),
                      (NULL));
    return;
  }

  // Get the segment offset
  guint64 offset = segment->base + segment->start + segment->offset;
  if (offset == GST_CLOCK_TIME_NONE)
    return;
  if (ctx->global_offset == GST_CLOCK_TIME_NONE || !ctx->is_continuous) {
    ctx->global_offset = MIN(offset, ctx->global_offset);
  } else if (ctx->global_offset > offset) {
    GST_ELEMENT_WARNING(
      sess->element,
      STREAM,
      FAILED,
      ("Cannot set a global offset smaller than the current one"),
      (NULL));
  }
}

//////////////////////////////////////////////////////////////////////////
// #MARK: Default Callbacks
//////////////////////////////////////////////////////////////////////////
static GF_Err
gpac_default_memin_process_cb(GF_Filter* filter)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);

  // Flush the queue
  GF_FilterPacket* packet = NULL;
  while ((packet = g_queue_pop_head(ctx->queue)))
    gf_filter_pck_send(packet);

  // All packets are sent, check if the EOS is set
  if (ctx->eos) {
    // Set the EOS on all output PIDs
    for (guint i = 0; i < gf_filter_get_opid_count(filter); i++)
      gf_filter_pid_set_eos(gf_filter_get_opid(filter, i));
    return GF_EOS;
  }

  return GF_OK;
}

static Bool
gpac_default_memin_process_event(GF_Filter* filter, const GF_FilterEvent* evt)
{
  if (evt->base.type == GF_FEVT_ENCODE_HINTS) {
    GF_FilterPid* pid = evt->base.on_pid;
    GF_Fraction intra_period = evt->encode_hints.intra_period;
    GpacPadPrivate* priv = gf_filter_pid_get_udta(pid);
    GstElement* element = gst_pad_get_parent_element(priv->self);

    // Set the IDR period
    priv->idr_period =
      gf_timestamp_rescale(intra_period.num, intra_period.den, GST_SECOND);

    // Determine the next IDR frame
    if (priv->idr_last != GST_CLOCK_TIME_NONE) {
      priv->idr_next = priv->idr_last + priv->idr_period;

      // Request a new IDR, immediately
      GstEvent* gst_event =
        gst_video_event_new_upstream_force_key_unit(priv->idr_next, TRUE, 1);
      if (!gst_pad_push_event(priv->self, gst_event)) {
        GST_ELEMENT_WARNING(element,
                            STREAM,
                            FAILED,
                            ("Failed to push the force key unit event"),
                            (NULL));
      }
    }

    return GF_TRUE;
  }
  return GF_FALSE;
}

static GF_Err
gpac_default_memout_process_cb(GF_Filter* filter)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);

  // Get the packet
  GF_FilterPacket* pck = gf_filter_pid_get_packet(ctx->ipid);
  if (!pck)
    return GF_OK;

  // Get the post-process context
  const gchar* source_name = gf_filter_pid_get_filter_name(ctx->ipid);
  post_process_registry_entry* pp_entry =
    gpac_filter_get_post_process_registry_entry(source_name);

  // Post-process the packet
  GF_Err e = pp_entry->post_process(filter, pck);
  gf_filter_pid_drop_packet(ctx->ipid);
  return e;
}

static GF_Err
gpac_default_memout_configure_pid_cb(GF_Filter* filter,
                                     GF_FilterPid* pid,
                                     Bool is_remove)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);

  // Free the previous post-process context
  if (ctx->ipid) {
    const gchar* source_name = gf_filter_pid_get_filter_name(ctx->ipid);
    post_process_registry_entry* pp_entry =
      gpac_filter_get_post_process_registry_entry(source_name);
    pp_entry->ctx_free(ctx->process_ctx);
  }

  if (!ctx->ipid) {
    GF_FilterEvent evt;
    gf_filter_pid_init_play_event(pid, &evt, 0, 1, "MemOut");
    gf_filter_pid_send_event(pid, &evt);
  }

  // Set the new PID
  ctx->ipid = pid;

  // Create a new post-process context
  const gchar* source_name = gf_filter_pid_get_filter_name(ctx->ipid);
  post_process_registry_entry* pp_entry =
    gpac_filter_get_post_process_registry_entry(source_name);
  pp_entry->ctx_init(&ctx->process_ctx);

  // Configure the PID with the new post-process context
  return pp_entry->configure_pid(filter, pid);
}
