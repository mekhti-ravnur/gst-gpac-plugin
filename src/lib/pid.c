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

#include "lib/pid.h"
#include "conversion/pid/registry.h"
#include "gpacmessages.h"

gboolean
gpac_pid_apply_overrides(GPAC_PID_PROP_IMPL_ARGS_NO_ELEMENT, GList** to_skip)
{
  GstStructure* caps = gst_caps_get_structure(priv->caps, 0);
  gchar* prefix = "gpac-";
  gboolean nested = FALSE;

retry:
  // Go through the root structure
  for (gint i = 0; i < gst_structure_n_fields(caps); i++) {
    const gchar* field_name = gst_structure_nth_field_name(caps, i);

    // Check if the field name starts with the prefix
    if (g_str_has_prefix(field_name, prefix)) {
      GType type = gst_structure_get_field_type(caps, field_name);
      if (type != G_TYPE_STRING) {
        GST_WARNING_OBJECT(
          priv->self,
          "Property overrides for GPAC must be strings, %s is %s",
          field_name,
          g_type_name(type));
        continue;
      }

      const gchar* prop_name = field_name + strlen(prefix);
      u32 prop_4cc = gf_props_get_id(prop_name);

      // Parse the property value
      const GValue* value = gst_structure_get_value(caps, field_name);
      const gchar* prop_value = g_value_get_string(value);

      // Check if the property value is valid
      if (prop_value == NULL || prop_value[0] == '\0') {
        GST_WARNING_OBJECT(priv->self, "Empty value for %s", prop_name);
        continue;
      }

      // Set the property
      GF_PropertyValue pv = gf_props_parse_value(
        gf_props_4cc_get_type(prop_4cc), prop_name, prop_value, NULL, ',');
      gpac_return_val_if_fail(gf_filter_pid_set_property(pid, prop_4cc, &pv),
                              FALSE);

      // Add the property to the to_skip list
      *to_skip = g_list_append(*to_skip, GUINT_TO_POINTER(prop_4cc));
    }
  }

  // If we have also processed nested structure, finish
  if (nested)
    goto finish;

  // Check if we have nested "gpac" structure
  if (gst_structure_has_field(caps, "gpac")) {
    if (!gst_structure_get(caps, "gpac", GST_TYPE_STRUCTURE, &caps, NULL)) {
      GST_ERROR_OBJECT(priv->self, "Failed to get nested gpac structure");
      return FALSE;
    }
    prefix = "";
    nested = TRUE;
    goto retry;
  }

finish:
  return TRUE;
}

gboolean
gpac_pid_reconfigure(GPAC_PID_PROP_IMPL_ARGS)
{
  // Check arguments
  g_return_val_if_fail(element != NULL, FALSE);
  g_return_val_if_fail(priv != NULL, FALSE);
  g_return_val_if_fail(pid != NULL, FALSE);

  // Lock the element
  GST_OBJECT_AUTO_LOCK(element, auto_lock);

  // Go through overrides if caps are set
  g_autoptr(GList) to_skip = g_list_alloc();
  if (HAS_FLAG(priv->flags, GPAC_PAD_CAPS_SET)) {
    if (!gpac_pid_apply_overrides(priv, pid, &to_skip)) {
      GST_ERROR_OBJECT(priv->self, "Failed to apply overrides");
      return FALSE;
    }
  }

  // Go through the property registry
  for (u32 i = 0; i < gpac_pid_get_num_supported_props(); i++) {
    prop_registry_entry* entry = &prop_registry[i];

    // Skip the property if it is in the to_skip list
    if (g_list_find(to_skip, GUINT_TO_POINTER(entry->prop_4cc)))
      continue;

    // Try each handler
    if (entry->caps_handler != NULL)
      if (HAS_FLAG(priv->flags, GPAC_PAD_CAPS_SET) &&
          entry->caps_handler(element, priv, pid))
        continue;

    if (entry->tags_handler != NULL)
      if (HAS_FLAG(priv->flags, GPAC_PAD_TAGS_SET) &&
          entry->tags_handler(element, priv, pid))
        continue;

    if (entry->segment_handler != NULL)
      if (HAS_FLAG(priv->flags, GPAC_PAD_SEGMENT_SET) &&
          entry->segment_handler(element, priv, pid))
        continue;

    if (entry->query_handler != NULL)
      if (entry->query_handler(element, priv, pid))
        continue;

    if (entry->default_handler != NULL)
      if (entry->default_handler(priv, pid))
        continue;

    // If there were no flag set, we may not have all the information to
    // properly set the property
    if (priv->flags == 0)
      continue;

    // No handler was able to handle the property
    GST_ERROR_OBJECT(
      priv->self,
      "None of the handlers of %s (%s) were able to set the property",
      gf_props_4cc_get_name(entry->prop_4cc),
      gf_4cc_to_str(entry->prop_4cc));
    return FALSE;
  }

  return TRUE;
}

GF_FilterPid*
gpac_pid_new(GPAC_SessionContext* sess)
{
  GF_FilterPid* pid = gf_filter_pid_new(sess->memin);
  if (!pid) {
    GST_ELEMENT_ERROR(
      sess->element, LIBRARY, FAILED, (NULL), ("Failed to create new PID"));
    return NULL;
  }
  return pid;
}

void
gpac_pid_del(GF_FilterPid* pid)
{
  gf_filter_pid_remove(pid);
}
