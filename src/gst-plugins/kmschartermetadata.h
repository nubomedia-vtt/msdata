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

#ifndef _KMS_CHARTER_METADATA_H_
#define _KMS_CHARTER_METADATA_H_

#include <gst/video/gstvideofilter.h>
#include <opencv/cv.h>

G_BEGIN_DECLS

#define KMS_TYPE_CHARTER_METADATA   (kms_charter_metadata_get_type())
#define KMS_CHARTER_METADATA(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),KMS_TYPE_CHARTER_METADATA,KmsCharterMetadata))
#define KMS_CHARTER_METADATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),KMS_TYPE_CHARTER_METADATA,KmsCharterMetadataClass))
#define KMS_IS_CHARTER_METADATA(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),KMS_TYPE_CHARTER_METADATA))
#define KMS_IS_CHARTER_METADATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),KMS_TYPE_CHARTER_METADATA))

typedef struct _KmsCharterMetadata KmsCharterMetadata;
typedef struct _KmsCharterMetadataClass KmsCharterMetadataClass;
typedef struct _KmsCharterMetadataPrivate KmsCharterMetadataPrivate;

struct _KmsCharterMetadata
{
  GstVideoFilter base;

  KmsCharterMetadataPrivate *priv;
};

struct _KmsCharterMetadataClass
{
  GstVideoFilterClass base_charter_class;
};

GType kms_charter_metadata_get_type (void);

gboolean kms_charter_metadata_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif  /* _KMS_CHARTER_METADATA_H_ */
