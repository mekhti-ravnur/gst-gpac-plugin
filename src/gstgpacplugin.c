#include "config.h"

#include "elements/gstgpacsink.h"
#include "elements/gstgpactf.h"

static gboolean
plugin_init(GstPlugin* plugin)
{
  gboolean ret = TRUE;
  ret |= GST_ELEMENT_REGISTER(gpac_tf, plugin);
  ret |= GST_ELEMENT_REGISTER(gpac_sink, plugin);
  return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MIN_REQUIRED_MAJOR,
                  GST_VERSION_MIN_REQUIRED_MINOR,
                  gpac_plugin,
                  PACKAGE_DESC,
                  plugin_init,
                  VERSION,
                  LICENSE,
                  PACKAGE_NAME,
                  PACKAGE_ORIGIN)
