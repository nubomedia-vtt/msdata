/**
Licensing and distribution

ArModule is licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

ALVAR 2.0.0 - A Library for Virtual and Augmented Reality Copyright 2007-2012 VTT Technical Research Centre of Finland Licensed under the GNU Lesser General Public License

Irrlicht Engine, the zlib and libpng. The Irrlicht Engine is based in part on the work of the Independent JPEG Group The module utilizes IJG code when the Irrlicht engine is compiled with support for JPEG images.
*/

/** @author Markus Ylikerälä */

#ifndef _KMS_MSDATA_H_
#define _KMS_MSDATA_H_

#include <gst/gst.h>
#include "commons/kmselement.h"

G_BEGIN_DECLS
#define KMS_TYPE_MSDATA   (kms_msdata_get_type())
#define KMS_MSDATA(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),KMS_TYPE_MSDATA,KmsMsData))
#define KMS_MSDATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),KMS_TYPE_MSDATA,KmsMsDataClass))
#define KMS_IS_MSDATA(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),KMS_TYPE_MSDATA))
#define KMS_IS_MSDATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),KMS_TYPE_MSDATA))
typedef struct _KmsMsData KmsMsData;
typedef struct _KmsMsDataClass KmsMsDataClass;
typedef struct _KmsMsDataPrivate KmsMsDataPrivate;

struct _KmsMsData
{
  KmsElement base;
  KmsMsDataPrivate *priv;
};

struct _KmsMsDataClass
{
  KmsElementClass parent_class;
};

GType kms_msdata_get_type (void);

gboolean kms_msdata_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* _KMS_MSDATA_H_ */
