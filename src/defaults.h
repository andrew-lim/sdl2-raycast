#ifndef DEFAULTS_H
#define DEFAULTS_H

// The longer each raycast strip, the less rays will be used.
// Increase this to boost FPS but reduce rendering quality.
// Currently supports values from 1 to 4.
const int DEFAULT_STRIP_WIDTH = 2;

const int DEFAULT_DISPLAY_WIDTH = 800;
const int DEFAULT_DISPLAY_HEIGHT = 600;
const int DEFAULT_FOV_DEGREES = 90;

const int MAP_WIDTH = 32;
const int MAP_HEIGHT = 24;

// Ground
extern int g_map[MAP_HEIGHT][MAP_WIDTH];

// Above ground walls
extern int g_map2[MAP_HEIGHT][MAP_WIDTH];
extern int g_map3[MAP_HEIGHT][MAP_WIDTH];

// Floor
extern int g_floormap[MAP_HEIGHT][MAP_WIDTH];

// Highest Ceiling
extern int g_ceilingmap[MAP_HEIGHT][MAP_WIDTH];

// Sprites
extern int g_spritemap[MAP_HEIGHT][MAP_WIDTH];

#endif
