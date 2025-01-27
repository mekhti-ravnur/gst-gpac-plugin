#include "lib/filters/filters.h"
#include "lib/memio.h"

typedef struct
{
  GQueue* output_queue;
} GenericCtx;

void
generic_ctx_init(void** process_ctx)
{
  *process_ctx = g_new0(GenericCtx, 1);
  GenericCtx* ctx = (GenericCtx*)*process_ctx;
  ctx->output_queue = g_queue_new();
}

void
generic_ctx_free(void* process_ctx)
{
  GenericCtx* ctx = (GenericCtx*)process_ctx;

  // Free the output queue
  while (!g_queue_is_empty(ctx->output_queue))
    gst_buffer_unref((GstBuffer*)g_queue_pop_head(ctx->output_queue));
  g_queue_free(ctx->output_queue);

  // Free the context
  g_free(ctx);
}

GF_Err
generic_post_process(GF_Filter* filter, GF_FilterPacket* pck)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  GenericCtx* generic_ctx = (GenericCtx*)ctx->process_ctx;

  // Get the data
  u32 size;
  const u8* data = gf_filter_pck_get_data(pck, &size);
  gf_filter_pck_ref(&pck);

  // Create a new buffer
  GstBuffer* buffer =
    gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,           // flags
                                (u8*)data,                          // data
                                size,                               // maxsize
                                0,                                  // offset
                                size,                               // size
                                pck,                                // user_data
                                (GDestroyNotify)gf_filter_pck_unref // notify
    );

  // Enqueue the buffer
  g_queue_push_tail(generic_ctx->output_queue, buffer);
  return GF_OK;
}

GPAC_FilterPPRet
generic_consume(GF_Filter* filter, void** outptr)
{
  GPAC_MemIoContext* ctx = (GPAC_MemIoContext*)gf_filter_get_rt_udta(filter);
  GenericCtx* generic_ctx = (GenericCtx*)ctx->process_ctx;
  *outptr = NULL;

  // Check if the queue is empty
  if (g_queue_is_empty(generic_ctx->output_queue))
    return GPAC_FILTER_PP_RET_EMPTY;

  // Assign the output
  *outptr = g_queue_pop_head(generic_ctx->output_queue);
  return GPAC_FILTER_PP_RET_BUFFER;
}
