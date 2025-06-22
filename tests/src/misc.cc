#include "helper/common.hpp"
#include <filesystem>
#include <gpac/isomedia.h>
#include <gpac/media_tools.h>

namespace fs = std::filesystem;

TEST_F(GstTestFixture, NonFragmented)
{
  this->SetUpPipeline({ false, "x264enc", 30 });
  GstElement* gpaccmafmux = gst_element_factory_make("gpacmp4mx", NULL);

  // Set the destination options
  std::string file = fs::temp_directory_path().string() + "/" + "nonfrag.mp4";
  GstElement* sink =
    gst_element_factory_make_full("filesink", "location", file.c_str(), NULL);

  // Add the elements to the pipeline
  gst_bin_add_many(GST_BIN(pipeline), gpaccmafmux, sink, NULL);

  // Link the elements
  if (!gst_element_link(this->GetLastElement(), gpaccmafmux) ||
      !gst_element_link(gpaccmafmux, sink)) {
    g_error("Failed to link elements");
    return;
  }

  this->StartPipeline();
  this->WaitForEOS();

  // Read the file
  ASSERT_TRUE(fs::exists(file));
  gf_sys_init(GF_MemTrackerNone, NULL);
  GF_ISOFile* isom = gf_isom_open(file.c_str(), GF_ISOM_OPEN_READ, NULL);
  ASSERT_TRUE(isom != NULL);

  // Check the track and sample count
  EXPECT_EQ(gf_isom_get_track_count(isom), 1);
  EXPECT_EQ(gf_isom_get_sample_count(isom, 1), 30);
  EXPECT_FALSE(gf_isom_is_fragmented(isom));

  // Close the file
  gf_isom_close(isom);
  gf_sys_close();
  fs::remove(file);
}

TEST_F(GstTestFixture, MultipleInputs)
{
  this->SetUpPipeline({ false, "x264enc", 30 });
  this->SetUpPipeline(
    { false, "avenc_aac", 1, 1, "audiotestsrc", "audio/x-raw, rate=44100" });

  // Set samples per buffer for the audio source
  GstElement* audio_source = this->GetSource(1);
  g_object_set(audio_source, "samplesperbuffer", 44100, NULL);

  // Create the muxer
  GstElement* gpaccmafmux = gst_element_factory_make("gpaccmafmux", NULL);

  // Set the destination options
  std::string file = fs::temp_directory_path().string() + "/" + "multi.mp4";
  GstElement* sink =
    gst_element_factory_make_full("filesink", "location", file.c_str(), NULL);

  // Add the elements to the pipeline
  gst_bin_add_many(GST_BIN(pipeline), gpaccmafmux, sink, NULL);

  // Link the elements
  if (!gst_element_link(this->GetLastElement(0), gpaccmafmux) ||
      !gst_element_link(this->GetLastElement(1), gpaccmafmux) ||
      !gst_element_link(gpaccmafmux, sink)) {
    g_error("Failed to link elements");
    return;
  }

  this->StartPipeline();
  this->WaitForEOS();

  // Read the file
  ASSERT_TRUE(fs::exists(file));
  gf_sys_init(GF_MemTrackerNone, NULL);
  GF_ISOFile* isom = gf_isom_open(file.c_str(), GF_ISOM_OPEN_READ, NULL);
  ASSERT_TRUE(isom != NULL);

  // Check the track and sample count
  EXPECT_EQ(gf_isom_get_track_count(isom), 2);
  EXPECT_EQ(gf_isom_get_sample_count(isom, 1), 30);
  EXPECT_EQ(gf_isom_get_sample_count(isom, 2), 45);
  EXPECT_TRUE(gf_isom_is_fragmented(isom));

  // Close the file
  gf_isom_close(isom);
  gf_sys_close();
  fs::remove(file);
}
