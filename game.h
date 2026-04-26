#ifndef GAME_H // if not defined then define game.h
#define GAME_H

#include <raylib.h>  // raylib and boolian
#include <stdbool.h>

// screen and bullets
#define screen_width  1280
#define screen_height 720
#define max_bullets   500

// for map. 42 x 42, 30 pixels each about 1260 by 1260
#define MAP_TILE_SIZE  30
#define MAP_COLS       42
#define MAP_ROWS       42
#define MAP_PIXEL_SIZE 1260

// just manually hardcode the walkable area for the maps
#define MAP1_BOUND_MIN  75.0f
#define MAP1_BOUND_MAX 1185.0f

#define MAP2_BOUND_MIN  45.0f
#define MAP2_BOUND_MAX 1215.0f


#define MAX_ENEMIES       20
#define MAX_ENEMY_BULLETS 200

// for death sprite
#define DEATH_LINGER_TIME 2.0f


typedef enum { // defines the two maps // https://youtu.be/CI9dRTvzgqE?t=130
    STATE_MAP1 = 0,
    STATE_MAP2 = 1
} GameState; // done so we can say GameState.STATE_MAP1 instead of just 0 which is more readable

// Round Phases
typedef enum {
    PHASE_FIGHTING,      // actively fighting enemies
    PHASE_INTERMISSION,  // ROUND CLEAR small pause between rounds
    PHASE_BOSS,          // Boss fight
    PHASE_GAMEOVER,      // game over
    PHASE_WIN            // game win
} RoundPhase;

// ─── Structs ──────────────────────────────────────────────────────────────────

struct weapon {
    char name[20];
    int damage;
    float bulletspeed;
    float firerate;
    float lastfiretime;
    bool isSniper;
    bool isShotgun;
    Texture2D weapon_texture;
    Texture2D crosshair_texture;
};

struct player {
    float x;
    float y;
    float speed;
    int health;
    struct weapon currentweapon;
    Texture2D walkingframes[3];
    int currentframe;
    float frameTimer;
    Texture2D deadsprite;
};

struct bullet {
    float x;
    float y;
    float prevX; // for sniper effects
    float prevY;
    float dx;    // velocity of bullet
    float dy;
    bool active;
    bool hastrail;
};

struct enemy {
    char name[20];
    float x;
    float y;
    float speed;
    int health;
    int damage;
    Texture2D frames[3];
    bool active;
    Texture2D deadsprite; // for dead sprite
    float deathTimer;
    bool isDead; 
};

#endif // to finish header file
