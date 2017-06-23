/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "../../gfx.h"
#include "rom/tjpgd.h"

#if GFX_USE_GDISP && GDISP_NEED_IMAGE && GDISP_NEED_IMAGE_JPG

#include "gdisp_image_support.h"

#define WORKSZ 3100

typedef struct gdispImagePrivate_JPG {
	size_t		in_pos;
	size_t		out_pos;
	pixel_t		*frame0cache;
    size_t      pixbuf_size;
	} gdispImagePrivate_JPG;

gdispImageError gdispImageOpen_JPG(gdispImage *img){
    gdispImagePrivate_JPG *priv;
	uint8_t		hdr[4];

	/* Read the file identifier */
	if (gfileRead(img->f, hdr, 2) != 2)
		return GDISP_IMAGE_ERR_BADFORMAT;		// It can't be us

	/* Process the JPEG header structure */

    if (hdr[0] != 0xff || hdr[1] != 0xd8)
		return GDISP_IMAGE_ERR_BADFORMAT;		// It can't be us

	/* We know we are a JPG format image */
	img->flags = 0;

    /* Process Start of frame segments */
    while(!gfileEOF(img->f)){
        hdr[0] = hdr[1];
        gfileRead(img->f, hdr+1, 1);
        if((hdr[0] == 0xff && (hdr[1] == 0xc0 || hdr[1] == 0xc2))){
            gfileRead(img->f, hdr, 3);
            gfileRead(img->f, hdr, 4);
            img->height = gdispImageGetAlignedBE16(hdr, 0);
            img->width = gdispImageGetAlignedBE16(hdr, 2);
            /*printf("Found SOF header at %u, width: %u, height: %u\n", gfileGetPos(img->f)-7, img->width, img->height);*/
            goto header_found;
        }
    }
    return GDISP_IMAGE_ERR_BADFORMAT;

header_found:

	/* Allocate our private area */
	if (!(img->priv = gdispImageAlloc(img, sizeof(gdispImagePrivate_JPG))))
		return GDISP_IMAGE_ERR_NOMEMORY;

	/* Initialise the essential bits in the private area */
	priv = (gdispImagePrivate_JPG *)img->priv;
    priv->pixbuf_size = img->width * img->height * sizeof(pixel_t);
    priv->frame0cache = 0;

	img->type = GDISP_IMAGE_TYPE_JPG;

	return GDISP_IMAGE_ERR_OK;
}

void gdispImageClose_JPG(gdispImage *img){
	gdispImagePrivate_JPG *priv = (gdispImagePrivate_JPG *)img->priv;
    if(priv){
        if (priv->frame0cache){
            gdispImageFree(img, (void *)priv->frame0cache, priv->pixbuf_size);
        }
        gdispImageFree(img, (void*) priv, sizeof(gdispImagePrivate_JPG));
    }
}

static UINT infunc(JDEC *decoder, BYTE *buf, UINT len)
{
	gdispImage * img = (gdispImage *)decoder->device;
	gdispImagePrivate_JPG *	priv = (gdispImagePrivate_JPG *)img->priv;
    if(buf){
        gfileSetPos(img->f, priv->in_pos);
        gfileRead(img->f, buf, len);
    }
    priv->in_pos += len;
    return len;
}

static UINT outfunc(JDEC *decoder, void *bitmap, JRECT *rect)
{
    unsigned char *in = (unsigned char *)bitmap;
    unsigned char *out;
    int y;
	gdispImage * img = (gdispImage *)decoder->device;
	gdispImagePrivate_JPG *	priv = (gdispImagePrivate_JPG *)img->priv;
    for (y = rect->top; y <= rect->bottom; y++) {
        priv->out_pos = ((img->width * y) + rect->left);
        for(size_t pixel = 0; pixel < ((rect->right - rect->left) + 1); pixel++){
            priv->frame0cache[priv->out_pos+pixel] = RGB2COLOR(in[0], in[1], in[2]);
            in += 3;
        }
    }
    return 1;
}

gdispImageError gdispImageCache_JPG(gdispImage *img){
	gdispImagePrivate_JPG *	priv;

	/* If we are already cached - just return OK */
	priv = (gdispImagePrivate_JPG *)img->priv;
	if (priv->frame0cache)
		return GDISP_IMAGE_ERR_OK;

    /* Otherwise start a new decode */
	priv->frame0cache = (pixel_t *)gdispImageAlloc(img, priv->pixbuf_size);
	if (!priv->frame0cache)
		return GDISP_IMAGE_ERR_NOMEMORY;

    char *work = gdispImageAlloc(img, WORKSZ);
    if(!work){
        return GDISP_IMAGE_ERR_NOMEMORY;
    }
    memset(work, 0, WORKSZ);

    priv->in_pos = 0;
    priv->out_pos = 0;
    gfileSetPos(img->f, 0);

    gdispImageError decode_errors[9] = {GDISP_IMAGE_ERR_OK,          /* 0 : JDR_OK */
                                        GDISP_IMAGE_ERR_BADDATA,     /* 1 : JDR_INTR */
                                        GDISP_IMAGE_ERR_BADDATA,     /* 2 : JDR_INP */
                                        GDISP_IMAGE_ERR_NOMEMORY,    /* 3 : JDR_MEM1 */
                                        GDISP_IMAGE_ERR_NOMEMORY,    /* 4 : JDR_MEM2 */
                                        GDISP_IMAGE_ERR_UNSUPPORTED, /* 5 : JDR_PAR */
                                        GDISP_IMAGE_ERR_BADFORMAT,   /* 6 : JDR_FMT1 */
                                        GDISP_IMAGE_ERR_BADFORMAT,   /* 7 : JDR_FMT2 */
                                        GDISP_IMAGE_ERR_BADFORMAT,   /* 8 : JDR_FMT3 */
                                       };

    int r;
    JDEC decoder;
    if((r = jd_prepare(&decoder, infunc, work, WORKSZ, (void *)img))){
        return decode_errors[r];
    }
    if((r = jd_decomp(&decoder, outfunc, 0))){
        return decode_errors[r];
    }

    gdispImageFree(img, (void*) work, WORKSZ);

	return GDISP_IMAGE_ERR_OK;
}

gdispImageError gdispGImageDraw_JPG(GDisplay *g, gdispImage *img, coord_t x, coord_t y, coord_t cx, coord_t cy, coord_t sx, coord_t sy){
    gdispImagePrivate_JPG *	priv;

    priv = (gdispImagePrivate_JPG *)img->priv;

    /* Check some reasonableness */
    if (sx >= img->width || sy >= img->height) return GDISP_IMAGE_ERR_OK;
    if (sx + cx > img->width) cx = img->width - sx;
    if (sy + cy > img->height) cy = img->height - sy;

    /* Cache the image if not already cached */
    if (!priv->frame0cache) {
        gdispImageError err = gdispImageCache_JPG(img);
        if(err){
            return err;
        }
    }
    gdispGBlitArea(g, x, y, cx, cy, sx, sy, img->width, priv->frame0cache);

    return GDISP_IMAGE_ERR_OK;
}

delaytime_t gdispImageNext_JPG(gdispImage *img) {
	/* No more frames/pages */
	return TIME_INFINITE;
}

#endif /* GFX_USE_GDISP && GDISP_NEED_IMAGE && GDISP_NEED_IMAGE_JPG */
