# Tests

## Setup

From the root of the repository, configure the project to build the tests:

```bash
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build
```

Some tests require [`gst-plugin-fmp4`](https://gitlab.freedesktop.org/gstreamer/gst-plugins-rs) to be built and installed on your system. Follow the instructions in the repository to build and install the plugin. Specifically, the `cmafmux` element is used to cross-validate the output of the `gpacmp4mx` element.

> [!NOTE]
> GStreamer documentation describes the plugin search path in detail. You can refer to the [GStreamer documentation](https://gstreamer.freedesktop.org/documentation/gstreamer/gstregistry.html?gi-language=c) to understand how GStreamer searches for plugins.

## Running tests

To run the tests, execute the following command:

```bash
./build/tests/gstgpacplugin_test
```
