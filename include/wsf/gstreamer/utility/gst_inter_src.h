/*
 * Copyright (C) 2015 Centricular Ltd.
 *     Author: Sebastian Dröge <sebastian@centricular.com>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#ifndef __GST_INTER_SRC_H__
#define __GST_INTER_SRC_H__

#include <gst/gst.h>

#ifndef __GTK_DOC_IGNORE__

G_BEGIN_DECLS

//#define OWR_TYPE_INTER_SRC            (_owr_inter_src_get_type())
#define GST_TYPE_INTER_SRC            (gst_inter_src_get_type())
#define GST_INTER_SRC(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_INTER_SRC, GstInterSrc))
#define GST_IS_INTER_SRC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_INTER_SRC))
#define GST_INTER_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) , GST_TYPE_INTER_SRC, GstInterSrcClass))
#define GST_IS_INTER_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) , GST_TYPE_INTER_SRC))
#define GST_INTER_SRC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) , GST_TYPE_INTER_SRC, GstInterSrcClass))

typedef struct _GstInterSrc      GstInterSrc;
typedef struct _GstInterSrcClass GstInterSrcClass;

struct _GstInterSrc {
    GstBin parent;

    GstElement *queue;
    GstPad *internal_srcpad, *dummy_sinkpad;
    GstPad *srcpad;
    GWeakRef sink_sinkpad;
};

struct _GstInterSrcClass {
    GstBinClass parent_class;
};

GType gst_inter_src_get_type(void);

G_END_DECLS

#endif /* __GTK_DOC_IGNORE__ */

#endif /* __OWR_INTER_SRC_H__ */
