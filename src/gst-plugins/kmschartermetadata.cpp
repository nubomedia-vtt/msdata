/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.orglicenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


//#include <vector>
//#include <cmath>
//#include <boost/tuple/tuple.hpp>

#include "kmschartermetadata.h"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#include <glib/gprintf.h>

#include <commons/kmsserializablemeta.h>

#include <sys/time.h>

#define GREEN CV_RGB (0, 255, 0)

#define PLUGIN_NAME "chartermetadata"

#define HAAR_CASCADES_DIR_OPENCV_PREFIX "/usr/share/opencv/haarcascades/"

#define FACE_HAAR_FILE "haarcascade_frontalface_default.xml"

#define MIN_FPS 5
#define MIN_TIME ((float)(1.0/7.0))

#define MAX_WIDTH 320

GST_DEBUG_CATEGORY_STATIC (kms_charter_metadata_debug_category);
#define GST_CAT_DEFAULT kms_charter_metadata_debug_category

#define KMS_CHARTER_METADATA_GET_PRIVATE(obj) (    \
  G_TYPE_INSTANCE_GET_PRIVATE (                 \
    (obj),                                      \
    KMS_TYPE_CHARTER_METADATA,                     \
    KmsCharterMetadataPrivate                      \
  )                                             \
)

struct _KmsCharterMetadataPrivate
{
  IplImage *cvImage;
  IplImage *cvResizedImage;
  gdouble resize_factor;

  gboolean show_debug_info;
  const char *images_path;
  gint throw_frames;
  gboolean qos_control;
  gboolean haar_detector;
  GMutex mutex;

  CvHaarClassifierCascade *pCascadeFace;
  CvMemStorage *pStorageFace;
  CvSeq *pFaceRectSeq;
};

enum
{
  PROP_0,
  PROP_SHOW_DEBUG_INFO,
  PROP_FILTER_VERSION
};

/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (KmsCharterMetadata, kms_charter_metadata,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (kms_charter_metadata_debug_category,
        PLUGIN_NAME, 0, "debug category for charter element"));

static void
kms_charter_metadata_initialize_classifiers (KmsCharterMetadata *
    charter)
{
  GST_DEBUG ("Loading classifier: %s",
      HAAR_CASCADES_DIR_OPENCV_PREFIX FACE_HAAR_FILE);
  charter->priv->pCascadeFace = (CvHaarClassifierCascade *)
      cvLoad ((HAAR_CASCADES_DIR_OPENCV_PREFIX FACE_HAAR_FILE), 0, 0, 0);
}

static void
kms_charter_metadata_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  KmsCharterMetadata *charter = KMS_CHARTER_METADATA (object);

  switch (property_id) {
    case PROP_SHOW_DEBUG_INFO:
      charter->priv->show_debug_info = g_value_get_boolean (value);
      break;
    case PROP_FILTER_VERSION:
      charter->priv->haar_detector = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kms_charter_metadata_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  KmsCharterMetadata *charter = KMS_CHARTER_METADATA (object);

  switch (property_id) {
    case PROP_SHOW_DEBUG_INFO:
      g_value_set_boolean (value, charter->priv->show_debug_info);
      break;
    case PROP_FILTER_VERSION:
      g_value_set_boolean (value, charter->priv->haar_detector);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
kms_charter_metadata_initialize_images (KmsCharterMetadata *
    charter, GstVideoFrame * frame)
{
  if (charter->priv->cvImage == NULL) {
    int target_width =
        frame->info.width <= MAX_WIDTH ? frame->info.width : MAX_WIDTH;

    charter->priv->resize_factor = frame->info.width / target_width;

    charter->priv->cvImage =
        cvCreateImageHeader (cvSize (frame->info.width, frame->info.height),
        IPL_DEPTH_8U, 3);

    charter->priv->cvResizedImage =
        cvCreateImage (cvSize (target_width,
            frame->info.height / charter->priv->resize_factor),
        IPL_DEPTH_8U, 3);
  } else if ((charter->priv->cvImage->width != frame->info.width)
      || (charter->priv->cvImage->height != frame->info.height)) {
    int target_width =
        frame->info.width <= MAX_WIDTH ? frame->info.width : MAX_WIDTH;

    charter->priv->resize_factor = frame->info.width / target_width;

    cvReleaseImageHeader (&charter->priv->cvImage);
    cvReleaseImage (&charter->priv->cvResizedImage);

    charter->priv->cvImage =
        cvCreateImageHeader (cvSize (frame->info.width, frame->info.height),
        IPL_DEPTH_8U, 3);

    charter->priv->cvResizedImage =
        cvCreateImage (cvSize (target_width,
            frame->info.height / charter->priv->resize_factor),
        IPL_DEPTH_8U, 3);
  }
}

//static unsigned long getMillisecondsTime()
static int getMillisecondsTime()
{
  struct timeval tv;
  if(gettimeofday(&tv, NULL) != 0){
    return 0;
  }
  //return (unsigned long)((tv.tv_sec * 1000ul) + (tv.tv_usec / 1000ul));
  return (int)((tv.tv_sec * 1000ul) + (tv.tv_usec / 1000ul));
  //return 123;
}

static GstStructure *createData(CvRect *r, gdouble resize_factor, guint value, std::string overlay){
    GstStructure *result;

    result = gst_structure_new ("face",
        "x", G_TYPE_UINT, (guint) (r->x * resize_factor),
        "y", G_TYPE_UINT, (guint) (r->y * resize_factor),
        "width", G_TYPE_UINT,
        (guint) (r->width * resize_factor), "height",
        G_TYPE_UINT, (guint) (r->height * resize_factor),
        "b", G_TYPE_UINT, (guint) (255),
        "g", G_TYPE_UINT, (guint) (0),
        "r", G_TYPE_UINT, (guint) (255),
        "d", G_TYPE_UINT, (guint) (getMillisecondsTime()),
			      //			      "overlay", G_TYPE_STRING, "/opt/prope.png",	
			      //			      			      "overlay", G_TYPE_STRING, "/opt/faerie2.bmp",	
			      "overlay", G_TYPE_STRING, 
				//				"/opt/prope.bmp",	

				overlay.c_str(),	

        NULL);


return result;
}

static void setData(GstStructure *container, GstStructure *data, int i){
    gchar *id = NULL;
    id = g_strdup_printf ("%d", i);
    gst_structure_set (container, id, GST_TYPE_STRUCTURE, data, NULL);
    gst_structure_free (data);
    g_free (id);
}

static void
kms_charter_metadata_send_metadata (KmsCharterMetadata *
    charter, GstVideoFrame * frame)
{
  GstStructure *faces;
  GstStructure *timestamp;
  gint i;

  faces = gst_structure_new_empty ("faces");

  timestamp = gst_structure_new ("time",
      "pts", G_TYPE_UINT64, GST_BUFFER_PTS (frame->buffer),
      "dts", G_TYPE_UINT64, GST_BUFFER_DTS (frame->buffer), NULL);
  gst_structure_set (faces, "timestamp", GST_TYPE_STRUCTURE, timestamp, NULL);
  gst_structure_free (timestamp);

  if(!charter->priv->pFaceRectSeq || charter->priv->pFaceRectSeq->total == 0){
    GstStructure *data;
    CvRect r = cvRect(0, 0, 100, 100);
    std::string overlay = std::string("/opt/prope.bmp");
    data = createData(&r, (gdouble)charter->priv->resize_factor, getMillisecondsTime(), overlay);
    setData(faces, data, 0);
  }
else
  for (i = 0;
      i <
      (charter->priv->pFaceRectSeq ? charter->priv->
          pFaceRectSeq->total : 0); i++) {
    CvRect *r;
    GstStructure *data;
//    gchar *id = NULL;

    r = (CvRect *) cvGetSeqElem (charter->priv->pFaceRectSeq, i);
    data = createData(r, charter->priv->resize_factor, getMillisecondsTime(), std::string("/opt/prope.bmp"));
 setData(faces, data, i);

#if 0
    face = gst_structure_new ("face",
        "x", G_TYPE_UINT, (guint) (r->x * charter->priv->resize_factor),
        "y", G_TYPE_UINT, (guint) (r->y * charter->priv->resize_factor),
        "width", G_TYPE_UINT,
        (guint) (r->width * charter->priv->resize_factor), "height",
        G_TYPE_UINT, (guint) (r->height * charter->priv->resize_factor),
        "b", G_TYPE_UINT, (guint) (255),
        "g", G_TYPE_UINT, (guint) (0),
        "r", G_TYPE_UINT, (guint) (255),
        "d", G_TYPE_UINT, (guint) (getMillisecondsTime()),
			      //			      "overlay", G_TYPE_STRING, "/opt/prope.png",	
			      //			      			      "overlay", G_TYPE_STRING, "/opt/faerie2.bmp",	
			      "overlay", G_TYPE_STRING, "/opt/prope.bmp",	

        NULL);

    id = g_strdup_printf ("%d", i);
    gst_structure_set (faces, id, GST_TYPE_STRUCTURE, face, NULL);
    gst_structure_free (face);
    g_free (id);
#endif
  }
  /* send faces detected as metadata */
  kms_buffer_add_serializable_meta (frame->buffer, faces);
}

static GstFlowReturn
kms_charter_metadata_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame)
{
  KmsCharterMetadata *charter = KMS_CHARTER_METADATA (filter);
  GstMapInfo info;

  if ((charter->priv->haar_detector)
      && (charter->priv->pCascadeFace == NULL)) {
    return GST_FLOW_OK;
  }

  kms_charter_metadata_initialize_images (charter, frame);
  gst_buffer_map (frame->buffer, &info, GST_MAP_READ);

  charter->priv->cvImage->imageData = (char *) info.data;
  cvResize (charter->priv->cvImage, charter->priv->cvResizedImage,
      CV_INTER_LINEAR);

  g_mutex_lock (&charter->priv->mutex);

  if (charter->priv->qos_control) {
    charter->priv->throw_frames++;
    GST_DEBUG ("Filter is too slow. Frame dropped %d",
        charter->priv->throw_frames);
    g_mutex_unlock (&charter->priv->mutex);
    goto send;
  }

  g_mutex_unlock (&charter->priv->mutex);

  cvClearSeq (charter->priv->pFaceRectSeq);
  cvClearMemStorage (charter->priv->pStorageFace);
  if (charter->priv->haar_detector) {
    charter->priv->pFaceRectSeq =
        cvHaarDetectObjects (charter->priv->cvResizedImage,
        charter->priv->pCascadeFace, charter->priv->pStorageFace, 1.2,
        3, CV_HAAR_DO_CANNY_PRUNING,
        cvSize (charter->priv->cvResizedImage->width / 20,
            charter->priv->cvResizedImage->height / 20),
        cvSize (charter->priv->cvResizedImage->width / 2,
            charter->priv->cvResizedImage->height / 2));

  }

send:
  //  if (charter->priv->pFaceRectSeq->total != 0) 
    {
    kms_charter_metadata_send_metadata (charter, frame);
  }

  gst_buffer_unmap (frame->buffer, &info);

  return GST_FLOW_OK;
}

static void
kms_charter_metadata_finalize (GObject * object)
{
  KmsCharterMetadata *charter = KMS_CHARTER_METADATA (object);

  cvReleaseImageHeader (&charter->priv->cvImage);
  cvReleaseImage (&charter->priv->cvResizedImage);

  if (charter->priv->pStorageFace != NULL)
    cvClearMemStorage (charter->priv->pStorageFace);
  if (charter->priv->pFaceRectSeq != NULL)
    cvClearSeq (charter->priv->pFaceRectSeq);

  cvReleaseMemStorage (&charter->priv->pStorageFace);
  cvReleaseHaarClassifierCascade (&charter->priv->pCascadeFace);

  g_mutex_clear (&charter->priv->mutex);

  G_OBJECT_CLASS (kms_charter_metadata_parent_class)->finalize (object);
}

static void
kms_charter_metadata_init (KmsCharterMetadata * charter)
{
  std::cout << "\n\n\nGO kms_charter_metadata_init" << std::endl;  
  charter->priv = KMS_CHARTER_METADATA_GET_PRIVATE (charter);

  charter->priv->pCascadeFace = NULL;
  charter->priv->pStorageFace = cvCreateMemStorage (0);
  charter->priv->pFaceRectSeq =
      cvCreateSeq (0, sizeof (CvSeq), sizeof (CvRect),
      charter->priv->pStorageFace);
  charter->priv->show_debug_info = FALSE;
  charter->priv->qos_control = FALSE;
  charter->priv->throw_frames = 0;
  charter->priv->haar_detector = TRUE;
  charter->priv->cvImage = NULL;
  charter->priv->cvResizedImage = NULL;
  g_mutex_init (&charter->priv->mutex);

  kms_charter_metadata_initialize_classifiers (charter);
}

static gboolean
kms_charter_metadata_src_eventfunc (GstBaseTransform * trans,
    GstEvent * event)
{
  KmsCharterMetadata *charter = KMS_CHARTER_METADATA (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:
    {
      gdouble proportion;
      GstClockTimeDiff diff;
      GstClockTime timestamp;
      GstQOSType type;
      gfloat difference;

      gst_event_parse_qos (event, &type, &proportion, &diff, &timestamp);
      gst_base_transform_update_qos (trans, proportion, diff, timestamp);
      difference = (((gfloat) (gint) diff) / (gfloat) GST_SECOND);

      g_mutex_lock (&charter->priv->mutex);

      if (difference > MIN_TIME) {
        if (charter->priv->throw_frames <= MIN_FPS) {
          charter->priv->qos_control = TRUE;
        } else {
          charter->priv->qos_control = FALSE;
          charter->priv->throw_frames = 0;
        }
      } else {
        charter->priv->qos_control = FALSE;
        charter->priv->throw_frames = 0;
      }

      g_mutex_unlock (&charter->priv->mutex);

      break;
    }
    default:
      break;
  }

  return gst_pad_push_event (trans->sinkpad, event);
}

static void
kms_charter_metadata_class_init (KmsCharterMetadataClass * klass)
{
  std::cout << "\n\n\nGO kms_charter_metadata_class_init" << std::endl;

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, PLUGIN_NAME);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Face Detector element", "Video/Filter",
      "Detect faces in an image", "David Fernandez <d.fernandezlop@gmail.com>");

  gobject_class->set_property = kms_charter_metadata_set_property;
  gobject_class->get_property = kms_charter_metadata_get_property;
  gobject_class->finalize = kms_charter_metadata_finalize;

  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (kms_charter_metadata_transform_frame_ip);

  /* Properties initialization */
  g_object_class_install_property (gobject_class, PROP_SHOW_DEBUG_INFO,
      g_param_spec_boolean ("show-debug-region", "show debug region",
          "show evaluation regions over the image", FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FILTER_VERSION,
      g_param_spec_boolean ("filter-version", "filter version",
          "True means filter based on haar detector. False filter based on lbp",
          TRUE, G_PARAM_READWRITE));

  klass->base_charter_class.parent_class.src_event =
      GST_DEBUG_FUNCPTR (kms_charter_metadata_src_eventfunc);

  g_type_class_add_private (klass, sizeof (KmsCharterMetadataPrivate));
}

gboolean
kms_charter_metadata_plugin_init (GstPlugin * plugin)
{
  std::cout << "\n\n\nGO kms_charter_metadata_plugin_init" << std::endl;


  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      KMS_TYPE_CHARTER_METADATA);
}
