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
#include <gst/gst.h>

#include <gio/gio.h>
#include <glib.h>

// Signal Registry
typedef enum
{
  GPAC_SIGNAL_0 = 0,

  // Dasher signals
  GPAC_SIGNAL_DASHER_MANIFEST,
  GPAC_SIGNAL_DASHER_MANIFEST_VARIANT,
  GPAC_SIGNAL_DASHER_INIT_SEGMENT,
  GPAC_SIGNAL_DASHER_SEGMENT,

  // Accessors
  GPAC_SIGNAL_START = GPAC_SIGNAL_DASHER_MANIFEST,
  GPAC_SIGNAL_END = GPAC_SIGNAL_DASHER_SEGMENT,
  GPAC_SIGNAL_LAST = GPAC_SIGNAL_END + 1,
} GPAC_SignalId;

// Signal Names
// Starts from GPAC_SIGNAL_START and ends at GPAC_SIGNAL_END
static const gchar* gpac_signal_names[] = {
  "gpac-signal-dasher-manifest",
  "gpac-signal-dasher-manifest-variant",
  "gpac-signal-dasher-init-segment",
  "gpac-signal-dasher-segment",
};

/*! installs the signals to the GObject class
    \param[in] gobject_class the GObject class to install the signals to
    \param[in] presets a comma-separated list of signal presets to install
    \note The preset names are defined in signals.c
*/
void
gpac_install_signals_by_presets(GObjectClass* gobject_class,
                                const gchar* presets);

/*! installs all signals to the GObject class
    \param[in] gobject_class the GObject class to install the signals to
*/
void
gpac_install_all_signals(GObjectClass* gobject_class);

/*! tries to emit a signal and returns the output stream
    \param[in] element the GstElement to emit the signal on
    \param[in] id the signal ID to emit
    \param[in] location the location string to pass to the signal
    \return the GOutputStream if the signal was emitted, NULL otherwise
    \note The signal must be registered before calling this function
*/
GOutputStream*
gpac_signal_try_emit(GstElement* element,
                     GPAC_SignalId id,
                     const gchar* location);
