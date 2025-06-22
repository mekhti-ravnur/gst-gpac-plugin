#include "helper/common.hpp"

TEST_F(GstTestFixture, StatePausedToNullToPlaying)
{
  this->SetUpPipeline({ false, "x264enc", 5 });
  GstElement* muxer =
    gst_element_factory_make_full("gpaccmafmux", "cdur", 5.0, NULL);
  GstElement* fake_sink = gst_element_factory_make("fakesink", NULL);
  gst_bin_add_many(GST_BIN(pipeline), muxer, fake_sink, NULL);

  if (!gst_element_link(this->GetLastElement(), muxer) ||
      !gst_element_link(muxer, fake_sink)) {
    g_error("Failed to link elements");
    return;
  }

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_error("Failed to start pipeline");
  }

  // Set the NULL state
  ret = gst_element_set_state(pipeline, GST_STATE_NULL);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_error("Failed to set pipeline to NULL state");
  }

  // Back to the PLAYING state
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_error("Failed to start pipeline");
  }

  // Wait for the EOS
  this->WaitForEOS();
}
