#include "common.h"
#include "gpacmessages.h"
#include "lib/pid.h"

#define DEFAULT_HANDLER_SIGNATURE(prop_nickname)                           \
  gboolean prop_nickname##_default_handler(GPAC_PROP_IMPL_ARGS_NO_ELEMENT)

// Optional if not explicitly marked as mandatory
#define DEFAULT_HANDLER(prop_nickname)                                     \
  gboolean prop_nickname##_default_handler(GPAC_PROP_IMPL_ARGS_NO_ELEMENT) \
  {                                                                        \
    return TRUE;                                                           \
  }

#define DEFAULT_HANDLER_MANDATORY(prop_nickname, prop_4cc)                 \
  gboolean prop_nickname##_default_handler(GPAC_PROP_IMPL_ARGS_NO_ELEMENT) \
  {                                                                        \
    SKIP_IF_SET(prop_4cc)                                                  \
    GST_WARNING("Could not determine the value for %s", #prop_nickname);   \
    return FALSE;                                                          \
  }

//
// Default Default handlers
//
DEFAULT_HANDLER_MANDATORY(stream_type, GF_PROP_PID_STREAM_TYPE)
DEFAULT_HANDLER_MANDATORY(codec_id, GF_PROP_PID_CODECID)
DEFAULT_HANDLER_MANDATORY(unframed, GF_PROP_PID_UNFRAMED)

// Following are optional
DEFAULT_HANDLER(width)
DEFAULT_HANDLER(height)
DEFAULT_HANDLER(bitrate)
DEFAULT_HANDLER(max_bitrate)
DEFAULT_HANDLER(decoder_config)
DEFAULT_HANDLER(duration)
DEFAULT_HANDLER(fps)
DEFAULT_HANDLER(timescale)
DEFAULT_HANDLER(sample_rate)
DEFAULT_HANDLER(num_channels)
DEFAULT_HANDLER(language)
DEFAULT_HANDLER(dbsize)

//
// Default handlers
//
DEFAULT_HANDLER_SIGNATURE(id)
{
  // Check if we have already set the id
  SKIP_IF_SET(GF_PROP_PID_ID);

  // Set a new monotonic id
  static guint id = 1;
  SET_PROP(GF_PROP_PID_ID, PROP_UINT(id++));
  return TRUE;
}
