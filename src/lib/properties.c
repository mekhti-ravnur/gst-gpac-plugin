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
#include "lib/properties.h"
#include "gpacmessages.h"
#include <gpac/filters.h>

typedef struct
{
  const gchar* name;
  gboolean is_simple; // if true, treat as "-arg" instead of "--arg=..."
} GPAC_Arg;
#define GPAC_DEF_ARG(name, is_simple) { name, is_simple }

// Define the override properties
static GF_GPACArg override_properties[] = {
  { 0 },
};

// Define the visible properties with their default values
static GPAC_Arg visible_properties[] = {
  GPAC_DEF_ARG("for-test", TRUE),
  GPAC_DEF_ARG("old-arch", TRUE),
  GPAC_DEF_ARG("strict-error", TRUE),
  { 0 },
};

void
gpac_get_property_attributes(GParamSpec* pspec,
                             gboolean* is_simple,
                             gboolean* is_visible)
{
  *is_visible = FALSE;
  *is_simple = FALSE;
  for (u32 i = 0; visible_properties[i].name; i++) {
    if (!g_strcmp0(g_param_spec_get_name(pspec), visible_properties[i].name)) {
      *is_simple = visible_properties[i].is_simple;
      *is_visible = TRUE;
      break;
    }
  }
}

void
gpac_install_local_properties(GObjectClass* gobject_class,
                              GPAC_PropertyId first_prop,
                              ...)
{
  // Initialize the variable argument list
  va_list args;
  GPAC_PropertyId prop = first_prop;
  va_start(args, first_prop);

  while (prop != GPAC_PROP_0) {
    switch (prop) {
      case GPAC_PROP_GRAPH:
        g_object_class_install_property(
          gobject_class,
          prop,
          g_param_spec_string("graph",
                              "Graph",
                              "Filter graph to use in gpac session",
                              NULL,
                              G_PARAM_READWRITE));
        break;

      case GPAC_PROP_NO_OUTPUT:
        g_object_class_install_property(
          gobject_class,
          prop,
          g_param_spec_boolean("no-output",
                               "No Output",
                               "Disable the memory output. Used only when "
                               "instantiating the element within gpacsink",
                               FALSE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        break;

      default:
        break;
    }

    // Get the next property
    prop = va_arg(args, GPAC_PropertyId);
  }

  // End the variable argument list
  va_end(args);
}

void
gpac_install_filter_properties(GObjectClass* gobject_class,
                               const gchar* filter_name)
{
  GF_FilterSession* session = gf_fs_new_defaults(0u);
  guint num_filters = gf_fs_filters_registers_count(session);

  const GF_FilterRegister* filter;
  for (guint i = 0; i < num_filters; i++) {
    filter = gf_fs_get_filter_register(session, i);
    if (!g_strcmp0(filter->name, filter_name))
      break;
    else
      filter = NULL;
  }
  gf_fs_del(session);
  g_assert(filter);

  guint option_idx = 0;
  while (filter->args && filter->args[option_idx].arg_name) {
    // Skip hidden options
    if (filter->args[option_idx].flags & GF_ARG_HINT_HIDE)
      goto skip;

    // Check if the option name is valid
    if (!g_param_spec_is_valid_name(filter->args[option_idx].arg_name))
      goto skip;

    // Check if the option is already registered
    if (g_object_class_find_property(gobject_class,
                                     filter->args[option_idx].arg_name))
      goto skip;

#define SPEC_INSTALL(type, ...)                                      \
  g_object_class_install_property(                                   \
    gobject_class,                                                   \
    GPAC_PROP_FILTER_OFFSET + option_idx,                            \
    g_param_spec_##type(filter->args[option_idx].arg_name,           \
                        filter->args[option_idx].arg_name,           \
                        filter->args[option_idx].arg_desc,           \
                        __VA_ARGS__,                                 \
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

    // Special case for enum values
    if (filter->args[option_idx].min_max_enum &&
        strstr(filter->args[option_idx].min_max_enum, "|")) {
      SPEC_INSTALL(string, NULL);
      goto skip;
    }

    // Register the option
    switch (filter->args[option_idx].arg_type) {
      case GF_PROP_SINT:
        SPEC_INSTALL(int, 0, G_MAXINT, 0);
        break;
      case GF_PROP_UINT:
        SPEC_INSTALL(uint, 0, G_MAXUINT, 0);
        break;
      case GF_PROP_LSINT:
        SPEC_INSTALL(int64, 0, G_MAXINT64, 0);
        break;
      case GF_PROP_LUINT:
        SPEC_INSTALL(uint64, 0, G_MAXUINT64, 0);
        break;
      case GF_PROP_FLOAT:
        SPEC_INSTALL(float, 0.0, G_MAXFLOAT, 0.0);
        break;
      case GF_PROP_DOUBLE:
        SPEC_INSTALL(double, 0.0, G_MAXDOUBLE, 0.0);
        break;
      case GF_PROP_BOOL:
        SPEC_INSTALL(boolean, FALSE);
        break;
      case GF_PROP_STRING:
        SPEC_INSTALL(string, filter->args[option_idx].arg_default_val);
        break;
      default:
        SPEC_INSTALL(string, NULL);
        break;
    }

#undef SPEC_INSTALL

  skip:
    option_idx++;
  }
}

void
gpac_install_global_properties(GObjectClass* gobject_class)
{
  // Install the visible global properties
  const GF_GPACArg* args = gf_sys_get_options();
  for (u32 i = GPAC_PROP_GLOBAL_OFFSET;
       visible_properties[i - GPAC_PROP_GLOBAL_OFFSET].name;
       i++) {
    // Find the property in the global properties
    for (u32 j = 0; args[j].name; j++) {
      const GF_GPACArg* arg = &args[j];
      const GPAC_Arg* prop = &visible_properties[i - GPAC_PROP_GLOBAL_OFFSET];
      if (!g_strcmp0(prop->name, arg->name)) {
        g_object_class_install_property(
          gobject_class,
          i,
          prop->is_simple ? g_param_spec_boolean(arg->name,
                                                 arg->name,
                                                 arg->description,
                                                 FALSE,
                                                 G_PARAM_READWRITE)
                          : g_param_spec_string(arg->name,
                                                arg->name,
                                                arg->description,
                                                NULL,
                                                G_PARAM_READWRITE));
        break;
      }
    }
  }
}

gboolean
gpac_set_property(GPAC_PropertyContext* ctx,
                  guint property_id,
                  const GValue* value,
                  GParamSpec* pspec)
{
  if (!ctx->properties)
    ctx->properties = gf_list_new();

  if (property_id < GPAC_PROP_FILTER_OFFSET) {
    switch (property_id) {
      case GPAC_PROP_GRAPH:
        g_free(ctx->graph);
        ctx->graph = g_value_dup_string(value);
        break;
      case GPAC_PROP_NO_OUTPUT:
        ctx->no_output = g_value_get_boolean(value);
        break;
      default:
        return FALSE;
    }
  } else if (property_id >= GPAC_PROP_FILTER_OFFSET &&
             property_id < GPAC_PROP_GLOBAL_OFFSET) {
    // Get the value as a string
    gchar* value_str;
    if (G_VALUE_HOLDS_STRING(value))
      value_str = g_value_dup_string(value);
    else if (G_VALUE_HOLDS_BOOLEAN(value))
      value_str = g_strdup(g_value_get_boolean(value) ? "true" : "false");
    else
      value_str = g_strdup_value_contents(value);

    // No value, return
    if (!value_str)
      return TRUE;

    // Add the property to the list
    gchar* property =
      g_strdup_printf("--%s=%s", g_param_spec_get_name(pspec), value_str);
    gf_list_add(ctx->properties, property);
    g_free(value_str);
    return TRUE;
  } else {
    // Check if the property is visible and if it is simple
    gboolean is_simple, is_visible;
    gpac_get_property_attributes(pspec, &is_simple, &is_visible);

    // If the property is not visible, return
    if (!is_visible)
      return FALSE;

    // Add the property to the list
    if (is_simple) {
      gchar* property = g_strdup_printf("-%s", g_param_spec_get_name(pspec));
      gf_list_add(ctx->properties, property);
    } else {
      gchar* property = g_strdup_printf(
        "--%s=%s", g_param_spec_get_name(pspec), g_value_get_string(value));
      gf_list_add(ctx->properties, property);
    }
  }
  return TRUE;
}

gboolean
gpac_get_property(GPAC_PropertyContext* ctx,
                  guint property_id,
                  GValue* value,
                  GParamSpec* pspec)
{
  if (property_id < GPAC_PROP_FILTER_OFFSET) {
    switch (property_id) {
      case GPAC_PROP_GRAPH:
        g_value_set_string(value, ctx->graph);
        break;
      case GPAC_PROP_NO_OUTPUT:
        g_value_set_boolean(value, ctx->no_output);
        break;
      default:
        return FALSE;
    }
  } else if (property_id >= GPAC_PROP_GLOBAL_OFFSET) {
    gboolean is_simple, is_visible;
    gpac_get_property_attributes(pspec, &is_simple, &is_visible);
    g_assert(is_visible);
    const gchar* prop_val =
      gf_sys_find_global_arg(g_param_spec_get_name(pspec));

    if (is_simple)
      g_value_set_boolean(value, prop_val != NULL);
    else
      g_value_set_string(value, prop_val);
  } else
    g_warn_if_reached();
  return TRUE;
}

gboolean
gpac_apply_properties(GPAC_PropertyContext* ctx)
{
  if (!ctx->properties)
    ctx->properties = gf_list_new();

  // Insert a dummy "executable" name
  gf_list_insert(ctx->properties, gf_strdup("gst"), 0);

  // Add the default properties
  for (u32 i = 0; override_properties[i].name; i++) {
    const GF_GPACArg* arg = &override_properties[i];
    gchar* property = g_strdup_printf("--%s=%s", arg->name, arg->val);
    gf_list_add(ctx->properties, property);
  }

  ctx->props_as_argv =
    (gchar**)g_malloc0_n(gf_list_count(ctx->properties), sizeof(gchar*));
  void* item;
  for (u32 i = 0; (item = gf_list_enum(ctx->properties, &i));)
    ctx->props_as_argv[i - 1] = (gchar*)item;

  // Set the gpac system arguments
  gpac_return_val_if_fail(gf_sys_set_args((s32)gf_list_count(ctx->properties),
                                          (const char**)ctx->props_as_argv),
                          FALSE);

  return TRUE;
}
