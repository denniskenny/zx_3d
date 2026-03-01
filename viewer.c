/*
 * viewer.c - Wireframe dodecahedron renderer for ZX Spectrum
 *
 * Draws to an off-screen back buffer at 0xC000, then blits to
 * screen RAM (0x4000) with LDIR right after HALT for clean frames.
 * All integer maths, no floating point.
 */

#include <string.h>
#include <stdint.h>
#include "assets/dodecahedron.h"

/* Back buffer lives at 0xC000 — same layout as screen RAM */
#define BACKBUF  ((uint8_t *)0xC000)
#define SCREEN   ((uint8_t *)0x4000)
#define PIX_SIZE 6144

/* ------------------------------------------------------------------ */
/* 256-entry sine table: sin(2*PI*i/256) * 127, range -127..127       */
/* ------------------------------------------------------------------ */
static const int8_t sin_tab[256] = {
      0,   3,   6,   9,  12,  16,  19,  22,
     25,  28,  31,  34,  37,  40,  43,  46,
     49,  51,  54,  57,  60,  63,  65,  68,
     71,  73,  76,  78,  81,  83,  85,  88,
     90,  92,  94,  96,  98, 100, 102, 104,
    106, 107, 109, 110, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124,
    125, 125, 126, 126, 126, 127, 127, 127,
    127, 127, 127, 127, 126, 126, 126, 125,
    125, 124, 123, 122, 122, 121, 120, 118,
    117, 116, 115, 113, 112, 110, 109, 107,
    106, 104, 102, 100,  98,  96,  94,  92,
     90,  88,  85,  83,  81,  78,  76,  73,
     71,  68,  65,  63,  60,  57,  54,  51,
     49,  46,  43,  40,  37,  34,  31,  28,
     25,  22,  19,  16,  12,   9,   6,   3,
      0,  -3,  -6,  -9, -12, -16, -19, -22,
    -25, -28, -31, -34, -37, -40, -43, -46,
    -49, -51, -54, -57, -60, -63, -65, -68,
    -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98,-100,-102,-104,
   -106,-107,-109,-110,-112,-113,-115,-116,
   -117,-118,-120,-121,-122,-122,-123,-124,
   -125,-125,-126,-126,-126,-127,-127,-127,
   -127,-127,-127,-127,-126,-126,-126,-125,
   -125,-124,-123,-122,-122,-121,-120,-118,
   -117,-116,-115,-113,-112,-110,-109,-107,
   -106,-104,-102,-100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73,
    -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28,
    -25, -22, -19, -16, -12,  -9,  -6,  -3
};

#define isin(a) (sin_tab[(uint8_t)(a)])
#define icos(a) (sin_tab[(uint8_t)((a) + 64)])

/* Bit mask LUT for fast pixel set */
static const uint8_t bit_mask[8] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/* Simple 8-bit LFSR PRNG */
static uint8_t rng_state = 1;
static uint8_t rng_next(void) {
    uint8_t s = rng_state;
    s ^= s << 3;
    s ^= s >> 5;
    s ^= s << 4;
    rng_state = s;
    return s;
}

/* Projected screen coordinates */
static int px[DODEC_NUM_VERTICES];
static int py[DODEC_NUM_VERTICES];

/* Face visibility flags (1 = front-facing) */
static uint8_t face_vis[DODEC_NUM_FACES];

/* ------------------------------------------------------------------ */
/* Plot pixel into back buffer at 0xC000 (same interleaved layout)    */
/* ------------------------------------------------------------------ */
static void plot_pixel(uint8_t x, uint8_t y) {
    uint8_t hi = 0xC0 | ((y & 0xC0) >> 3) | (y & 0x07);
    uint8_t lo = ((y & 0x38) << 2) | (x >> 3);
    uint8_t *addr = (uint8_t *)(((uint16_t)hi << 8) | lo);
    *addr |= bit_mask[x & 7];
}

/* ------------------------------------------------------------------ */
/* Bresenham line draw into back buffer                               */
/* ------------------------------------------------------------------ */
static void draw_line(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx, sy, err, e2;

    if (dx < 0) { dx = -dx; sx = -1; } else { sx = 1; }
    if (dy < 0) { dy = -dy; sy = -1; } else { sy = 1; }
    err = dx - dy;

    for (;;) {
        plot_pixel((uint8_t)x0, (uint8_t)y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = err << 1;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/* ------------------------------------------------------------------ */
/* LDIR blit: back buffer → screen RAM  (6144 bytes)                  */
/* ------------------------------------------------------------------ */
static void blit(void) {
    __asm
        ld  hl, #0xC000
        ld  de, #0x4000
        ld  bc, #6144
        ldir
    __endasm;
}

/* ------------------------------------------------------------------ */
/* Rotate & project vertices into px[]/py[]                           */
/* ------------------------------------------------------------------ */
static void transform(uint8_t ay, uint8_t ax, uint8_t az) {
    int8_t sa = isin(ay);
    int8_t ca = icos(ay);
    int8_t sb = isin(ax);
    int8_t cb = icos(ax);
    int8_t sc = isin(az);
    int8_t cc = icos(az);
    uint8_t i;

    for (i = 0; i < DODEC_NUM_VERTICES; i++) {
        int x = dodec_vertices[i][0];
        int y = dodec_vertices[i][1];
        int z = dodec_vertices[i][2];

        /* Z-axis rotation (XY plane) */
        int tx = (x * cc >> 7) - (y * sc >> 7);
        int ty = (x * sc >> 7) + (y * cc >> 7);

        /* Y-axis rotation (XZ plane) */
        int rx = (tx * ca >> 7) - (z * sa >> 7);
        int rz = (tx * sa >> 7) + (z * ca >> 7);

        /* X-axis rotation (YZ plane) */
        int ry = (ty * cb >> 7) - (rz * sb >> 7);
        int rz2 = (ty * sb >> 7) + (rz * cb >> 7);

        int d = rz2 + 400;
        px[i] = 128 + (rx * 150 / d);
        py[i] =  96 - (ry * 150 / d);
    }
}

/* ------------------------------------------------------------------ */
/* Read keyboard half-row via IN.  Returns bits active-low.           */
/* ------------------------------------------------------------------ */
static uint8_t key_read(uint8_t high_byte) {
    uint8_t val;
    __asm
        ld   b, 4 (ix)       ; high_byte
        ld   c, #0xFE
        in   a, (c)
        ld   l, a
    __endasm;
    (void)high_byte;
    return val;
}

/* ------------------------------------------------------------------ */
/* Read Kempston joystick port 0x1F.  Bits active-high.               */
/* ------------------------------------------------------------------ */
static uint8_t joy_read(void) {
    uint8_t val;
    __asm
        ld   bc, #0x001F
        in   a, (c)
        ld   l, a
    __endasm;
    return val;
}

/* ------------------------------------------------------------------ */
/* Print string into back buffer using ROM charset at 0x3D00          */
/* ------------------------------------------------------------------ */
static void print_at(uint8_t row, uint8_t col, const char *str) {
    const uint8_t *charset = (const uint8_t *)0x3D00;
    while (*str) {
        const uint8_t *glyph = charset + (((uint8_t)*str - 32) << 3);
        uint16_t base = 0xC000 + ((uint16_t)(row >> 3) << 11)
                      + ((uint16_t)(row & 7) << 5) + col;
        uint8_t line;
        for (line = 0; line < 8; line++)
            *(uint8_t *)(base + ((uint16_t)line << 8)) = glyph[line];
        col++;
        str++;
    }
}

/* ------------------------------------------------------------------ */
/* Backface culling: test each face's 2D winding after projection.    */
/* CCW in 3D becomes CW on screen (y-down), so cross < 0 = visible.  */
/* ------------------------------------------------------------------ */
static void cull_faces(void) {
    uint8_t f;
    for (f = 0; f < DODEC_NUM_FACES; f++) {
        int v0 = dodec_faces[f][0];
        int v1 = dodec_faces[f][1];
        int v2 = dodec_faces[f][2];
        int cross = (px[v1] - px[v0]) * (py[v2] - py[v0])
                  - (py[v1] - py[v0]) * (px[v2] - px[v0]);
        face_vis[f] = (cross < 0) ? 1 : 0;
    }
}

/* ------------------------------------------------------------------ */
/* Draw visible edges into back buffer                                */
/* Skip edges where both adjacent faces are back-facing.              */
/* ------------------------------------------------------------------ */
static void draw_edges(void) {
    uint8_t i;
    for (i = 0; i < DODEC_NUM_EDGES; i++) {
        if (!face_vis[dodec_edge_faces[i][0]] &&
            !face_vis[dodec_edge_faces[i][1]])
            continue;

        int x1 = px[dodec_edges[i][0]];
        int y1 = py[dodec_edges[i][0]];
        int x2 = px[dodec_edges[i][1]];
        int y2 = py[dodec_edges[i][1]];

        if ((unsigned)x1 < 256 && (unsigned)y1 < 192 &&
            (unsigned)x2 < 256 && (unsigned)y2 < 192) {
            draw_line(x1, y1, x2, y2);
        }
    }
}

int main(void) {
    uint8_t ay = 0;
    uint8_t ax = 0;
    uint8_t az = 0;
    int8_t  dx = 2;   /* X-axis rotation step */
    int8_t  dy = 1;   /* Y-axis rotation step */
    int8_t  dz = 1;   /* Z-axis rotation step */
    uint8_t prev_input = 0;
    uint8_t frame_count = 0;
    uint8_t ink = 7;

    /* Set border to black */
    *(uint8_t *)0x5C48 = 0;
    __asm
        xor a
        out (0xFE), a
    __endasm;

    /* Set attributes: white ink on black paper */
    memset((void *)0x5800, 7, 768);
    /* Clear screen and back buffer */
    memset(SCREEN,  0, PIX_SIZE);
    memset(BACKBUF, 0, PIX_SIZE);

    while (1) {
        /* Draw frame into back buffer (invisible) */
        transform(ay, ax, az);
        cull_faces();
        draw_edges();
        print_at(21, 4, "Orbital Model Calculations");

        /* Wait for vblank.  --reserve-regs-iy keeps IY free
         * for the ROM IM 1 ISR, so no save/restore needed. */
        __asm
            ei
            halt
            di
        __endasm;
        blit();

        /* Clear back buffer for next frame */
        memset(BACKBUF, 0, PIX_SIZE);

        /* Change ink colour every 10 frames */
        frame_count++;
        if (frame_count >= 10) {
            frame_count = 0;
            ink = (rng_next() & 7);
            if (ink == 0) ink = 7;
            memset((void *)0x5800, ink, 768);
        }

        /* Read input: keyboard + Kempston joystick */
        {
            uint8_t input = 0;
            uint8_t kb;
            uint8_t joy;

            /* Q key: port 0xFBFE bit 0 (active low) */
            kb = key_read(0xFB);
            if (!(kb & 0x01)) input |= 0x01;

            /* A key: port 0xFDFE bit 0 (active low) */
            kb = key_read(0xFD);
            if (!(kb & 0x01)) input |= 0x02;

            /* O key: port 0xDFFE bit 1 (active low) */
            kb = key_read(0xDF);
            if (!(kb & 0x02)) input |= 0x04;

            /* Kempston: fire1=bit4, fire2=bit5, up=bit3 (active high) */
            joy = joy_read();
            if (joy & 0x10) input |= 0x01;  /* fire1 → X */
            if (joy & 0x20) input |= 0x02;  /* fire2 → Y */
            if (joy & 0x08) input |= 0x04;  /* up    → Z */

            /* Toggle on rising edge (debounce) */
            uint8_t pressed = input & ~prev_input;
            if (pressed & 0x01) dx = -dx;
            if (pressed & 0x02) dy = -dy;
            if (pressed & 0x04) dz = -dz;
            prev_input = input;
        }

        ax += dx;
        ay += dy;
        az += dz;
    }

    return 0;
}
