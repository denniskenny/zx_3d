/*
 * dodecahedron.h - Dodecahedron mesh data for ZX Spectrum
 *
 * 20 vertices, 30 edges, 12 pentagonal faces.
 *
 * Coordinates are 16-bit signed integers (Z80 int).
 * Scale factor: 100 units = 1.0
 * Derived from golden ratio phi = 1.618, 1/phi = 0.618
 * Vertex values: +/-100 (1.0), +/-62 (1/phi), +/-162 (phi)
 * Max coordinate magnitude: 162
 *
 * Fixed-point rotation tips for Z80:
 *   - Store sin/cos as int8_t scaled to 127 = 1.0
 *   - coord * sin_val fits in int16_t (max 162*127 = 20574)
 *   - Shift right by 7 after multiply to get result in same scale
 *
 * Face winding: counter-clockwise when viewed from outside.
 * For backface culling, compute 2D cross product of first two
 * projected edges; positive = front-facing (CCW convention).
 */

#ifndef DODECAHEDRON_H
#define DODECAHEDRON_H

#define DODEC_NUM_VERTICES  20
#define DODEC_NUM_EDGES     30
#define DODEC_NUM_FACES     12
#define DODEC_VERTS_PER_FACE 5
#define DODEC_SCALE         100

/* --- Vertices: [index] = { x, y, z } --- */

static const int dodec_vertices[DODEC_NUM_VERTICES][3] = {
    /*  0 */ { 100,  100,  100},
    /*  1 */ { 100,  100, -100},
    /*  2 */ { 100, -100,  100},
    /*  3 */ { 100, -100, -100},
    /*  4 */ {-100,  100,  100},
    /*  5 */ {-100,  100, -100},
    /*  6 */ {-100, -100,  100},
    /*  7 */ {-100, -100, -100},
    /*  8 */ {   0,   62,  162},
    /*  9 */ {   0,   62, -162},
    /* 10 */ {   0,  -62,  162},
    /* 11 */ {   0,  -62, -162},
    /* 12 */ {  62,  162,    0},
    /* 13 */ {  62, -162,    0},
    /* 14 */ { -62,  162,    0},
    /* 15 */ { -62, -162,    0},
    /* 16 */ { 162,    0,   62},
    /* 17 */ { 162,    0,  -62},
    /* 18 */ {-162,    0,   62},
    /* 19 */ {-162,    0,  -62},
};

/* --- Edges: [index] = { vertex_a, vertex_b } --- */

static const unsigned char dodec_edges[DODEC_NUM_EDGES][2] = {
    { 0,  8}, { 0, 12}, { 0, 16},
    { 1,  9}, { 1, 12}, { 1, 17},
    { 2, 10}, { 2, 13}, { 2, 16},
    { 3, 11}, { 3, 13}, { 3, 17},
    { 4,  8}, { 4, 14}, { 4, 18},
    { 5,  9}, { 5, 14}, { 5, 19},
    { 6, 10}, { 6, 15}, { 6, 18},
    { 7, 11}, { 7, 15}, { 7, 19},
    { 8, 10}, { 9, 11}, {12, 14},
    {13, 15}, {16, 17}, {18, 19},
};

/* --- Faces: 12 pentagons, CCW winding from outside --- */

static const unsigned char dodec_faces[DODEC_NUM_FACES][DODEC_VERTS_PER_FACE] = {
    /*  0 */ {12, 14,  4,  8,  0},
    /*  1 */ { 8, 10,  2, 16,  0},
    /*  2 */ {16, 17,  1, 12,  0},
    /*  3 */ { 9,  5, 14, 12,  1},
    /*  4 */ {17,  3, 11,  9,  1},
    /*  5 */ {13,  3, 17, 16,  2},
    /*  6 */ {10,  6, 15, 13,  2},
    /*  7 */ {18,  6, 10,  8,  4},
    /*  8 */ {19, 18,  4, 14,  5},
    /*  9 */ { 9, 11,  7, 19,  5},
    /* 10 */ {15,  7, 19, 18,  6},
    /* 11 */ {13, 15,  7, 11,  3},
};

/* --- Edge-to-face adjacency: each edge borders exactly 2 faces --- */

static const unsigned char dodec_edge_faces[DODEC_NUM_EDGES][2] = {
    /*  0 { 0, 8} */ { 0,  1},
    /*  1 { 0,12} */ { 0,  2},
    /*  2 { 0,16} */ { 1,  2},
    /*  3 { 1, 9} */ { 3,  4},
    /*  4 { 1,12} */ { 2,  3},
    /*  5 { 1,17} */ { 2,  4},
    /*  6 { 2,10} */ { 1,  6},
    /*  7 { 2,13} */ { 5,  6},
    /*  8 { 2,16} */ { 1,  5},
    /*  9 { 3,11} */ { 4, 11},
    /* 10 { 3,13} */ { 5, 11},
    /* 11 { 3,17} */ { 4,  5},
    /* 12 { 4, 8} */ { 0,  7},
    /* 13 { 4,14} */ { 0,  8},
    /* 14 { 4,18} */ { 7,  8},
    /* 15 { 5, 9} */ { 3,  9},
    /* 16 { 5,14} */ { 3,  8},
    /* 17 { 5,19} */ { 8,  9},
    /* 18 { 6,10} */ { 6,  7},
    /* 19 { 6,15} */ { 6, 10},
    /* 20 { 6,18} */ { 7, 10},
    /* 21 { 7,11} */ { 9, 11},
    /* 22 { 7,15} */ {10, 11},
    /* 23 { 7,19} */ { 9, 10},
    /* 24 { 8,10} */ { 1,  7},
    /* 25 { 9,11} */ { 4,  9},
    /* 26 {12,14} */ { 0,  3},
    /* 27 {13,15} */ { 6, 11},
    /* 28 {16,17} */ { 2,  5},
    /* 29 {18,19} */ { 8, 10},
};

#endif /* DODECAHEDRON_H */
