#include "helper/common.hpp"
#include "helper/smemcapture.hpp"
#include <filesystem>
#include <gio/gio.h>
#include <gpac/isomedia.h>
#include <gpac/media_tools.h>

namespace fs = std::filesystem;

#define CHECK_MANIFEST_FILE(idx)                               \
  do {                                                         \
    std::string manifest;                                      \
    if ((idx) > 0)                                             \
      manifest = "master_" + std::to_string(idx) + ".m3u8";    \
    else                                                       \
      manifest = "master.m3u8";                                \
    const auto* buffer =                                       \
      capture.get_labeled(const_cast<std::string&>(manifest)); \
    ASSERT_TRUE(buffer != nullptr);                            \
    ASSERT_GT(buffer->size(), 0);                              \
  } while (0)

#define EXPECT_WITHIN_RANGE(value, min, max)                                   \
  do {                                                                         \
    EXPECT_GE(value, min) << "Value " << value << " is below minimum " << min; \
    EXPECT_LE(value, max) << "Value " << value << " is above maximum " << max; \
  } while (0)

// Combines init and segment files into a single buffer
static std::vector<uint8_t>
merge_init_and_segment_files(SignalMemoryCapture& capture,
                             std::string tmpl,
                             std::string seg_ext,
                             std::string override_init = "")
{
  std::string init_name = tmpl;
  const size_t pos = init_name.find("%s");
  if (override_init.empty()) {
    if (pos != std::string::npos)
      init_name.replace(pos, 2, "init");
    init_name += ".mp4";
  } else {
    init_name = override_init;
  }

  const auto* init_buf = capture.get_labeled(init_name);
  if (init_buf == nullptr)
    throw std::runtime_error("No init segment found for template: " + tmpl);

  std::vector<uint8_t> combined;
  combined.insert(combined.end(), init_buf->begin(), init_buf->end());
  for (int seg_idx = 1;; seg_idx++) {
    std::string seg_name = tmpl;
    if (pos != std::string::npos)
      seg_name.replace(pos, 2, std::to_string(seg_idx));
    seg_name += "." + seg_ext;

    const auto* seg_buf = capture.get_labeled(seg_name);
    if (!seg_buf)
      break;
    combined.insert(combined.end(), seg_buf->begin(), seg_buf->end());
  }
  return combined;
}

GF_ISOFile*
gf_isom_open_from_buffer(const std::vector<uint8_t>& buffer)
{
  /* Write buffer to a temporary file */
  char tmp_filename[] = "/tmp/isomXXXXXX";
  const int fd = mkstemp(tmp_filename);
  if (fd == -1)
    throw std::runtime_error("Failed to create temp file");
  const ssize_t written = write(fd, buffer.data(), buffer.size());
  if (written != (ssize_t)buffer.size()) {
    close(fd);
    unlink(tmp_filename);
    throw std::runtime_error("Failed to write temp file");
  }
  close(fd);
  /* Open with gf_isom_open */
  GF_ISOFile* isom = gf_isom_open(tmp_filename, GF_ISOM_OPEN_READ, NULL);
  unlink(tmp_filename);
  if (!isom)
    throw std::runtime_error("gf_isom_open failed");
  return isom;
}

TEST_F(GstTestFixture, HLSMultiVariant)
{
  PipelineConfigurationMany cfg;
  cfg.v_num_buffers = 30 * 10 * 3;
  cfg.a_num_buffers = 48000 / 1024 * 10 * 3;

  this->SetUpPipelineMany(cfg);
  GstElement* gpachlssink =
    gst_element_factory_make_full("gpachlssink", "segdur", 10.0, NULL);

  // Create signal handlers
  SignalMemoryCapture capture;
  capture.connect(gpachlssink, "get-manifest");
  capture.connect(gpachlssink, "get-manifest-variant");
  capture.connect(gpachlssink, "get-segment-init");
  capture.connect(gpachlssink, "get-segment");

  // Add the sink to the pipeline
  gst_bin_add(GST_BIN(pipeline), gpachlssink);
  // Link the elements
  for (auto& encoder : GetEncoders()) {
    if (!gst_element_link(encoder, gpachlssink)) {
      g_error("Failed to link elements");
      return;
    }
  }

  GST_DEBUG_BIN_TO_DOT_FILE(
    GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "hls-multi-variant");

  this->StartPipeline();
  this->WaitForEOS();
  capture.finish(gpachlssink);

  // Check manifests
  CHECK_MANIFEST_FILE(0);
  CHECK_MANIFEST_FILE(1);
  CHECK_MANIFEST_FILE(2);
  CHECK_MANIFEST_FILE(3);
  CHECK_MANIFEST_FILE(4);

  //
  // Check the signals
  //

  // Init: One for video representations and one for audio
  ASSERT_EQ(capture.get_all("get-segment-init").size(), 2);

  // Segments: 3 for each video representation and 3 for audio
  ASSERT_EQ(capture.get_all("get-segment").size(), 4 * 3);

  //
  // Check the streams
  //

  // Audio
  auto audio_streams =
    merge_init_and_segment_files(capture, "PID4_dash_track4_%s", "m4s");
  ASSERT_GT(audio_streams.size(), 0);

  // Video 1
  auto video1_streams =
    merge_init_and_segment_files(capture, "PID1_dash_track3_%s_rep1", "m4s");
  ASSERT_GT(video1_streams.size(), 0);

  // Video 2
  auto video2_streams =
    merge_init_and_segment_files(capture,
                                 "PID2_dash_track3_%s_rep2",
                                 "m4s",
                                 "PID1_dash_track3_init_rep1.mp4");
  ASSERT_GT(video2_streams.size(), 0);

  // Video 3
  auto video3_streams =
    merge_init_and_segment_files(capture,
                                 "PID3_dash_track3_%s_rep3",
                                 "m4s",
                                 "PID1_dash_track3_init_rep1.mp4");
  ASSERT_GT(video3_streams.size(), 0);

  // Deep validate
  gf_sys_init(GF_MemTrackerNone, NULL);

  // Audio
  GF_ISOFile* audio_isom = gf_isom_open_from_buffer(audio_streams);
  ASSERT_TRUE(audio_isom != NULL);
  EXPECT_EQ(gf_isom_get_track_count(audio_isom), 1);
  EXPECT_WITHIN_RANGE(gf_isom_get_sample_count(audio_isom, 1),
                      cfg.a_num_buffers - 1,
                      cfg.a_num_buffers + 1);
  EXPECT_TRUE(gf_isom_is_fragmented(audio_isom));
  gf_isom_close(audio_isom);

  // Video 1
  GF_ISOFile* video1_isom = gf_isom_open_from_buffer(video1_streams);
  ASSERT_TRUE(video1_isom != NULL);
  EXPECT_EQ(gf_isom_get_track_count(video1_isom), 1);
  EXPECT_EQ(gf_isom_get_sample_count(video1_isom, 1), cfg.v_num_buffers);
  EXPECT_TRUE(gf_isom_is_fragmented(video1_isom));
  gf_isom_close(video1_isom);

  // Video 2
  GF_ISOFile* video2_isom = gf_isom_open_from_buffer(video2_streams);
  ASSERT_TRUE(video2_isom != NULL);
  EXPECT_EQ(gf_isom_get_track_count(video2_isom), 1);
  EXPECT_EQ(gf_isom_get_sample_count(video2_isom, 1), cfg.v_num_buffers);
  EXPECT_TRUE(gf_isom_is_fragmented(video2_isom));
  gf_isom_close(video2_isom);

  // Video 3
  GF_ISOFile* video3_isom = gf_isom_open_from_buffer(video3_streams);
  ASSERT_TRUE(video3_isom != NULL);
  EXPECT_EQ(gf_isom_get_track_count(video3_isom), 1);
  EXPECT_EQ(gf_isom_get_sample_count(video3_isom, 1), cfg.v_num_buffers);
  EXPECT_TRUE(gf_isom_is_fragmented(video3_isom));
  gf_isom_close(video3_isom);

  gf_sys_close();
}
