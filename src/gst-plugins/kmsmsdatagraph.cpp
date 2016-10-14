/**
Licensing and distribution

ArModule is licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

ALVAR 2.0.0 - A Library for Virtual and Augmented Reality Copyright 2007-2012 VTT Technical Research Centre of Finland Licensed under the GNU Lesser General Public License

Irrlicht Engine, the zlib and libpng. The Irrlicht Engine is based in part on the work of the Independent JPEG Group The module utilizes IJG code when the Irrlicht engine is compiled with support for JPEG images.
*/

/** @author Markus Ylikerälä */

#define _XOPEN_SOURCE 500

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <glib/gstdio.h>
#include <ftw.h>
#include <string.h>
#include <errno.h>

#include <gst/gst.h>
#include "commons/kmselement.h"
#include <gst/video/gstvideofilter.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>

#include "kmsmsdatagraph.h"

#include <commons/kmsserializablemeta.h>


#define PLUGIN_NAME "kmsmsdatagraph"
                     
GST_DEBUG_CATEGORY_STATIC (kms_msdata_graph_debug_category);
#define GST_CAT_DEFAULT kms_msdata_graph_debug_category

#define KMS_MSDATA_GRAPH_GET_PRIVATE(obj) ( \
  G_TYPE_INSTANCE_GET_PRIVATE (              \
    (obj),                                   \
    KMS_TYPE_MSDATA_GRAPH,                  \
    KmsMsDataGraphPrivate                   \
  )                                          \
)

enum
{
  PROP_0,
  PROP_SHOW_DEBUG_INFO
};

struct _KmsMsDataGraphPrivate
{
  GstElement *text_overlay;
  IplImage *cvImage;
  gdouble offsetXPercent, offsetYPercent, widthPercent, heightPercent;
  gboolean show_debug_info;
};

static std::vector<int> numbers;
static gint latestValue;


/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (KmsMsDataGraph, kms_msdata_graph,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (kms_msdata_graph_debug_category,
        PLUGIN_NAME, 0, "debug category for graphs element"));



void newMsDataValue (GstElement* object, GstBuffer* buffer, GstElement* elm)
{
  GstMapInfo info;
  gchar *msg;

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ)) {
    GST_DEBUG ("cannot read buffer");
    return;
  }

  msg = g_strndup ((const gchar *) info.data, info.size);
  gst_buffer_unmap (buffer, &info);

  if (msg != NULL) {
    //std::cout << "got X data: " << msg << std::endl;
    if(g_ascii_isxdigit(*msg)){
      latestValue = g_ascii_strtoll(msg, NULL, 0);
    }
    g_free (msg);
  }
}


static void
kms_msdata_graph_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  KmsMsDataGraph *graphs = KMS_MSDATA_GRAPH (object);

  GST_OBJECT_LOCK (graphs);

  switch (property_id) {
    case PROP_SHOW_DEBUG_INFO:
      graphs->priv->show_debug_info = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (graphs);
}


static void
kms_msdata_graph_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  KmsMsDataGraph *graphs = KMS_MSDATA_GRAPH (object);

  GST_DEBUG_OBJECT (graphs, "get_property");

  GST_OBJECT_LOCK (graphs);

  switch (property_id) {
    case PROP_SHOW_DEBUG_INFO:
      g_value_set_boolean (value, graphs->priv->show_debug_info);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (graphs);
}

static void
kms_msdata_graph_initialize_images (KmsMsDataGraph *
    graphs, GstVideoFrame * frame)
{
  if (graphs->priv->cvImage == NULL) {
    graphs->priv->cvImage =
        cvCreateImage (cvSize (frame->info.width, frame->info.height),
        IPL_DEPTH_8U, 3);

  } else if ((graphs->priv->cvImage->width != frame->info.width)
      || (graphs->priv->cvImage->height != frame->info.height)) {

    cvReleaseImage (&graphs->priv->cvImage);
    graphs->priv->cvImage =
        cvCreateImage (cvSize (frame->info.width, frame->info.height),
        IPL_DEPTH_8U, 3);
  }
}

static void visualize(cv::Mat feature_vis){
  int windowWidth = 640;
  int windowHeight = 480;

  numbers.push_back(latestValue);
  int memoryLength = numbers.size();


  int yBias = 50;
  int xBias = 50;
  char text[100];
  
  sprintf(text, "random");
  cv::putText(feature_vis, text, cvPoint(10, windowHeight - 15), 1, 1, CV_RGB(255, 0, 0), 1);
  
  sprintf(text, "0");
  cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias), cvPoint(windowWidth - xBias, windowHeight - yBias), CV_RGB(255, 255, 255), 1);

    sprintf(text, "100");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 100), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 100), cvPoint(windowWidth - xBias, windowHeight - yBias - 100), CV_RGB(255, 255, 255), 1);

    sprintf(text, "200");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 200), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 200), cvPoint(windowWidth - xBias, windowHeight - yBias - 200), CV_RGB(255, 255, 255), 1);

    sprintf(text, "300");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 300), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 300), cvPoint(windowWidth - xBias, windowHeight - yBias - 300), CV_RGB(255, 255, 255), 1);

    sprintf(text, "400");
    cv::putText(feature_vis, text, cvPoint(10, windowHeight - yBias - 400), 1, 1, CV_RGB(255, 0, 0), 1);
    cv::line(feature_vis, cvPoint(xBias, windowHeight - yBias - 400), cvPoint(windowWidth - xBias, windowHeight - yBias - 400), CV_RGB(255, 255, 255), 1);


    for (int i = 1; i<memoryLength; i++) {
      int y1 = windowHeight - yBias - numbers.at(i - 1);
      int y2 = windowHeight - yBias - numbers.at(i);
      int x1 = (windowWidth / memoryLength) * i + xBias;
      int x2 = (windowWidth / memoryLength) * (i + 1) + xBias;
      cv::line(feature_vis, cvPoint( x1, y1), cvPoint(x2, y2), CV_RGB(255, 0, 0), 2);
    }
    if(numbers.size() > 255){
      numbers.erase(numbers.begin());
    }


}



static GstFlowReturn
kms_msdata_graph_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame)
{
  //std::cout << "got frame" << std::endl;
  KmsMsDataGraph *graphs = KMS_MSDATA_GRAPH (filter);
  GstMapInfo info;

  gst_buffer_map (frame->buffer, &info, GST_MAP_READ);

  kms_msdata_graph_initialize_images (graphs, frame);
  graphs->priv->cvImage->imageData = (char *) info.data;

  GST_OBJECT_LOCK (graphs);

  visualize(graphs->priv->cvImage);

  GST_OBJECT_UNLOCK (graphs);

  gst_buffer_unmap (frame->buffer, &info);


  return GST_FLOW_OK;
}

static void
kms_msdata_graph_dispose (GObject * object)
{
  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (kms_msdata_graph_parent_class)->dispose (object);
}

static void
kms_msdata_graph_finalize (GObject * object)
{
  KmsMsDataGraph *graphs = KMS_MSDATA_GRAPH (object);

  if (graphs->priv->cvImage != NULL)
    cvReleaseImage (&graphs->priv->cvImage);

  G_OBJECT_CLASS (kms_msdata_graph_parent_class)->finalize (object);
}


static void
kms_msdata_graph_init (KmsMsDataGraph * graphs)
{
  graphs->priv = KMS_MSDATA_GRAPH_GET_PRIVATE (graphs);

  graphs->priv->show_debug_info = FALSE;
  graphs->priv->cvImage = NULL;

  latestValue = 0;
}

static void
kms_msdata_graph_class_init (KmsMsDataGraphClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, PLUGIN_NAME);

  GST_DEBUG ("class init");

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "MsDataGraph image overlay element", "Video/Filter",
      "Draw a graph with given values",
      "Markus Ylikerala <Markus.Ylikerala@vtt.fi>");

  gobject_class->set_property = kms_msdata_graph_set_property;
  gobject_class->get_property = kms_msdata_graph_get_property;
  gobject_class->dispose = kms_msdata_graph_dispose;
  gobject_class->finalize = kms_msdata_graph_finalize;

  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (kms_msdata_graph_transform_frame_ip);

  /* Properties initialization */
  g_object_class_install_property (gobject_class, PROP_SHOW_DEBUG_INFO,
      g_param_spec_boolean ("show-debug-region", "show debug region",
          "show evaluation regions over the image", FALSE, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (KmsMsDataGraphPrivate));
}

gboolean
kms_msdata_graph_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      KMS_TYPE_MSDATA_GRAPH);
}
