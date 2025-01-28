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

#include "lib/properties.h"
#include "lib/session.h"

typedef struct
{
  GPAC_PropertyContext prop;
  GPAC_SessionContext sess;
} GPAC_Context;

/*! initializes a gpac context
    \param[in] ctx the gpac context to initialize
    \return TRUE if the context was initialized successfully, FALSE otherwise
*/
gboolean
gpac_init(GPAC_Context* ctx);

/*! destroys a gpac context
    \param[in] ctx the gpac context to destroy
*/
void
gpac_destroy(GPAC_Context* ctx);
