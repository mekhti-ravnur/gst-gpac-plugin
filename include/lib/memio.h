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

#include <gpac/filters.h>

#include "lib/filters/filters.h"
#include "lib/session.h"

typedef enum
{
  GPAC_MEMIO_DIR_IN,
  GPAC_MEMIO_DIR_OUT,
} GPAC_MemIoDirection;

typedef struct
{
  // queue should be freed by the caller
  GQueue* queue;
  gboolean eos;
  GPAC_MemIoDirection dir;
  GPAC_SessionContext* sess;

  /*< memout-specific >*/
  guint64 global_offset;
  gboolean is_continuous;
} GPAC_MemIoContext;

typedef struct
{
  post_process_registry_entry* entry;
  void* private_ctx;
} GPAC_MemOutPIDContext;

typedef enum
{
  GPAC_MEMOUT_PID_FLAG_NONE = 0,
  // PID has been initialized
  GPAC_MEMOUT_PID_FLAG_INITIALIZED = (1 << 0),
  // The element should not consume the output of this PID. consume callback
  // will still be called, but the output will be NULL
  GPAC_MEMOUT_PID_FLAG_DONT_CONSUME = (1 << 1),
} GPAC_MemOutPIDFlags;

/*! the memory io filter process callback
    \param[in] filter the filter to process
    \return the result of the operation
*/
typedef GF_Err (*GPAC_MemIoFn)(GF_Filter* filter);

/*! creates a new memory io filter
    \param[in] sess the session context
    \param[in] dir the direction of the memory io filter
    \return the result of the operation
*/
GF_Err
gpac_memio_new(GPAC_SessionContext* sess, GPAC_MemIoDirection dir);

/*! frees a memory io filters
    \param[in] sess the session context
    \note this function will free the runtime user data of both the input and
   output filters
*/
void
gpac_memio_free(GPAC_SessionContext* sess);

/*! assigns a queue to the memory io filter
    \param[in] sess the session context
    \param[in] dir the direction of the memory io filter
    \param[in] queue the queue to assign
*/
void
gpac_memio_assign_queue(GPAC_SessionContext* sess,
                        GPAC_MemIoDirection dir,
                        GQueue* queue);

/*! sets the end of stream flag of the memory input filter
    \param[in] sess the session context
    \param[in] eos the end of stream flag
*/
void
gpac_memio_set_eos(GPAC_SessionContext* sess, gboolean eos);

/*! sets the caps of the memory output filter
    \param[in] sess the session context
    \param[in] caps the gst caps to set
    \return TRUE if the caps were set successfully, FALSE otherwise
*/
gboolean
gpac_memio_set_gst_caps(GPAC_SessionContext* sess, GstCaps* caps);

/*! sets the GF_FilterCapability of the memory output filter
    \param[in] sess the session context
    \param[in] caps the gpac capabilities to set
    \param[in] nb_caps the number of capabilities
    \return TRUE if the capabilities were set successfully, FALSE otherwise
*/
gboolean
gpac_memio_set_gf_caps(GPAC_SessionContext* sess,
                       const GF_FilterCapability* caps,
                       guint nb_caps);

/*! consumes the output of the memory output filter
    \param[in] sess the session context
    \param[out] outptr the output pointer
    \return the result of the operation
*/
GPAC_FilterPPRet
gpac_memio_consume(GPAC_SessionContext* sess, void** outptr);

/*! sets the global offset of the memory output filter
    \param[in] sess the session context
    \param[in] segment the segment to set the offset from
*/
void
gpac_memio_set_global_offset(GPAC_SessionContext* sess,
                             const GstSegment* segment);
