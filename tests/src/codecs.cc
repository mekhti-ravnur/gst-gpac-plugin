#include "helper/common.hpp"
#include <filesystem>
#include <gpac/isomedia.h>
#include <gpac/media_tools.h>

namespace fs = std::filesystem;

#define SETUP_PIPELINE(enc, fname, sample_count)                             \
  this->SetUpPipeline({ false, enc, sample_count });                         \
  std::string file = fs::temp_directory_path().string() + "/" + fname;       \
  std::string graph = "-o " + file;                                          \
  GstElement* element =                                                      \
    gst_element_factory_make_full("gpacsink", "graph", graph.c_str(), NULL); \
  if (!gst_bin_add(GST_BIN(pipeline), element)) {                            \
    g_error("Failed to create elements");                                    \
    return;                                                                  \
  }                                                                          \
  if (!gst_element_link(this->GetLastElement(), element)) {                  \
    g_error("Failed to link elements");                                      \
    return;                                                                  \
  }                                                                          \
  this->StartPipeline();                                                     \
  this->WaitForEOS();

#define TEARDOWN_PIPELINE() fs::remove(file);

void
CheckFile(const std::string& file, u32 track_count, u32 sample_count)
{
  ASSERT_TRUE(fs::exists(file));
  gf_sys_init(GF_MemTrackerNone, NULL);
  GF_ISOFile* isom = gf_isom_open(file.c_str(), GF_ISOM_OPEN_READ, NULL);
  ASSERT_TRUE(isom != NULL);
  EXPECT_EQ(gf_isom_get_track_count(isom), track_count);
  EXPECT_EQ(gf_isom_get_sample_count(isom, 1), sample_count);
  gf_isom_close(isom);
  gf_sys_close();
}

TEST_F(GstTestFixture, HandlesX264)
{
  SETUP_PIPELINE("x264enc", "x264.mp4", 5);
  CheckFile(file, 1, 5);
  TEARDOWN_PIPELINE();
}

TEST_F(GstTestFixture, HandlesX265)
{
  SETUP_PIPELINE("x265enc", "x265.mp4", 5);
  CheckFile(file, 1, 5);
  TEARDOWN_PIPELINE();
}

TEST_F(GstTestFixture, HandlesAV1)
{
  SETUP_PIPELINE("av1enc", "av1.mp4", 2);
  CheckFile(file, 1, 2);
  TEARDOWN_PIPELINE();
}
