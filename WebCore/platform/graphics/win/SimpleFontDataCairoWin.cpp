/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SimpleFontData.h"

#include <windows.h>

#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "MathExtras.h"
#include "NotImplemented.h"
#include <cairo.h>
#include <cairo-win32.h>
#include <mlang.h>
#include <tchar.h>

namespace WebCore {

void SimpleFontData::platformInit()
{
    m_scriptCache = 0;
    m_scriptFontProperties = 0;
    m_isSystemFont = false;

    if (m_font.useGDI())
       return initGDIFont();

    HDC hdc = GetDC(0);
    SaveDC(hdc);

    cairo_scaled_font_t* scaledFont = m_font.scaledFont();
    const double metricsMultiplier = cairo_win32_scaled_font_get_metrics_factor(scaledFont) * m_font.size();

    cairo_win32_scaled_font_select_font(scaledFont, hdc);

    TEXTMETRIC textMetrics;
    GetTextMetrics(hdc, &textMetrics);
    m_ascent = lroundf(textMetrics.tmAscent * metricsMultiplier);
    m_descent = lroundf(textMetrics.tmDescent * metricsMultiplier);
    m_xHeight = m_ascent * 0.56f; // Best guess for xHeight for non-Truetype fonts.
    m_lineGap = lroundf(textMetrics.tmExternalLeading * metricsMultiplier);
    m_lineSpacing = m_ascent + m_descent + m_lineGap;

    OUTLINETEXTMETRIC metrics;
    if (GetOutlineTextMetrics(hdc, sizeof(metrics), &metrics) > 0) {
        // This is a TrueType font.  We might be able to get an accurate xHeight
        GLYPHMETRICS gm;
        MAT2 mat = { 1, 0, 0, 1 };
        DWORD len = GetGlyphOutline(hdc, 'x', GGO_METRICS, &gm, 0, 0, &mat);
        if (len != GDI_ERROR && gm.gmptGlyphOrigin.y > 0)
            m_xHeight = gm.gmptGlyphOrigin.y * metricsMultiplier;
    }

    cairo_win32_scaled_font_done_font(scaledFont);

    m_isSystemFont = false;
    m_scriptCache = 0;
    m_scriptFontProperties = 0;

    RestoreDC(hdc, -1);
    ReleaseDC(0, hdc);
}

void SimpleFontData::platformDestroy()
{
    cairo_font_face_destroy(m_font.fontFace());
    cairo_scaled_font_destroy(m_font.scaledFont());

    DeleteObject(m_font.hfont());

    platformCommonDestroy();
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (m_font.useGDI())
       return widthForGDIGlyph(glyph);

    HDC hdc = GetDC(0);
    SaveDC(hdc);

    cairo_scaled_font_t* scaledFont = m_font.scaledFont();
    cairo_win32_scaled_font_select_font(scaledFont, hdc);

    int width;
    GetCharWidthI(hdc, glyph, 1, 0, &width);

    cairo_win32_scaled_font_done_font(scaledFont);

    RestoreDC(hdc, -1);
    ReleaseDC(0, hdc);

    const double metricsMultiplier = cairo_win32_scaled_font_get_metrics_factor(scaledFont) * m_font.size();
    return width * metricsMultiplier;
}

void SimpleFontData::setFont(cairo_t* cr) const
{
    ASSERT(cr);
    m_font.setFont(cr);
}

}
