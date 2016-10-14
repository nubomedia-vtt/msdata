/**
Licensing and distribution

ArModule is licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

ALVAR 2.0.0 - A Library for Virtual and Augmented Reality Copyright 2007-2012 VTT Technical Research Centre of Finland Licensed under the GNU Lesser General Public License

Irrlicht Engine, the zlib and libpng. The Irrlicht Engine is based in part on the work of the Independent JPEG Group The module utilizes IJG code when the Irrlicht engine is compiled with support for JPEG images.
*/

/** @author Markus Ylikerälä */

#include <glib/gstdio.h>
#include <iostream>

#include "kmsmsdata.h"

#include "kmsmsdatagraph.h"

GST_DEBUG_CATEGORY_STATIC (kms_msdata_debug_category);
#define GST_CAT_DEFAULT kms_msdata_debug_category
#define PLUGIN_NAME "kmsmsdata"

#define KMS_MSDATA_GET_PRIVATE(obj) (                    \
    G_TYPE_INSTANCE_GET_PRIVATE (                           \
        (obj),                                              \
        KMS_TYPE_MSDATA,                                 \
        KmsMsDataPrivate                                  \
    )                                                       \
)

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (KmsMsData, kms_msdata,
                         KMS_TYPE_ELEMENT,
                         GST_DEBUG_CATEGORY_INIT (kms_msdata_debug_category,
                             PLUGIN_NAME, 0,
                             "debug category for kms_msdata element") );

struct _KmsMsDataPrivate
{
  GstElement *imgelm;
};



static void
kms_msdata_connect_video (KmsMsData * self, GstElement * agnosticbin)
{
  GstPad *sink = gst_element_get_static_pad (self->priv->imgelm, "sink");

  kms_element_connect_sink_target (KMS_ELEMENT (self), sink,
				   KMS_ELEMENT_PAD_TYPE_VIDEO);
  gst_element_link (self->priv->imgelm, agnosticbin);
}

static void
kms_msdata_connect_audio (KmsMsData * self, GstElement * agnosticbin)
{
  GstPad *target = gst_element_get_static_pad (agnosticbin, "sink");

  kms_element_connect_sink_target (KMS_ELEMENT (self), target, KMS_ELEMENT_PAD_TYPE_AUDIO);
  g_object_unref (target);
}

static void
kms_msdata_connect_data (KmsMsData * self, GstElement * tee)
{
  GstElement *identity =  gst_element_factory_make ("identity", NULL);
  GstPad *identity_sink = gst_element_get_static_pad (identity, "sink");

  gst_bin_add (GST_BIN (self), identity);

  kms_element_connect_sink_target (KMS_ELEMENT (self), identity_sink, KMS_ELEMENT_PAD_TYPE_DATA);
  gst_element_link (identity, tee);

  g_signal_connect (identity, "handoff", G_CALLBACK (newMsDataValue), self->priv->imgelm);

  g_object_unref (identity_sink);
}

static void
kms_msdata_init (KmsMsData *self)
{
  self->priv = KMS_MSDATA_GET_PRIVATE (self);

  self->priv->imgelm = gst_element_factory_make ("kmsmsdatagraph", NULL);
  gst_bin_add (GST_BIN (self), self->priv->imgelm);

  kms_msdata_connect_video (self, kms_element_get_video_agnosticbin (KMS_ELEMENT (self)));
  kms_msdata_connect_audio (self, kms_element_get_audio_agnosticbin (KMS_ELEMENT (self)));
  kms_msdata_connect_data (self, kms_element_get_data_tee (KMS_ELEMENT (self)));

  //gst_element_sync_state_with_parent (self->priv->imgelm);
}

static void
kms_msdata_class_init (KmsMsDataClass *klass)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, PLUGIN_NAME);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "KmsMsData", "Generic",
      "Receives numeric data from the client and prints it in the image as a graph",
      "Markus Ylikerala <Markus.Ylikerala@vtt.fi>");

  g_type_class_add_private (klass, sizeof (KmsMsDataPrivate));
}

gboolean
kms_msdata_plugin_init (GstPlugin *plugin)
{
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
                               KMS_TYPE_MSDATA);
}
