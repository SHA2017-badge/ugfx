/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "../../gfx.h"
#include "png_reader.h"

#if GFX_USE_GDISP && GDISP_NEED_IMAGE && GDISP_NEED_IMAGE_PNG

#include "gdisp_image_support.h"

typedef struct gdispImagePrivate_PNG {
    pixel_t *               frame0cache;
    size_t                  pixbuf_size;
	struct lib_png_reader * pr;
    int                     bufsize;
    int                     buflen;
    int                     bufpos;
    uint8_t                 buf[0];
} gdispImagePrivate_PNG;

/*-----------------------------------------------------------------
 * Public PNG functions
 *---------------------------------------------------------------*/

void gdispImageClose_PNG(gdispImage *img) {
	gdispImagePrivate_PNG *priv;

	priv = (gdispImagePrivate_PNG *)img->priv;
	if (priv) {
		if(priv->pr){
			lib_png_destroy(priv->pr);
		}
		priv->pr = 0;
        if (priv->frame0cache){
            gdispImageFree(img, (void *)priv->frame0cache, priv->pixbuf_size);
        }
		priv->frame0cache = 0;
		gdispImageFree(img, (void *)priv, sizeof(gdispImagePrivate_PNG));
		img->priv = 0;
	}
}

gdispImagePrivate_PNG *
pnglib_file_new(gdispImage * img, int bufsize)
{
    img->priv = gdispImageAlloc(img, (sizeof(gdispImagePrivate_PNG) + bufsize));
    if (img->priv == NULL)
        return NULL;

    memset(img->priv, 0, sizeof(gdispImagePrivate_PNG) + bufsize);
	gdispImagePrivate_PNG *priv = (gdispImagePrivate_PNG*) img->priv;
    priv->bufsize = bufsize;

    return priv;
}

ssize_t
pnglib_file_read(gdispImage *img, uint8_t *buf, size_t buf_len)
{
	gdispImagePrivate_PNG *priv = (gdispImagePrivate_PNG*) img->priv;
    ssize_t res = 0;
    while (buf_len > 0)
    {
        if (priv->buflen == 0)
        {
            ssize_t r = gfileRead(img->f, priv->buf, priv->bufsize);
            if (r < 0)
                return r;
            if (r == 0)
                return res;
            priv->bufpos = 0;
            priv->buflen = r;
        }

        if (priv->buflen)
        {
            ssize_t maxcopy = priv->buflen > buf_len ? buf_len : priv->buflen;
            memcpy(buf, &priv->buf[priv->bufpos], maxcopy);
            buf = &buf[maxcopy];
            buf_len -= maxcopy;
            res += maxcopy;
            priv->bufpos += maxcopy;
            priv->buflen -= maxcopy;
        }
    }

    return res;
}

gdispImageError gdispImageOpen_PNG(gdispImage *img) {
	gdispImagePrivate_PNG *priv = pnglib_file_new(img, 1024);
	if(priv){
		img->priv = priv;
		img->type = GDISP_IMAGE_TYPE_PNG;

		priv->pr = lib_png_new((lib_reader_read_t) &pnglib_file_read, img);
		if(priv->pr == NULL){
			gdispImageClose_PNG(img);
			return GDISP_IMAGE_ERR_NOMEMORY;
		}

		int header = lib_png_read_header(priv->pr);
		if(header != 0){
			gdispImageClose_PNG(img);
			return GDISP_IMAGE_ERR_BADDATA;
		}
		img->width  = (coord_t) priv->pr->ihdr.width;
		img->height = (coord_t) priv->pr->ihdr.height;
		priv->pixbuf_size = img->width * img->height * sizeof(pixel_t);
		if(priv->pr->ihdr.color_type & 4){
			img->flags = GDISP_IMAGE_FLG_TRANSPARENT;
		}
		return GDISP_IMAGE_ERR_OK;
	}

	return GDISP_IMAGE_ERR_NOMEMORY;
}

gdispImageError gdispGImageDraw_PNG(GDisplay *g, gdispImage *img, coord_t x, coord_t y, coord_t cx, coord_t cy, coord_t sx, coord_t sy) {
    gdispImagePrivate_PNG * priv;

    priv = (gdispImagePrivate_PNG *)img->priv;

    /* Check some reasonableness */
    if (sx >= img->width || sy >= img->height) return GDISP_IMAGE_ERR_OK;
    if (sx + cx > img->width) cx = img->width - sx;
    if (sy + cy > img->height) cy = img->height - sy;

    /* Cache the image if not already cached */
    if (!priv->frame0cache) {
        gdispImageError err = gdispImageCache_PNG(img);
        if(err){
            return err;
        }
    }
    gdispGBlitArea(g, x, y, cx, cy, sx, sy, img->width, priv->frame0cache);

	return GDISP_IMAGE_ERR_OK;

}

gdispImageError gdispImageCache_PNG(gdispImage *img) {
	gdispImagePrivate_PNG *priv = (gdispImagePrivate_PNG*) img->priv;

    /* If we are already cached - just return OK */
	if (priv->frame0cache)
		return GDISP_IMAGE_ERR_OK;

	/* Otherwise start a new decode */
	priv->frame0cache = (pixel_t *) gdispImageAlloc(img, priv->pixbuf_size);
	if (!priv->frame0cache){
		return GDISP_IMAGE_ERR_NOMEMORY;
	}
	// fill buffer with white pixels - to avoid alpha channel transparency issues
	memset(priv->frame0cache, 0xff, priv->pixbuf_size);

	int res = lib_png_load_image(priv->pr, priv->frame0cache, 0, 0, img->width, img->height, img->width);

	if(res < 0){
		return GDISP_IMAGE_ERR_BADDATA;
	}

	return GDISP_IMAGE_ERR_OK;
}

delaytime_t gdispImageNext_PNG(gdispImage *img) {
	(void) img;

	/* No more frames/pages */
	return TIME_INFINITE;
}

#endif /* GFX_USE_GDISP && GDISP_NEED_IMAGE && GDISP_NEED_IMAGE_PNG */
