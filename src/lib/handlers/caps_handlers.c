#include "common.h"
#include "gpacmessages.h"
#include "lib/pid.h"

#define CAPS_HANDLER_SIGNATURE(prop_nickname)                \
  gboolean prop_nickname##_caps_handler(GPAC_PROP_IMPL_ARGS)

#define DEFAULT_HANDLER(prop_nickname)                       \
  gboolean prop_nickname##_caps_handler(GPAC_PROP_IMPL_ARGS) \
  {                                                          \
    return FALSE;                                            \
  }

#define GET_MEDIA_AND_CODEC                                             \
  GstStructure* structure = gst_caps_get_structure(priv->caps, 0);      \
  const gchar* media_type_full = gst_structure_get_name(structure);     \
  g_auto(GStrv) media_type_parts = g_strsplit(media_type_full, "/", 2); \
  gchar* media = media_type_parts[0];                                   \
  gchar* codec = media_type_parts[1];

//
// Default Caps handlers
//
DEFAULT_HANDLER(duration)

//
// Caps handlers
//
CAPS_HANDLER_SIGNATURE(stream_type)
{
  GET_MEDIA_AND_CODEC

  // Get the stream type
  u32 stream_type = gf_stream_type_by_name(media);
  g_return_val_if_fail(stream_type != GF_STREAM_UNKNOWN, FALSE);

  // Check the subtype
  if (!g_strcmp0(media, "video")) {
    const gchar* subtype = gst_structure_get_string(structure, "stream-format");
    if (!g_strcmp0(subtype, "avc") || !g_strcmp0(subtype, "hev1"))
      SET_PROP(GF_PROP_PID_ISOM_SUBTYPE, PROP_STRING(subtype));
  }

  // Set the stream type
  SET_PROP(GF_PROP_PID_STREAM_TYPE, PROP_UINT(stream_type));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(codec_id)
{
  GET_MEDIA_AND_CODEC
  GF_CodecID codec_id = GF_CODECID_NONE;

  // In most cases the substring after "x-" can be used to query the codec id
  if (g_str_has_prefix(codec, "x-")) {
    codec_id = gf_codecid_parse(codec + 2);
    if (codec_id != GF_CODECID_NONE)
      goto finish;
  }

  // See caps.h for the supported formats
  // For the other cases, we will match the codec id manually
  if (!g_strcmp0(media, "audio")) {
    if (!g_strcmp0(codec, "mpeg"))
      codec_id = GF_CODECID_AAC_MPEG4;
  }

  // If we still couldn't determine the codec id, log a warning
  if (codec_id == GF_CODECID_NONE) {
    GST_ERROR("Could not determine codec id for %s/%s", media, codec);
    return FALSE;
  }

finish:
  // Set the codec id
  SET_PROP(GF_PROP_PID_CODECID, PROP_UINT(codec_id));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(unframed)
{
  GET_MEDIA_AND_CODEC

  const gchar* stream_format =
    gst_structure_get_string(structure, "stream-format");

  // Check if the stream is framed
  gboolean framed = TRUE;
  if (!g_strcmp0(media, "video")) {
    if (stream_format) {
      if (!g_strcmp0(stream_format, "byte-stream"))
        framed = FALSE;
      else if (!g_strcmp0(stream_format, "obu-stream"))
        framed = FALSE;
    }
  } else if (!g_strcmp0(media, "audio")) {
    gst_structure_get_boolean(structure, "framed", &framed);
  }

  //* For AVC, HEVC, AV1, etc. our caps always ask for streams with start codes.
  //* So we'll always have unframed data. But for maximum compatibility, we may
  //* allow framed data and explicitly load "unframer" in gpac.

  // Push the caps with the unframed property
  gf_filter_override_caps(gf_filter_pid_get_owner(pid), NULL, 0);
  gf_filter_push_caps(gf_filter_pid_get_owner(pid),
                      GF_PROP_PID_UNFRAMED,
                      &PROP_BOOL(!framed),
                      NULL,
                      GF_CAPS_OUTPUT,
                      0);

  // Set the unframed property
  SET_PROP(GF_PROP_PID_UNFRAMED, PROP_BOOL(!framed));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(width)
{
  GET_MEDIA_AND_CODEC

  // Only process video media
  if (g_strcmp0(media, "video"))
    return TRUE;

  gint width = -1;
  gst_structure_get_int(structure, "width", &width);
  g_return_val_if_fail(width > 0, FALSE);

  // Set the width property
  SET_PROP(GF_PROP_PID_WIDTH, PROP_UINT(width));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(height)
{
  GET_MEDIA_AND_CODEC

  // Only process video media
  if (g_strcmp0(media, "video"))
    return TRUE;

  gint height = -1;
  gst_structure_get_int(structure, "height", &height);
  g_return_val_if_fail(height > 0, FALSE);

  // Set the height property
  SET_PROP(GF_PROP_PID_HEIGHT, PROP_UINT(height));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(sample_rate)
{
  GET_MEDIA_AND_CODEC

  // Only process audio media
  if (g_strcmp0(media, "audio"))
    return TRUE;

  gint rate = -1;
  gst_structure_get_int(structure, "rate", &rate);
  g_return_val_if_fail(rate > 0, FALSE);

  // Set the sample rate property
  SET_PROP(GF_PROP_PID_SAMPLE_RATE, PROP_UINT(rate));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(fps)
{
  GET_MEDIA_AND_CODEC

  // Only process video media
  if (g_strcmp0(media, "video"))
    return TRUE;

  gint num = -1, denom = -1;
  gst_structure_get_fraction(structure, "framerate", &num, &denom);
  if (num < 0 || denom < 0)
    return FALSE;

  // Set the framerate property
  SET_PROP(GF_PROP_PID_FPS, PROP_FRAC_INT(num, denom));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(timescale)
{
  GET_MEDIA_AND_CODEC

  guint64 timescale = 0;
  if (!g_strcmp0(media, "video")) {
    const GF_PropertyValue* p =
      gf_filter_pid_get_property(pid, GF_PROP_PID_FPS);
    if (p)
      timescale = p->value.frac.num;
  } else if (!g_strcmp0(media, "audio")) {
    const GF_PropertyValue* p =
      gf_filter_pid_get_property(pid, GF_PROP_PID_SAMPLE_RATE);
    if (p)
      timescale = p->value.uint;
  } else {
    GST_ERROR("Unsupported media type (%s) for timescale", media);
    g_return_val_if_reached(FALSE);
  }

  if (timescale == 0)
    return FALSE;

  // Set the timescale property
  SET_PROP(GF_PROP_PID_TIMESCALE, PROP_UINT(timescale));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(num_channels)
{
  GET_MEDIA_AND_CODEC

  // Only process audio media
  if (g_strcmp0(media, "audio"))
    return TRUE;

  gint channels = -1;
  gst_structure_get_int(structure, "channels", &channels);

  // Set the number of channels property
  SET_PROP(GF_PROP_PID_NUM_CHANNELS, PROP_UINT(channels));
  return TRUE;
}

CAPS_HANDLER_SIGNATURE(decoder_config)
{
  GstStructure* structure = gst_caps_get_structure(priv->caps, 0);

  // Get the codec data
  const GValue* codec_data = gst_structure_get_value(structure, "codec_data");
  if (!GST_VALUE_HOLDS_BUFFER(codec_data))
    return TRUE;

  GstBuffer* buffer = gst_value_get_buffer(codec_data);
  g_auto(GstBufferMapInfo) map = GST_MAP_INFO_INIT;
  if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    GST_ERROR("Failed to map codec_data buffer");
    return FALSE;
  }

  // Copy the data
  u8* data = (u8*)g_malloc0(map.size);
  memcpy((void*)data, map.data, map.size);

  // Set the decoder config property
  SET_PROP(GF_PROP_PID_DECODER_CONFIG, PROP_CONST_DATA(data, map.size));
  return TRUE;
}
