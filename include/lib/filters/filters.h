#pragma once

#include <gpac/filters.h>
#include <gst/gst.h>

typedef enum
{
  GPAC_FILTER_PP_RET_VALID = 1,

  // Return types that do not result in a buffer
  GPAC_FILTER_PP_RET_EMPTY = ((1 << 1)),
  GPAC_FILTER_PP_RET_ERROR = ((1 << 2)),

  // Return types that result in a buffer
  GPAC_FILTER_PP_RET_NULL = ((1 << 3) | GPAC_FILTER_PP_RET_VALID),
  GPAC_FILTER_PP_RET_BUFFER = ((1 << 4) | GPAC_FILTER_PP_RET_VALID),
  GPAC_FILTER_PP_RET_BUFFER_LIST = ((1 << 5) | GPAC_FILTER_PP_RET_VALID),
} GPAC_FilterPPRet;

#define GPAC_FILTER_PP_IMPL_DECL(filter_name)                                 \
  void filter_name##_ctx_init(void** process_ctx);                            \
  void filter_name##_ctx_free(void* process_ctx);                             \
  GF_Err filter_name##_post_process(GF_Filter* filter, GF_FilterPacket* pck); \
  GPAC_FilterPPRet filter_name##_consume(GF_Filter* filter, void** outptr);

#define GPAC_FILTER_PP_IMPL_DEFINE(filter_name) \
  { #filter_name,                               \
    filter_name##_ctx_init,                     \
    filter_name##_post_process,                 \
    filter_name##_consume,                      \
    filter_name##_ctx_free }

// Forward declarations
GPAC_FILTER_PP_IMPL_DECL(generic);
GPAC_FILTER_PP_IMPL_DECL(mp4mx);

typedef struct
{
  const gchar* filter_name;

  // Handlers
  void (*ctx_init)(void** process_ctx);
  GF_Err (*post_process)(GF_Filter* filter, GF_FilterPacket* pck);
  GPAC_FilterPPRet (*consume)(GF_Filter* filter, void** outptr);
  void (*ctx_free)(void* process_ctx);
} post_process_registry_entry;

static post_process_registry_entry pp_registry[] = {
  GPAC_FILTER_PP_IMPL_DEFINE(generic),
  GPAC_FILTER_PP_IMPL_DEFINE(mp4mx),
};

static inline u32
gpac_filter_get_num_supported_post_process()
{
  return G_N_ELEMENTS(pp_registry);
}

static inline post_process_registry_entry*
gpac_filter_get_post_process_registry_entry(const gchar* filter_name)
{
  for (u32 i = 0; i < gpac_filter_get_num_supported_post_process(); i++) {
    if (g_strcmp0(pp_registry[i].filter_name, filter_name) == 0) {
      return &pp_registry[i];
    }
  }

  // Not found, return the generic post-process handler
  return &pp_registry[0];
}
