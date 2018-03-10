#include <QImage>
#include "../../../gfx.h"
#include "../../../src/gdisp/gdisp_driver.h"
#include "gdisp_lld_qimage.h"

bool_t qimage_init(GDisplay* g, coord_t width, coord_t height)
{
    QImage* qimage = new QImage(width, height, QImage::Format_RGB888);
    if (!qimage) {
        return FALSE;
    }
    qimage->fill(Qt::gray);

    g->priv = qimage;

    return TRUE;
}

void qimage_setPixel(GDisplay* g)
{
    QImage* qimage = static_cast<QImage*>(g->priv);
    if (!qimage) {
        return;
    }

    QRgb rgbVal = qRgb(RED_OF(g->p.color), GREEN_OF(g->p.color), BLUE_OF(g->p.color));
    qimage->setPixel(g->p.x, g->p.y, rgbVal);
}

color_t qimage_getPixel(GDisplay* g)
{
    const QImage* qimage = static_cast<const QImage*>(g->priv);
    if (!qimage) {
        return 0;
    }

    return static_cast<color_t>(qimage->pixel(g->p.x, g->p.y));
}
