#pragma once

#include <gst/gst.h>
#include <gtest/gtest.h>
#include <thread>

class GstAppSink
{
private:
  std::mutex buffer_queue_mutex;
  std::condition_variable buffer_queue_cv;
  GQueue* buffer_queue;
  GstElement* appsink;

protected:
  static GstFlowReturn NewSample(GstElement* sink, GstAppSink* self)
  {
    GstSample* sample;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample == NULL)
      return GST_FLOW_ERROR;

    // Push the sample into the buffer queue
    GstBufferList* buffer = gst_sample_get_buffer_list(sample);
    gst_buffer_list_ref(buffer);
    g_queue_push_tail(self->buffer_queue, buffer);
    gst_sample_unref(sample);
    self->buffer_queue_cv.notify_one();
    return GST_FLOW_OK;
  }

public:
  GstAppSink(GstElement* test_element, GstElement* tee, GstElement* pipeline)
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
    g_signal_connect(appsink, "new-sample", G_CALLBACK(NewSample), this);
    buffer_queue = g_queue_new();

    // Add the appsink to the test element
    gst_bin_add_many(GST_BIN(pipeline), queue, test_element, appsink, NULL);

    // Link the elements
    if (!gst_element_link(tee, queue) ||
        !gst_element_link(queue, test_element) ||
        !gst_element_link(test_element, appsink)) {
      g_error("Failed to link elements");
      return;
    }
  }

  GstBufferList* PopBuffer()
  {
    // Wait until we have at least one buffer in the queue
    std::unique_lock<std::mutex> lock(buffer_queue_mutex);
    buffer_queue_cv.wait(lock, [&]() {
      gboolean eos = false;
      g_object_get(G_OBJECT(this->appsink), "eos", &eos, NULL);
      if (eos)
        return true;
      return !g_queue_is_empty(this->buffer_queue);
    });

    return (GstBufferList*)g_queue_pop_head(buffer_queue);
  }

  ~GstAppSink()
  {
    // Flush the buffer queue
    while (!g_queue_is_empty(buffer_queue)) {
      GstBufferList* buffer = (GstBufferList*)g_queue_pop_head(buffer_queue);
      gst_buffer_list_unref(buffer);
    }

    // Free resources
    gst_object_unref(appsink);
    g_queue_free(buffer_queue);
  }

  int GetBufferCount() { return g_queue_get_length(buffer_queue); }
};

class GstTestFixture : public ::testing::Test
{
protected:
  GstElement* tee;
  GstElement* pipeline;

  static void SetUpTestSuite() { gst_init(NULL, NULL); }
  static void TearDownTestSuite() { gst_deinit(); }

  void SetUp() override
  {
    GstElement* source = gst_element_factory_make_full(
      "videotestsrc", "num-buffers", 60, "do-timestamp", TRUE, NULL);
    GstElement* capsfilter = gst_element_factory_make_full(
      "capsfilter",
      "caps",
      gst_caps_from_string("video/x-raw, framerate=30/1"),
      NULL);
    GstElement* encoder =
      gst_element_factory_make_full("x264enc", "key-int-max", 30, NULL);
    tee = gst_element_factory_make("tee", NULL);

    // Create the pipeline
    pipeline = gst_pipeline_new("test-pipeline");
    if (!pipeline || !source || !capsfilter || !encoder || !tee) {
      g_error("Failed to create elements");
      return;
    }

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, encoder, tee, NULL);
    if (!gst_element_link(source, capsfilter) ||
        !gst_element_link(capsfilter, encoder) ||
        !gst_element_link(encoder, tee)) {
      g_error("Failed to link elements");
      return;
    }
  }

  void TearDown() override
  {
    // Wait until error or EOS
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
