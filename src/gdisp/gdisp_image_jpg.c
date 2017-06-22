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
    size_t      file_size;
	size_t		frame0pos;
	pixel_t		*frame0cache;
    size_t      pixbuf_size;
    size_t      decode_size;
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

	/* Allocate our private area */
	if (!(img->priv = gdispImageAlloc(img, sizeof(gdispImagePrivate_JPG))))
		return GDISP_IMAGE_ERR_NOMEMORY;

	/* Initialise the essential bits in the private area */
	priv = (gdispImagePrivate_JPG *)img->priv;

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

    priv->file_size   = gfileGetSize(img->f);
    priv->pixbuf_size = img->width * img->height * sizeof(pixel_t);
    priv->decode_size = img->width * img->height * 3;

	img->type = GDISP_IMAGE_TYPE_JPG;

    priv->frame0cache = 0;
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

typedef struct {
    unsigned char *inData;
    int inPos;
    unsigned char *outData;
    int outW;
    int outH;
} JpegDev;

static UINT infunc(JDEC *decoder, BYTE *buf, UINT len)
{
    JpegDev *jd = (JpegDev *)decoder->device;
    /*printf("Reading %d bytes from pos %d\n", len, jd->inPos);*/
    if (buf != NULL) {
        memcpy(buf, jd->inData + jd->inPos, len);
    }
    jd->inPos += len;
    return len;
}

static UINT outfunc(JDEC *decoder, void *bitmap, JRECT *rect)
{
    unsigned char *in = (unsigned char *)bitmap;
    unsigned char *out;
    int y;
    /*printf("Rect %d,%d - %d,%d\n", rect->top, rect->left, rect->bottom, rect->right);*/
    JpegDev *jd = (JpegDev *)decoder->device;
    for (y = rect->top; y <= rect->bottom; y++) {
        out = jd->outData + ((jd->outW * y) + rect->left) * 3;
        memcpy(out, in, ((rect->right - rect->left) + 1) * 3);
        in += ((rect->right - rect->left) + 1) * 3;
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

    JDEC decoder;
    JpegDev jd;
    int r;
    int mx, my;
    unsigned char *decoded = gdispImageAlloc(img, priv->decode_size);
    if(!decoded){
        return GDISP_IMAGE_ERR_NOMEMORY;
    }
    char *work = gdispImageAlloc(img, WORKSZ);
    if(!work){
        return GDISP_IMAGE_ERR_NOMEMORY;
    }
    memset(work, 0, WORKSZ);

    jd.inPos = 0;
    jd.outData = decoded;
    jd.outW = img->width;
    jd.outH = img->height;
    jd.inData = gdispImageAlloc(img, priv->file_size);
    if(!jd.inData){
        return GDISP_IMAGE_ERR_NOMEMORY;
    }

    gfileSetPos(img->f, 0);
    gfileRead(img->f, jd.inData, priv->file_size);
    /*printf("Read %u bytes\n", priv->file_size);*/

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

    if((r = jd_prepare(&decoder, infunc, work, WORKSZ, (void *)&jd))){
        return decode_errors[r];
    }
    if((r = jd_decomp(&decoder, outfunc, 0))){
        return decode_errors[r];
    }

    unsigned char *p = decoded + 2;
    priv->frame0pos = 0;
    for (my = 0; my < img->height; my++) {
        for (mx = 0; mx < img->width; mx++) {
            // BW only
            /*if(*p > 100){*/
            /*priv->frame0cache[priv->frame0pos] = White;*/
            /*} else {*/
            /*priv->frame0cache[priv->frame0pos] = Black;*/
            /*}*/
            // RGB
            priv->frame0cache[priv->frame0pos] = RGB2COLOR(((uint8_t *)p)[0], ((uint8_t *)p)[1], ((uint8_t *)p)[2]);

            p += 3;
            priv->frame0pos++;
        }
    }

    gdispImageFree(img, (void*) work, WORKSZ);
    gdispImageFree(img, (void*) decoded, priv->decode_size);
    gdispImageFree(img, (void*) jd.inData, priv->file_size);

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
