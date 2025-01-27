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
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public
 *  License along with this library; see the file LICENSE.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "lib/time.h"

guint64
gpac_time_rescale_with_fps(GstClockTime time,
                           GF_Fraction fps,
                           guint64 desired_timescale)
{
  if (!GST_CLOCK_TIME_IS_VALID(time) || !desired_timescale)
    return 0;

  // Timescale is already in the desired timescale
  if (GST_SECOND == desired_timescale)
    return time;

  // Calculate the frame duration in GST_SECOND
  guint64 frame_duration = gf_timestamp_rescale(GST_SECOND, fps.num, fps.den);

  // Convert the given time to the desired timescale
  guint64 frame_tick =
    gf_timestamp_rescale(desired_timescale, fps.num, fps.den);
  guint64 rescaled_time =
    gf_timestamp_rescale(time, frame_duration, frame_tick);

  return rescaled_time;
}
