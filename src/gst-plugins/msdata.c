#include <config.h>
#include <gst/gst.h>

#include "kmsmsdata.h"
#include "kmsmsdatagraph.h"

static gboolean
init (GstPlugin * plugin)
{
  if (!kms_msdata_plugin_init (plugin))
    return FALSE;
  if (!kms_msdata_graph_plugin_init(plugin))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    msdata,
    "Filter documentation",
    init, VERSION, GST_LICENSE_UNKNOWN, "PACKAGE_NAME", "origin")
