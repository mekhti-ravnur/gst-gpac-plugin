#pragma once

#include <glib.h>
#include <gpac/tools.h>

#define GPAC_LOCAL_ERROR GF_Err gpac_error = GF_OK;
#define GPAC_ERROR_START(expr)                                     \
  const gchar* gpac_msg =                                          \
    g_strconcat(expr, ": ", gf_error_to_string(gpac_error), NULL);
#define GPAC_ERROR_END g_free((gpointer)gpac_msg);

#define gpac_return_if_fail(expr)                                  \
  G_STMT_START                                                     \
  {                                                                \
    GPAC_LOCAL_ERROR                                               \
    if (G_LIKELY((gpac_error = expr) == GF_OK)) {                  \
    } else {                                                       \
      GPAC_ERROR_START(#expr)                                      \
      g_return_if_fail_warning(G_LOG_DOMAIN, G_STRFUNC, gpac_msg); \
      GPAC_ERROR_END                                               \
      return (gpac_error);                                         \
    }                                                              \
  }                                                                \
  G_STMT_END

#define gpac_return_val_if_fail(expr, val)                         \
  G_STMT_START                                                     \
  {                                                                \
    GPAC_LOCAL_ERROR                                               \
    if (G_LIKELY((gpac_error = expr) == GF_OK)) {                  \
    } else {                                                       \
      GPAC_ERROR_START(#expr)                                      \
      g_return_if_fail_warning(G_LOG_DOMAIN, G_STRFUNC, gpac_msg); \
      GPAC_ERROR_END                                               \
      return (val);                                                \
    }                                                              \
  }                                                                \
  G_STMT_END
