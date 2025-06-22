#include "helper/common.hpp"

typedef std::pair<guint32, guint32> fourcc_t;

auto
extract_box_fourccs(GstBuffer* buffer,
                    std::vector<fourcc_t>& fourccs,
                    guint32* leftover = NULL) -> bool
{
  GstBufferMapInfo map_info;
  if (!gst_buffer_map(buffer, &map_info, GST_MAP_READ))
    return false;

  guint8* data = map_info.data;
  guint32 const size = map_info.size;
  guint32 offset = 0;

  while (offset < size) {
    guint32 const box_size =
      GUINT32_FROM_LE((data[offset] << 24) | (data[offset + 1] << 16) |
                      (data[offset + 2] << 8) | data[offset + 3]);
    guint32 const box_type =
      GUINT32_FROM_BE((data[offset + 4] << 24) | (data[offset + 5] << 16) |
                      (data[offset + 6] << 8) | data[offset + 7]);
    fourcc_t const fourcc = std::make_pair(box_type, box_size);
    fourccs.push_back(fourcc);
    offset += box_size;
  }

  gst_buffer_unmap(buffer, &map_info);
  if (leftover)
    *leftover = offset - size;
  return true;
}

void
IsSegmentInit(GstBuffer* buffer)
{
  // It must only have the following flags
  EXPECT_TRUE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_HEADER));
  EXPECT_TRUE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT));

  // This is not a delta unit
  EXPECT_FALSE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT));

  // It must contain also contain at least "ftyp" and "moov" boxes
  std::vector<fourcc_t> fourccs;
  extract_box_fourccs(buffer, fourccs);

  bool has_ftyp = false;
  bool has_moov = false;
  for (fourcc_t const fourcc : fourccs) {
    guint32 const fourcc_type = fourcc.first;
    if (fourcc_type == GST_MAKE_FOURCC('f', 't', 'y', 'p'))
      has_ftyp = true;
    if (fourcc_type == GST_MAKE_FOURCC('m', 'o', 'o', 'v'))
      has_moov = true;
  }

  EXPECT_TRUE(has_ftyp);
  EXPECT_TRUE(has_moov);
}

auto
IsSegmentHeader(GstBuffer* buffer, bool is_independent = false) -> guint32
{
  // It must only have the following flags
  if (is_independent)
    EXPECT_FALSE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT));
  else
    EXPECT_TRUE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT));
  EXPECT_TRUE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_HEADER));

  // It must contain also contain at least "ftyp" and "moov" boxes
  guint32 leftover;
  std::vector<fourcc_t> fourccs;
  extract_box_fourccs(buffer, fourccs, &leftover);

  bool has_moof = false;
  bool has_mdat = false;
  for (fourcc_t const fourcc : fourccs) {
    guint32 const fourcc_type = fourcc.first;
    guint32 const box_size = fourcc.second;
    if (fourcc_type == GST_MAKE_FOURCC('m', 'o', 'o', 'f'))
      has_moof = true;
    if (fourcc_type == GST_MAKE_FOURCC('m', 'd', 'a', 't')) {
      has_mdat = true;

      // only size+fourcc is inside the header
      EXPECT_EQ(box_size - 8, leftover);
    }
  }

  EXPECT_TRUE(has_moof);
  EXPECT_TRUE(has_mdat);
  return leftover;
}

auto
IsSegmentData(GstBuffer* buffer,
              guint32 expected_segment_size,
              bool is_independent = false) -> guint32
{
  // It must not exceed the expected segment size
  guint32 const buffer_size = gst_buffer_get_size(buffer);
  EXPECT_LE(buffer_size, expected_segment_size);
  guint32 const leftover = expected_segment_size - buffer_size;

  // It must only have the following flags
  if (leftover == 0) {
    EXPECT_TRUE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_MARKER));
  }

  // Data buffers are always delta units
  EXPECT_TRUE(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT));

  // cmafmux sends buffers partially, so we need to check the leftover
  return leftover;
}

TEST_F(GstTestFixture, StructureTest)
{
  // Set up the pipeline
  this->SetUpPipeline({ true, "x264enc" });

  // Create test elements
  GstElement* cmafmux = gst_element_factory_make_full("cmafmux",
                                                      "chunk-duration",
                                                      GST_SECOND,
                                                      "fragment-duration",
                                                      5 * GST_SECOND,
                                                      NULL);
  GstElement* gpaccmafmux = gst_element_factory_make_full(
    "gpaccmafmux", "cdur", 1.0, "segdur", 5.0, NULL);

  // Disable B-frames
  g_object_set(GetEncoder(), "b-adapt", FALSE, "bframes", 0, NULL);

  // Create element sinks
  GstAppSink* cmafmux_sink = new GstAppSink(cmafmux, tee, pipeline);
  GstAppSink* gpaccmafmux_sink = new GstAppSink(gpaccmafmux, tee, pipeline);

  // Start the pipeline
  this->StartPipeline();

  // Go through all buffers
  int segment_count = 0;
  while (true) {
    GstBufferList* cmaf_buffer = cmafmux_sink->PopBuffer();
    GstBufferList* gpaccmaf_buffer = gpaccmafmux_sink->PopBuffer();

    // If neither buffer is available, we are done
    if (!cmaf_buffer && !gpaccmaf_buffer)
      break;

    // Both buffers should return something
    ASSERT_TRUE(cmaf_buffer && gpaccmaf_buffer);

    // Both buffers should have the same number of buffers
    EXPECT_EQ(gst_buffer_list_length(cmaf_buffer),
              gst_buffer_list_length(gpaccmaf_buffer));

    std::vector<GstBufferList*> const buffer_lists = { cmaf_buffer,
                                                       gpaccmaf_buffer };
    for (GstBufferList* buffer_list : buffer_lists) {
      guint idx = 0;
      GstBuffer* buf;
      guint32 const buffer_count = gst_buffer_list_length(buffer_list);
      bool const is_cmaf = buffer_list == cmaf_buffer;
      bool const is_independent = segment_count == 0 || segment_count == 5;

#define GET_NEXT_BUFFER()                        \
  ASSERT_LT(idx, buffer_count);                  \
  buf = gst_buffer_list_get(buffer_list, idx++);

      if (segment_count == 0) {
        EXPECT_GE(buffer_count, 3); // init, header, delta

        // Check buffer #0
        GET_NEXT_BUFFER();
        IsSegmentInit(buf);
      } else {
        EXPECT_GE(buffer_count, 2); // header, delta
      }

      // Check buffer #1
      GET_NEXT_BUFFER();
      guint32 const data_size = IsSegmentHeader(buf, is_independent);

      // Check buffer #2...N
      guint32 leftover = data_size;
      while (gst_buffer_list_length(buffer_list) > idx) {
        GET_NEXT_BUFFER();
        leftover = IsSegmentData(buf, leftover);
        if (leftover == 0)
          break;
      }
      EXPECT_EQ(leftover, 0);
      EXPECT_EQ(gst_buffer_list_length(buffer_list), idx);

#undef GET_NEXT_BUFFER
    }
    segment_count++;
  }
}

TEST_F(GstTestFixture, TimingTest)
{
  // Set up the pipeline
  this->SetUpPipeline({ true, "x264enc" });

  // Create test elements
  GstElement* cmafmux = gst_element_factory_make_full("cmafmux",
                                                      "chunk-duration",
                                                      GST_SECOND,
                                                      "fragment-duration",
                                                      5 * GST_SECOND,
                                                      NULL);
  GstElement* gpaccmafmux = gst_element_factory_make_full(
    "gpaccmafmux", "cdur", 1.0, "segdur", 5.0, NULL);

  // Disable B-frames
  g_object_set(GetEncoder(), "b-adapt", FALSE, "bframes", 0, NULL);

  // Create element sinks
  GstAppSink* cmafmux_sink = new GstAppSink(cmafmux, tee, pipeline);
  GstAppSink* gpaccmafmux_sink = new GstAppSink(gpaccmafmux, tee, pipeline);

  // Start the pipeline
  this->StartPipeline();

  // Go through all buffers
  while (true) {
    GstBufferList* cmaf_buffer = cmafmux_sink->PopBuffer();
    GstBufferList* gpaccmaf_buffer = gpaccmafmux_sink->PopBuffer();

    // If neither buffer is available, we are done
    if (!cmaf_buffer && !gpaccmaf_buffer)
      break;

    // Both buffers should return something
    ASSERT_TRUE(cmaf_buffer && gpaccmaf_buffer);

#define GET_NEXT_BUFFER()                        \
  ASSERT_LT(idx, buffer_count);                  \
  buf = gst_buffer_list_get(buffer_list, idx++);

#define ROUND_TIME(time) ROUND_UP(GST_TIME_AS_USECONDS(time), 10)

    EXPECT_EQ(gst_buffer_list_length(cmaf_buffer),
              gst_buffer_list_length(gpaccmaf_buffer));

    // Go through all buffers
    for (guint idx = 0; idx < gst_buffer_list_length(cmaf_buffer); idx++) {
      GstBuffer* cmaf_buf = gst_buffer_list_get(cmaf_buffer, idx);
      GstBuffer* gpaccmafmux_buf = gst_buffer_list_get(gpaccmaf_buffer, idx);

      // The output will have the correct timing, but converting to nanoseconds
      // introduce fractional errors, so we need to round up to the nearest 10
      guint64 const cm_pts = ROUND_TIME(GST_BUFFER_PTS(cmaf_buf));
      guint64 const cm_dts = ROUND_TIME(GST_BUFFER_DTS(cmaf_buf));
      guint64 const cm_dur = ROUND_TIME(GST_BUFFER_DURATION(cmaf_buf));
      guint64 const gp_pts = ROUND_TIME(GST_BUFFER_PTS(gpaccmafmux_buf));
      guint64 const gp_dts = ROUND_TIME(GST_BUFFER_DTS(gpaccmafmux_buf));
      guint64 const gp_dur = ROUND_TIME(GST_BUFFER_DURATION(gpaccmafmux_buf));

      // Check all fields
      EXPECT_EQ(cm_pts, gp_pts);
      EXPECT_EQ(cm_dts, gp_dts);
      EXPECT_EQ(cm_dur, gp_dur);
    }

#undef GET_NEXT_BUFFER
#undef ROUND_TIME
  }
}
