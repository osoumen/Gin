/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

template <typename T>
inline uint8 toByte (T v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return uint8 (v);
}

inline uint8 getIntensity (uint8 r, uint8 g, uint8 b)
{
    return (uint8)((7471 * b + 38470 * g + 19595 * r) >> 16);
}

inline uint8 computeAlpha (uint8 la, uint8 ra)
{
    return (uint8)(((la * (256 - (ra + (ra >> 7)))) >> 8) + ra);
}

inline PixelARGB blend (const PixelARGB& c1, const PixelARGB& c2)
{
    int a = c1.getAlpha();
    int invA = 255 - a;

    int r = ((c2.getRed()   * invA) + (c1.getRed()   * a)) / 256;
    int g = ((c2.getGreen() * invA) + (c1.getGreen() * a)) / 256;
    int b = ((c2.getBlue()  * invA) + (c1.getBlue()  * a)) / 256;
    uint8 a2 = computeAlpha (c2.getAlpha(), c1.getAlpha());

    return PixelARGB (a2, toByte (r), toByte (g), toByte (b));
}

//==============================================================================
Image applyVignette (Image src, float amountIn, float radiusIn, float fallOff)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    jassert (src.getFormat() == Image::ARGB);
    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    double outA = w * 0.5 * radiusIn;
    double outB = h * 0.5 * radiusIn;

    double inA = outA * fallOff;
    double inB = outB * fallOff;

    double cx = w * 0.5;
    double cy = h * 0.5;

    double amount = 1.0 - amountIn;

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    Ellipse<double> outE { outA, outB };
    Ellipse<double> inE  { inA,  inB  };

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        double dy = y - cy;

        for (int x = 0; x < w; x++)
        {
            double dx = x - cx;

            bool outside = outE.isPointOutside ({dx, dy});
            bool inside  = inE.isPointInside ({dx, dy});

            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            if (inside)
            {
                d->set (*s);
            }
            else if (outside)
            {
                uint8 r = toByte (0.5 + (s->getRed() * amount));
                uint8 g = toByte (0.5 + (s->getGreen() * amount));
                uint8 b = toByte (0.5 + (s->getBlue() * amount));
                uint8 a = s->getAlpha();

                d->setARGB (a, r, g, b);
            }
            else
            {
                double angle = std::atan2 (dy, dx);

                auto p1 = outE.pointAtAngle (angle);
                auto p2 = inE.pointAtAngle (angle);

                auto l1 = Line<double> ({dx,dy}, p2);
                auto l2 = Line<double> (p1, p2);

                double factor = 1.0 - (amountIn * jlimit (0.0, 1.0, l1.getLength() / l2.getLength()));

                uint8 r = toByte (0.5 + (s->getRed()   * factor));
                uint8 g = toByte (0.5 + (s->getGreen() * factor));
                uint8 b = toByte (0.5 + (s->getBlue()  * factor));
                uint8 a = s->getAlpha();

                d->setARGB (a, r, g, b);
            }

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    
    return dst;
}

Image applySepia (Image src)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    jassert (src.getFormat() == Image::ARGB);
    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            uint8 ro = toByte ((r * .393) + (g *.769) + (b * .189));
            uint8 go = toByte ((r * .349) + (g *.686) + (b * .168));
            uint8 bo = toByte ((r * .272) + (g *.534) + (b * .131));

            d->setARGB (a, ro, go, bo);

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    return dst;
}

Image applyGreyScale (Image src)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            uint8 ro = toByte (r * 0.30 + 0.5);
            uint8 go = toByte (g * 0.59 + 0.5);
            uint8 bo = toByte (b * 0.11 + 0.5);

            d->setARGB (a,
                        toByte (ro + go + bo),
                        toByte (ro + go + bo),
                        toByte (ro + go + bo));

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    return dst;
}

Image applySoften (Image src)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        for (int x = 0; x < w; x++)
        {
            int ro = 0, go = 0, bo = 0;
            uint8 a = 0;

            for (int m = -1; m <= 1; m++)
            {
                for (int n = -1; n <= 1; n++)
                {
                    int cx = jlimit (0, w - 1, x + m);
                    int cy = jlimit (0, h - 1, y + n);

                    PixelARGB* s = (PixelARGB*) srcData.getPixelPointer (cx, cy);

                    ro += s->getRed();
                    go += s->getGreen();
                    bo += s->getBlue();
                }
            }

            PixelARGB* s = (PixelARGB*) srcData.getPixelPointer (x, y);
            a = s->getAlpha();

            PixelARGB* d = (PixelARGB*) dstData.getPixelPointer (x, y);

            d->setARGB (a, toByte (ro / 9), toByte (go / 9), toByte (bo / 9));
        }
    });
    return dst;
}

Image applySharpen (Image src)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        for (int x = 0; x < w; x++)
        {
            auto getPixelPointer = [&] (int cx, int cy) -> PixelARGB*
            {
                cx = jlimit (0, w - 1, cx);
                cy = jlimit (0, h - 1, cy);

                return (PixelARGB*) srcData.getPixelPointer (cx, cy);
            };

            int ro = 0, go = 0, bo = 0;
            uint8 ao = 0;

            PixelARGB* s = getPixelPointer (x, y);

            ro = s->getRed()   * 5;
            go = s->getGreen() * 5;
            bo = s->getBlue()  * 5;
            ao = s->getAlpha();

            s = getPixelPointer (x, y - 1);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            s = getPixelPointer (x - 1, y);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            s = getPixelPointer (x + 1, y);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            s = getPixelPointer (x, y + 1);
            ro -= s->getRed();
            go -= s->getGreen();
            bo -= s->getBlue();

            PixelARGB* d = (PixelARGB*) dstData.getPixelPointer (x, y);

            d->setARGB (ao, toByte (ro), toByte (go), toByte (bo));
        }
    });
    return dst;
}

Image applyGamma (Image src, float gamma)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            uint8 ro = toByte (std::pow (r / 255.0, gamma) * 255.0 + 0.5);
            uint8 go = toByte (std::pow (g / 255.0, gamma) * 255.0 + 0.5);
            uint8 bo = toByte (std::pow (b / 255.0, gamma) * 255.0 + 0.5);

            d->setARGB (a, ro, go, bo);

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    return dst;
}

Image applyInvert (Image src)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            uint8 ro = 255 - r;
            uint8 go = 255 - g;
            uint8 bo = 255 - b;

            d->setARGB (a, ro, go, bo);

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    return dst;
}

Image applyContrast (Image src, float contrast)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    contrast = (100.0f - contrast) / 100.0f;
    contrast = square (contrast);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            double ro = (double) r / 255.0;
            ro = ro - 0.5;
            ro = ro * contrast;
            ro = ro + 0.5;
            ro = ro * 255.0;

            double go = (double) g / 255.0;
            go = go - 0.5;
            go = go * contrast;
            go = go + 0.5;
            go = go * 255.0;

            double bo = (double) b / 255.0;
            bo = bo - 0.5;
            bo = bo * contrast;
            bo = bo + 0.5;
            bo = bo * 255.0;

            ro = toByte (ro);
            go = toByte (go);
            bo = toByte (bo);

            d->setARGB (a, toByte (ro), toByte (go), toByte (bo));

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    return dst;
}

Image applyBrightnessContrast (Image src, float brightness, float contrast)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    double multiply = 1;
    double divide = 1;

    if (contrast < 0)
    {
        multiply = contrast + 100;
        divide = 100;
    }
    else if (contrast > 0)
    {
        multiply = 100;
        divide = 100 - contrast;
    }
    else
    {
        multiply = 1;
        divide = 1;
    }

    uint8* rgbTable = new uint8[65536];

    if (divide == 0)
    {
        for (int intensity = 0; intensity < 256; intensity++)
        {
            if (intensity + brightness < 128)
                rgbTable[intensity] = 0;
            else
                rgbTable[intensity] = 255;
        }
    }
    else if (divide == 100)
    {
        for (int intensity = 0; intensity < 256; intensity++)
        {
            int shift = int ((intensity - 127) * multiply / divide + 127 - intensity + brightness);

            for (int col = 0; col < 256; col++)
            {
                int index = (intensity * 256) + col;
                rgbTable[index] = toByte (col + shift);
            }
        }
    }
    else
    {
        for (int intensity = 0; intensity < 256; intensity++)
        {
            int shift = int ((intensity - 127 + brightness) * multiply / divide + 127 - intensity);

            for (int col = 0; col < 256; col++)
            {
                int index = (intensity * 256) + col;
                rgbTable[index] = toByte (col + shift);
            }
        }
    }

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            if (divide == 0)
            {
                int i = getIntensity (toByte (r), toByte (g), toByte (b));
                uint8 c = rgbTable[i];

                d->setARGB (a, c, c, c);
            }
            else
            {
                int i = getIntensity (toByte (r), toByte (g), toByte (b));
                int shiftIndex = i * 256;

                uint8 ro = rgbTable[shiftIndex + r];
                uint8 go = rgbTable[shiftIndex + g];
                uint8 bo = rgbTable[shiftIndex + b];

                ro = toByte (ro);
                go = toByte (go);
                bo = toByte (bo);

                d->setARGB (a, ro, go, bo);
            }

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });

    delete[] rgbTable;
    return dst;
}

Image applyHueSaturationLightness (Image src, float hueIn, float saturation, float lightness)
{
    const int w = src.getWidth();
    const int h = src.getHeight();

    if (src.getFormat() != Image::ARGB)
        return Image();

    if (saturation > 100)
        saturation = ((saturation - 100) * 3) + 100;
    saturation = (saturation * 1024) / 100;

    hueIn /= 360.0f;

    Image dst (Image::ARGB, w, h, true);

    Image::BitmapData srcData (src, Image::BitmapData::readOnly);
    Image::BitmapData dstData (dst, Image::BitmapData::writeOnly);

    multiThreadedFor<int> (0, h, 1, [&] (int y)
    {
        uint8* ps = srcData.getLinePointer (y);
        uint8* ds = dstData.getLinePointer (y);

        for (int x = 0; x < w; x++)
        {
            PixelARGB* s = (PixelARGB*)ps;
            PixelARGB* d = (PixelARGB*)ds;

            uint8 r = s->getRed();
            uint8 g = s->getGreen();
            uint8 b = s->getBlue();
            uint8 a = s->getAlpha();

            int intensity = getIntensity (toByte (r), toByte (g), toByte (b));
            int ro = toByte (int (intensity * 1024 + (r - intensity) * saturation) >> 10);
            int go = toByte (int (intensity * 1024 + (g - intensity) * saturation) >> 10);
            int bo = toByte (int (intensity * 1024 + (b - intensity) * saturation) >> 10);

            Colour c (toByte (ro), toByte (go), toByte (bo));
            float hue = c.getHue();
            hue += hueIn;

            while (hue < 0.0f)  hue += 1.0f;
            while (hue >= 1.0f) hue -= 1.0f;

            c = Colour::fromHSV (hue, c.getSaturation(), c.getBrightness(), float (a));
            ro = c.getRed();
            go = c.getGreen();
            bo = c.getBlue();

            ro = toByte (ro);
            go = toByte (go);
            bo = toByte (bo);

            d->setARGB (a, toByte (ro), toByte (go), toByte (bo));

            if (lightness > 0)
                *d = blend (PixelARGB (toByte ((lightness * 255) / 100), 255, 255, 255), *d);
            else if (lightness < 0)
                *d = blend (PixelARGB (toByte ((-lightness * 255) / 100), 0, 0, 0), *d);

            ps += srcData.pixelStride;
            ds += srcData.pixelStride;
        }
    });
    return dst;
}
