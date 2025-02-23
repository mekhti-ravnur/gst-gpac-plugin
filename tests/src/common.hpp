#pragma once

#include <gst/gst.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#define ROUND_UP(val, round) (((val) + (round) - 1) / (round) * (round))

struct PipelineConfiguration
{
  bool use_tee = false;
  std::string encoder = "x264enc";
  int num_buffers = 300;
  std::string source = "videotestsrc";
  std::string caps = "video/x-raw, framerate=30/1";
};

class GstAppSink
{
private:
  GstElement* appsink;

public:
  GstAppSink(GstElement* test_element,
             GstElement* connect_from,
             GstElement* pipeline)
  {
    // Create the elements
    GstElement* queue = gst_element_factory_make("queue", NULL);
    appsink = gst_element_factory_make("appsink", NULL);
    if (!queue || !appsink) {
      g_error("Failed to create elements");
      return;
    }

    // Set the appsink properties
    g_object_set(
      appsink, "emit-signals", TRUE, "sync", FALSE, "buffer-list", TRUE, NULL);

    // Add the appsink to the test element
    gst_bin_add_many(GST_BIN(pipeline), queue, test_element, appsink, NULL);

    // Link the elements
    if (!gst_element_link(connect_from, queue) ||
        !gst_element_link(queue, test_element) ||
        !gst_element_link(test_element, appsink)) {
      g_error("Failed to link elements");
      return;
    }
  }

  GstBufferList* PopBuffer()
  {
    GstSample* sample;
    g_signal_emit_by_name(appsink, "pull-sample", &sample);
    if (sample == NULL)
      return NULL;

    GstBufferList* buffer = gst_sample_get_buffer_list(sample);
    gst_buffer_list_ref(buffer);
    gst_sample_unref(sample);

    return buffer;
  }

  ~GstAppSink() { gst_object_unref(appsink); }

  void SetSync(bool sync) { g_object_set(appsink, "sync", sync, NULL); }
};

class GstTestFixture : public ::testing::Test
{
private:
  std::vector<GstElement*> sources;
  std::vector<GstElement*> source_sinks;
  std::vector<GstElement*> encoders;
  bool is_eos = false;

protected:
  GstElement* tee;
  GstElement* pipeline;

  static void SetUpTestSuite() { gst_init(NULL, NULL); }
  static void TearDownTestSuite() { gst_deinit(); }
  GstElement* GetSource(int i = 0) { return sources[i]; }
  GstElement* GetLastElement(int i = 0) { return source_sinks[i]; }
  GstElement* GetEncoder(int i = 0) { return encoders[i]; }
  void SetLive(bool is_live)
  {
    for (GstElement* source : sources)
      g_object_set(source, "is-live", is_live, NULL);
  }

  void SetUp() override
  {
    // Create the pipeline
    pipeline = gst_pipeline_new("test-pipeline");
    if (!pipeline) {
      g_error("Failed to create elements");
      return;
    }
  }

  void SetUpPipeline(PipelineConfiguration cfg)
  {
    GstElement* source = gst_element_factory_make_full(
      cfg.source.c_str(), "do-timestamp", TRUE, "is-live", FALSE, NULL);
    sources.push_back(source);

    GstElement* capsfilter = gst_element_factory_make_full(
      "capsfilter", "caps", gst_caps_from_string(cfg.caps.c_str()), NULL);

    // Check if the elements were created
    if (!source || !capsfilter) {
      g_error("Failed to create elements");
      return;
    }

    // Add the source to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, NULL);

    // Link the elements
    if (!gst_element_link(source, capsfilter)) {
      g_error("Failed to link elements");
      return;
    }

    // Reconfigure the source
    g_object_set(source, "num-buffers", cfg.num_buffers, NULL);

    // Create the encoder
    GstElement* encoder =
      gst_element_factory_make_full(cfg.encoder.c_str(), NULL);
    if (!encoder) {
      g_error("Failed to create elements");
      return;
    }
    encoders.push_back(encoder);

    // Add the encoder to the pipeline
    gst_bin_add(GST_BIN(pipeline), encoder);

    // Link the encoder to the last element
    if (!gst_element_link(capsfilter, encoder)) {
      g_error("Failed to link elements");
      return;
    }
    source_sinks.push_back(encoder);

    // Add the tee if needed
    if (cfg.use_tee) {
      // Cannot handle multiple sources
      assert(source_sinks.size() == 1);

      tee = gst_element_factory_make("tee", NULL);
      if (!tee) {
        g_error("Failed to create elements");
        return;
      }

      gst_bin_add(GST_BIN(pipeline), tee);

      // Link the elements
      if (!gst_element_link(source_sinks[0], tee)) {
        g_error("Failed to link elements");
        return;
      }
      source_sinks[0] = tee;
    }
  }

  void WaitForEOS()
  {
    // Check if already in EOS
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    if (state == GST_STATE_NULL || is_eos)
      return;

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(
      bus,
      GST_CLOCK_TIME_NONE,
      (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Parse message
    if (msg != NULL) {
      GError* err;
      gchar* debug_info;

      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          g_printerr("Error received from element %s: %s\n",
                     GST_OBJECT_NAME(msg->src),
                     err->message);
          g_printerr("Debugging information: %s\n",
                     debug_info ? debug_info : "none");
          g_clear_error(&err);
          g_free(debug_info);
          break;
        case GST_MESSAGE_EOS:
          is_eos = true;
          break;
        default:
          // We should not reach here because we only asked for ERRORs and EOS
          g_printerr("Unexpected message received.\n");
          break;
      }
      gst_message_unref(msg);
    }

    // Free resources
    gst_object_unref(bus);
  }

  void TearDown() override
  {
    // Wait until error or EOS
    WaitForEOS();

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
  }

  bool StartPipeline()
  {
    GstStateChangeReturn ret =
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      g_error("Failed to start pipeline");
      return false;
    }
    return true;
  }
};
