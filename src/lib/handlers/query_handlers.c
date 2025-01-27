#include "common.h"
#include "gpacmessages.h"
#include "lib/pid.h"

#define QUERY_HANDLER_SIGNATURE(prop_nickname)                \
  gboolean prop_nickname##_query_handler(GPAC_PROP_IMPL_ARGS)

#define DEFAULT_HANDLER(prop_nickname)                        \
  gboolean prop_nickname##_query_handler(GPAC_PROP_IMPL_ARGS) \
  {                                                           \
    return FALSE;                                             \
  }

//
// Default Query handlers
//

//
// Query handlers
//
QUERY_HANDLER_SIGNATURE(duration)
{
  const GF_PropertyValue* p;

  // Query the duration
  g_autoptr(GstQuery) query = gst_query_new_duration(GST_FORMAT_TIME);
  gboolean ret = gst_pad_peer_query(priv->self, query);
  g_return_val_if_fail(ret, FALSE);

  // Parse the duration
  gint64 duration;
  gst_query_parse_duration(query, NULL, &duration);

  // Get the fps from the PID
  GF_Fraction fps = { 30, 1 };
  p = gf_filter_pid_get_property(pid, GF_PROP_PID_FPS);
  if (p)
    fps = p->value.frac;

  // Get the timescale from the PID
  guint64 timescale = GST_SECOND;
  p = gf_filter_pid_get_property(pid, GF_PROP_PID_TIMESCALE);
  if (p)
    timescale = p->value.uint;

  // Construct the duration
  GF_Fraction64 duration_fr = {
    .num = gpac_time_rescale_with_fps(duration, fps, timescale),
    .den = timescale,
  };

  // Set the duration
  SET_PROP(GF_PROP_PID_DURATION, PROP_FRAC64(duration_fr));
  return TRUE;
}
