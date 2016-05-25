/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
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



#include <opencv/cv.h>
#include <opencv/highgui.h>



static cv::Mat auximg;

#include "kmsgraphsmetadata.h"

#include <commons/kmsserializablemeta.h>

#define TEMP_PATH "/tmp/XXXXXX"
#define BLUE_COLOR (cvScalar (255, 0, 0, 0))
#define SRC_OVERLAY ((double)1)

#define PLUGIN_NAME "graphsmetadata"
                     
GST_DEBUG_CATEGORY_STATIC (kms_graphs_metadata_debug_category);
#define GST_CAT_DEFAULT kms_graphs_metadata_debug_category

#define KMS_GRAPHS_METADATA_GET_PRIVATE(obj) ( \
  G_TYPE_INSTANCE_GET_PRIVATE (              \
    (obj),                                   \
    KMS_TYPE_GRAPHS_METADATA,                  \
    KmsGraphsMetadataPrivate                   \
  )                                          \
)

enum
{
  PROP_0,
  PROP_SHOW_DEBUG_INFO
};

struct _KmsGraphsMetadataPrivate
{
  GstElement *text_overlay;
  IplImage *cvImage;

  gdouble offsetXPercent, offsetYPercent, widthPercent, heightPercent;
  gboolean show_debug_info;
};

typedef struct _MsMetadata2{
  CvRect rect;
  int data;
  char* augmentable;
} MsMetadata2;

static GHashTable* myhash;
//IplImage *costumeAux;
static cv::Mat costumeAux;


/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (KmsGraphsMetadata, kms_graphs_metadata,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (kms_graphs_metadata_debug_category,
        PLUGIN_NAME, 0, "debug category for graphs element"));


static void
kms_graphs_metadata_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  KmsGraphsMetadata *graphs = KMS_GRAPHS_METADATA (object);

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
kms_graphs_metadata_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  KmsGraphsMetadata *graphs = KMS_GRAPHS_METADATA (object);

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

#if 0
static void addFgWithAlpha(cv::Mat &bg, cv::Mat &fg) {
if (fg.channels() < 4) {
fg.copyTo(bg);  
     return;
   }
   std::vector<cv::Mat> splitted_bg, splitted_fg;
   cv::split(bg, splitted_bg);
   cv::split(fg, splitted_fg);
   cv::Mat mask = splitted_fg[3];
   cv::Mat invmask = ~mask;
   splitted_bg[0] = (splitted_bg[0] - mask) + (splitted_fg[0] - invmask);
   splitted_bg[1] = (splitted_bg[1] - mask) + (splitted_fg[1] - invmask);
   splitted_bg[2] = (splitted_bg[2] - mask) + (splitted_fg[2] - invmask);
   if (bg.channels() > 3) {
     splitted_bg[3] = splitted_bg[3] + splitted_fg[3];
   }
   cv::merge(splitted_bg, bg);
 }
#endif

#if 0
static void inject(cv::Mat dstMat, MsMetadata2 *r){
//std::cout << "INJECTING..." << std::endl;
//Gnuplot gp;

  int font = cv::FONT_HERSHEY_PLAIN;
  int font_thickness = 3;
  double font_scale = 6.0;
  int baseline=0;

  std::stringstream ss;
  ss << r->data;
  std::string txt = ""; //"JES";
  std::string tmp = ss.str();
  txt += tmp.substr(tmp.length()/2, tmp.length());

cv::Size textSize = cv::getTextSize(txt.c_str(), font, font_scale, font_thickness, &baseline);
  cv::Mat fg = cv::Mat(textSize.height+baseline+4, textSize.width+4, CV_8UC4);
  fg.setTo(cv::Scalar(255,255,255,64));
cv::putText(fg, txt.c_str(), cv::Point(2, fg.rows-(baseline/2)), font, font_scale, cv::Scalar(0,64,0,255), font_thickness);



cv::resize(fg, fg, cv::Size(dstMat.cols, dstMat.rows));
addFgWithAlpha(dstMat, fg);
}
#endif




static void
kms_graphs_metadata_initialize_images (KmsGraphsMetadata *
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

static GSList *
receiveMetadata(GstStructure * faces)
{
  gint len, aux;
  GSList *list = NULL;

  len = gst_structure_n_fields (faces);

  for (aux = 0; aux < len; aux++) {
    GstStructure *face;
    gboolean ret;

    const gchar *name = gst_structure_nth_field_name (faces, aux);

    if (g_strcmp0 (name, "timestamp") == 0) {
      continue;
    }

    ret = gst_structure_get (faces, name, GST_TYPE_STRUCTURE, &face, NULL);

    if (ret) {

      MsMetadata2 *aux = g_slice_new0 (MsMetadata2);
      gst_structure_get (face, "x", G_TYPE_UINT, &aux->rect.x, NULL);
      gst_structure_get (face, "y", G_TYPE_UINT, &aux->rect.y, NULL);
      gst_structure_get (face, "width", G_TYPE_UINT, &aux->rect.width, NULL);
      gst_structure_get (face, "height", G_TYPE_UINT, &aux->rect.height, NULL);
      gst_structure_get (face, "data", G_TYPE_UINT, &aux->data, NULL);
#if 0      
      gst_structure_get (face, "overlay", G_TYPE_STRING, &aux->augmentable, NULL);
      if(g_hash_table_contains(myhash, aux->augmentable) == FALSE){


	costumeAux = cvLoadImage (aux->augmentable, CV_LOAD_IMAGE_UNCHANGED);	
	g_hash_table_add(myhash, aux->augmentable);

	std::cout << "\nTHE IMAGE: " << aux->augmentable << " " <<
	  costumeAux.depth() << " " << costumeAux.channels() << " " << costumeAux.cols << " " << costumeAux.rows << std::endl;
      } 
      else{
      }
#endif
      gst_structure_free (face);
      list = g_slist_append (list, aux);

    }
  }
  return list;
}

static void
cvrect_free (gpointer data)
{
  g_slice_free (MsMetadata2, data);
}






static void goVis(KmsGraphsMetadata *graphs, cv::Mat feature_vis, MsMetadata2 *r){
  srand(time(NULL));
  int windowWidth = 640;
  int windowHeight = 480;
  //int memoryLength = 135;
  int memoryLength = 256;
  int randomNumber;
  std::vector<int> numbers;
  for (int i = 0; i<memoryLength; i++) {
    randomNumber = rand() % 400 + 1;
    numbers.push_back(randomNumber);
  }

  //while (true) 
    {
    //numbers.push_back(rand() % 400 + 1);

    //cv::Mat feature_vis = graphs->priv->cvImage;

    //cv::Mat feature_vis = cv::Mat::zeros(windowHeight, windowWidth, CV_8UC3);
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
    numbers.erase(numbers.begin());
    numbers.push_back(rand() % 400 + 1);

    //cv::imshow("graph", feature_vis);

    //if (cv::waitKey(10) > 0) {
      // Press any key to quit
      //break;
    //}
  }

#if 0

  //std::cout << "\njestas: " << std::endl;
  int w = 50;
  int h = 200;
  cv::Mat roi(graphs->priv->cvImage, cv::Rect(0, 0, w, h));
      cv::Mat dstMat = roi.clone();
      cv::resize(auximg, dstMat, cv::Size(w, h));

      dstMat.copyTo(roi);       

      int max = 165;
      int min = 100;
      int randNum = rand()%(max-min + 1) + min;


      cvRectangle(graphs->priv->cvImage, cvPoint(20, randNum), cvPoint(25, 165), cvScalar(0,0,255,255), -1);
#endif
}

static void
    kms_graphs_metadata_display_detections_overlay_img
    (KmsGraphsMetadata * graphs, const GSList * faces_list)
{
const GSList *iterator = NULL;
  
  for (iterator = faces_list; iterator; iterator = iterator->next) {

      MsMetadata2 *r = (MsMetadata2*)iterator->data;
      goVis(graphs, graphs->priv->cvImage, r);

#if 0
      cv::Mat roi(graphs->priv->cvImage, cv::Rect(r->rect.x, r->rect.y, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));
      inject(dstMat, r);
      dstMat.copyTo(roi);       
#endif

#if 0
    if(costumeAux.data == NULL){
      MsMetadata2 *r = (MsMetadata2*)iterator->data;
      cv::Mat roi(graphs->priv->cvImage, cv::Rect(0, 0, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      //cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));

inject(dstMat, r);

      dstMat.copyTo(roi);       

      cvRectangle (graphs->priv->cvImage, cvPoint (0, 0),
		   cvPoint (r->rect.width, r->rect.height), cvScalar (0, 255, 255, 0), 3,
        8, 0);
    }
    else{
      MsMetadata2 *r = (MsMetadata2*)iterator->data;
      /*
      cvRectangle (graphs->priv->cvImage, cvPoint (0, 0),
		   cvPoint (r->rect.width, r->rect.height), cvScalar (0, 255, 255, 0), 3,
        8, 0);
      */
      cv::Mat roi(graphs->priv->cvImage, cv::Rect(r->rect.x, r->rect.y, r->rect.width, r->rect.height));
      cv::Mat dstMat = roi.clone();
      cv::resize(costumeAux, dstMat, cv::Size(r->rect.width, r->rect.height));

inject(dstMat, r);

      dstMat.copyTo(roi);       
    }
#endif
  }
}

static GstFlowReturn
kms_graphs_metadata_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame)
{
  KmsGraphsMetadata *graphs = KMS_GRAPHS_METADATA (filter);
  GstMapInfo info;
  KmsSerializableMeta *metadata;
  GSList *faces_list;

  gst_buffer_map (frame->buffer, &info, GST_MAP_READ);

  kms_graphs_metadata_initialize_images (graphs, frame);
  graphs->priv->cvImage->imageData = (char *) info.data;

  GST_OBJECT_LOCK (graphs);
  /* check if the buffer has metadata */
  metadata = kms_buffer_get_serializable_meta (frame->buffer);

  if (metadata == NULL) 
    {

    /*
    if(!foobar){
      foobar = !foobar;
      //std::string overlay = std::string("/opt/temperature0.bmp");
      std::string overlay = loadPlanar("http://ssi.vtt.fi/temperature0.bmp");

      auximg = cvLoadImage (overlay.c_str(), CV_LOAD_IMAGE_UNCHANGED);
      std::cout << "\nTHE IMAGE: " << overlay << " " <<
	auximg.depth() << " " << auximg.channels() << " " << auximg.cols << " " << auximg.rows << std::endl;
    }
    */
      //std::cout << "\nSHOW GRAPH" << std::endl;
      //    goVis(graphs, graphs->priv->cvImage);
    goto end;
  }

  faces_list = receiveMetadata (metadata->data);

  if (faces_list != NULL) {
    kms_graphs_metadata_display_detections_overlay_img (graphs,
        faces_list);
    g_slist_free_full (faces_list, cvrect_free);
  }

end:
  GST_OBJECT_UNLOCK (graphs);

  gst_buffer_unmap (frame->buffer, &info);

  return GST_FLOW_OK;
}

static void
kms_graphs_metadata_dispose (GObject * object)
{
  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (kms_graphs_metadata_parent_class)->dispose (object);
}

static void
kms_graphs_metadata_finalize (GObject * object)
{
  KmsGraphsMetadata *graphs = KMS_GRAPHS_METADATA (object);

  if (graphs->priv->cvImage != NULL)
    cvReleaseImage (&graphs->priv->cvImage);

  G_OBJECT_CLASS (kms_graphs_metadata_parent_class)->finalize (object);
}


static void
kms_graphs_metadata_init (KmsGraphsMetadata * graphs)
{
  graphs->priv = KMS_GRAPHS_METADATA_GET_PRIVATE (graphs);

  graphs->priv->show_debug_info = FALSE;
  graphs->priv->cvImage = NULL;
}

static void
kms_graphs_metadata_class_init (KmsGraphsMetadataClass * klass)
{
  std::cout << "\n\n\nGO sgd_class_init" << std::endl;
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
      "image overlay element", "Video/Filter",
      "Set a defined image in a defined position",
      "David Fernandez <d.fernandezlop@gmail.com>");

  gobject_class->set_property = kms_graphs_metadata_set_property;
  gobject_class->get_property = kms_graphs_metadata_get_property;
  gobject_class->dispose = kms_graphs_metadata_dispose;
  gobject_class->finalize = kms_graphs_metadata_finalize;

  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (kms_graphs_metadata_transform_frame_ip);

  /* Properties initialization */
  g_object_class_install_property (gobject_class, PROP_SHOW_DEBUG_INFO,
      g_param_spec_boolean ("show-debug-region", "show debug region",
          "show evaluation regions over the image", FALSE, G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (KmsGraphsMetadataPrivate));

  myhash = g_hash_table_new(g_str_hash, g_str_equal);
  /*
#if 0
  std::string overlay = std::string("/opt/temperature0.bmp");
    auximg = cvLoadImage (overlay.c_str(), CV_LOAD_IMAGE_UNCHANGED);	
    std::cout << "\nTHE IMAGE: " << overlay << " " <<
      auximg.depth() << " " << auximg.channels() << " " << auximg.cols << " " << auximg.rows << std::endl;
#endif
  */
}

gboolean
kms_graphs_metadata_plugin_init (GstPlugin * plugin)
{
  std::cout << "\n\n\nGO sgd_plugin_init " << PLUGIN_NAME << std::endl;
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      KMS_TYPE_GRAPHS_METADATA);
}
