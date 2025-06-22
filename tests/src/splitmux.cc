#include "helper/common.hpp"
#include <filesystem>

namespace fs = std::filesystem;

TEST_F(GstTestFixture, SplitMux)
{
  this->SetUpPipeline({ false, "x264enc", 300 });

  // Create a random folder
  std::string folder = fs::temp_directory_path().string() + "/splitmux";
  fs::create_directory(folder);
  std::string location = folder + "/video%05d.mp4";

  // Create the elements
  GstElement* muxer =
    gst_element_factory_make_full("gpaccmafmux", "cdur", 5.0, NULL);
  GstElement* element = gst_element_factory_make_full("splitmuxsink",
                                                      "muxer",
                                                      muxer,
                                                      "location",
                                                      location.c_str(),
                                                      "max-size-time",
                                                      5 * GST_SECOND,
                                                      NULL);

  // Set the GOP size
  g_object_set(GetEncoder(), "key-int-max", 150, NULL);

  if (!gst_bin_add(GST_BIN(pipeline), element)) {
    g_error("Failed to create elements");
    return;
  }

  if (!gst_element_link(this->GetLastElement(), element)) {
    g_error("Failed to link elements");
    return;
  }

  // Start the pipeline
  this->StartPipeline();
  this->WaitForEOS();

  // Check the number of files created
  uint32_t count = 0;
  for (const auto& entry : fs::directory_iterator(folder))
    count++;
  EXPECT_EQ(count, 2);

  // Clean up
  fs::remove_all(folder);
}
