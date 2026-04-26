// gcc game.c -o game -lraylib -lm -lpthread -ldl -lrt -lX11
#include "game.h"
#include <math.h>
#include <string.h>

// we are using these two functions because they need to be called multiple times and i am too lazy for that

// spawns an enemy from a template at a random position along the edge of the map
void spawnEnemy(struct enemy* pool, int index,
    const struct enemy* template, float boundry_minimum, float boundry_maximum) // const just does not change it
{
    pool[index] = *template;
    pool[index].active    = true;
    pool[index].isDead    = false;
    pool[index].deathTimer = 0.0f;

    int edge = GetRandomValue(0, 3);
    float pos = GetRandomValue(boundry_minimum, boundry_maximum);

/*      TOP (edge 0)
    ┌─────────────────┐
    │                 │
LEFT│                 │RIGHT
(2) │                 │(3)
    │                 │
    └─────────────────┘
        BOTTOM (edge 1)*/

    if      (edge == 0) { pool[index].x = pos;              pool[index].y = boundry_minimum; } // top
    else if (edge == 1) { pool[index].x = pos;              pool[index].y = boundry_maximum; } // bottom
    else if (edge == 2) { pool[index].x = boundry_minimum;  pool[index].y = pos;             } // left
    else                { pool[index].x = boundry_maximum;  pool[index].y = pos;             } // right
}



void fireEnemyBullet(struct bullet* pool, int poolSize, 
                            float fromX, float fromY, 
                            float angle, float speed)
{
    for (int i = 0; i < poolSize; i++)
    {
        if (pool[i].active) continue; // find an inactive bullet slot

        pool[i].active = true;
        pool[i].x      = fromX;
        pool[i].y      = fromY;
        pool[i].prevX  = fromX;
        pool[i].prevY  = fromY;
        pool[i].dx     = cosf(angle) * speed;
        pool[i].dy     = sinf(angle) * speed;
        pool[i].hastrail = false;
        break;
    }
}

// Main function
int main()
{
    // 1. Initialization
    InitWindow(screen_width, screen_height, "PF project demo");
    HideCursor();
    SetTargetFPS(60);


    InitAudioDevice();
    // Weapon sounds
    Sound snd_rifle   = LoadSound("assets/In use assets/Sound effects/weapon_rifle.mp3");
    Sound snd_smg     = LoadSound("assets/In use assets/Sound effects/weapon_smg.mp3");
    Sound snd_sniper  = LoadSound("assets/In use assets/Sound effects/weapon_sniper.mp3");
    Sound snd_shotgun = LoadSound("assets/In use assets/Sound effects/weapon_shotgun.mp3");

    // Other sounds
    Sound snd_damage  = LoadSound("assets/In use assets/Sound effects/Taking damage.mp3");
    Sound snd_gameover = LoadSound("assets/In use assets/Sound effects/Game over.mp3");

    // Music streams
    Music mus_phase1 = LoadMusicStream("assets/In use assets/Sound effects/Phase1 Track.mp3");
    Music mus_boss   = LoadMusicStream("assets/In use assets/Sound effects/Boss Track.mp3");
    Music mus_run    = LoadMusicStream("assets/In use assets/Sound effects/Running.mp3");

    PlayMusicStream(mus_phase1); // start phase 1 music immediately

    struct player player1;  
    player1.x = 400;
    player1.y = 225;
    player1.speed = 200;
    player1.health = 100;

    // camera setup
    Camera2D camera = { 0 };
    camera.target   = (Vector2){ player1.x, player1.y }; // camera follows me
    camera.offset   = (Vector2){ screen_width / 2.0f, screen_height / 2.0f }; // center of the screen
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    // Loading player animation frames
    player1.walkingframes[0] = LoadTexture("assets/In use assets/P1 animation/tile_0012.png");
    player1.walkingframes[1] = LoadTexture("assets/In use assets/P1 animation/tile_0013.png");
    player1.walkingframes[2] = LoadTexture("assets/In use assets/P1 animation/tile_0014.png");
    player1.deadsprite       = LoadTexture("assets/In use assets/P1 animation/tile_0015.png");
    player1.currentframe     = 0;
    player1.frameTimer       = 0.0f; // for animation

    // defines weapons       name,   damage, bspeed,  rate, time,isSniper, isShotgun,;
    struct weapon pistol  = { "Pistol",  10,  500.0f, 0.20f, 0.0f, false, false };
    struct weapon smg     = { "SMG",      5,  650.0f, 0.10f, 0.0f, false, false };
    struct weapon sniper  = { "Sniper",  60, 1500.0f, 0.60f, 0.0f, true,  false };
    struct weapon rifle   = { "Rifle",   20,  550.0f, 0.20f, 0.0f, false, false };
    struct weapon shotgun = { "Shotgun",  8,  400.0f, 0.50f, 0.0f, false, true  };

    pistol.weapon_texture     = LoadTexture("assets/In use assets/Weapons/tile_0010.png");
    pistol.crosshair_texture  = LoadTexture("assets/In use assets/Weapons/tile_0010 crosshair.png");
    smg.weapon_texture        = LoadTexture("assets/In use assets/Weapons/tile_0012.png");
    smg.crosshair_texture     = LoadTexture("assets/In use assets/Weapons/tile_0012 crosshair.png");
    sniper.weapon_texture     = LoadTexture("assets/In use assets/Weapons/tile_0013.png");
    sniper.crosshair_texture  = LoadTexture("assets/In use assets/Weapons/tile_0013 crosshair.png");
    rifle.weapon_texture      = LoadTexture("assets/In use assets/Weapons/tile_0015.png");
    rifle.crosshair_texture   = LoadTexture("assets/In use assets/Weapons/tile_0015 crosshair.png");
    shotgun.weapon_texture    = LoadTexture("assets/In use assets/Weapons/tile_0017.png");
    shotgun.crosshair_texture = LoadTexture("assets/In use assets/Weapons/tile_0017 crosshair.png");
    player1.currentweapon = pistol;

    // Array for bullets initializated
    struct bullet bullets[max_bullets];
    for (int i = 0; i < max_bullets; i++) bullets[i].active = false;

    //                    name,    x,   y, speed, health, damage
    struct enemy blue   = { "Blue",  0, 0, 100,  50, 10 };
    struct enemy yellow = { "Red",   0, 0, 120,  75, 15 };
    struct enemy gold   = { "Gold",  0, 0,  80, 100, 20 };
    struct enemy boss   = { "Boss",  0, 0,  60, 500, 30 };

    blue.frames[0]   = LoadTexture("assets/In use assets/Enemies/Enemy 1/tile_0000.png");
    blue.frames[1]   = LoadTexture("assets/In use assets/Enemies/Enemy 1/tile_0001.png");
    blue.frames[2]   = LoadTexture("assets/In use assets/Enemies/Enemy 1/tile_0002.png");
    blue.deadsprite  = LoadTexture("assets/In use assets/Enemies/Enemy 1/tile_0003.png");

    yellow.frames[0]  = LoadTexture("assets/In use assets/Enemies/Enemy 2/tile_0004.png");
    yellow.frames[1]  = LoadTexture("assets/In use assets/Enemies/Enemy 2/tile_0005.png");
    yellow.frames[2]  = LoadTexture("assets/In use assets/Enemies/Enemy 2/tile_0006.png");
    yellow.deadsprite = LoadTexture("assets/In use assets/Enemies/Enemy 2/tile_0007.png");

    gold.frames[0]  = LoadTexture("assets/In use assets/Enemies/Enemy 3/tile_0008.png");
    gold.frames[1]  = LoadTexture("assets/In use assets/Enemies/Enemy 3/tile_0009.png");
    gold.frames[2]  = LoadTexture("assets/In use assets/Enemies/Enemy 3/tile_0010.png");
    gold.deadsprite = LoadTexture("assets/In use assets/Enemies/Enemy 3/tile_0011.png");

    boss.frames[0]  = LoadTexture("assets/In use assets/Enemies/Enemy 4/tile_0012.png");
    boss.frames[1]  = LoadTexture("assets/In use assets/Enemies/Enemy 4/tile_0013.png");
    boss.frames[2]  = LoadTexture("assets/In use assets/Enemies/Enemy 4/tile_0014.png");
    boss.deadsprite = LoadTexture("assets/In use assets/Enemies/Enemy 4/tile_0015.png");
    boss.active     = false; // boss only active in final phase

    // maps
    Texture2D map1_texture = LoadTexture("assets/In use assets/Maps/Map1_final.png");  // purple night desert
    Texture2D map2_texture = LoadTexture("assets/In use assets/Maps/Map2.png");        // sandy day desert

    // starts with map 1 
    GameState currentState = STATE_MAP1; // remmeber the typedef emum we used back there

    int roundConfig[5][3] = {
    //  blue, yellow, gold counts for each round
        {2, 0, 0},
        {3, 2, 0},
        {4, 2, 1},
        {4, 4, 2},
        {6, 4, 3}, 
    };
    int currentRound = 0;   // 0-4 for round 1-5

    // fighting phase
    RoundPhase phase  = PHASE_FIGHTING;
    bool needsSpawn   = true;    // starts spawn

    
    struct enemy enemies[MAX_ENEMIES]; 
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active    = false;
        enemies[i].isDead    = false;
        enemies[i].deathTimer = 0.0f; // makes them active 
    }

    // setups enemy animation
    int   enemyFrame      = 0;
    float enemyFrameTimer = 0.0f;

    // BOSS fight stuff
    float bossShootTimer   = 0.0f;  // time since the last attack was fired
    float bossPatternTimer = 0.0f;  // time spent in the current pattern (has 3 attack patterns)
    int   bossPattern      = 0;     // 0 = spiral, 1 = aimed burst, 2 = ring blast
    float bossSpiralAngle  = 0.0f;  // accumulates to rotate the spiral each volley (for spin attack)
    int   bossFrame        = 0;     // animation frame for the boss sprite
    float bossFrameTimer   = 0.0f;

    // added different bullets to seperate from main player bullets
    struct bullet ebullets[MAX_ENEMY_BULLETS];
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) ebullets[i].active = false;

    // adds tick damage to avoid instakill
    float playerDmgCooldown = 0.0f;

    // intermission timer for between rounds
    #define INTERMISSION_DURATION 2.5f
    float intermissionTimer = 0.0f;

    // player death timer
    float playerDeathTimer   = 0.0f;
    bool  playerDeathPlaying = false; 

    // MAIN GAME LOOP
    while (!WindowShouldClose())
    {
        // deltatime tells me time it took to process the last frame
        float deltatime  = GetFrameTime();
        // currenttime tells me the time the window has been open for fire rate
        float currenttime = GetTime();
        // for animation of player
        bool ismoving = false;

        UpdateMusicStream(mus_phase1);
        UpdateMusicStream(mus_boss);
        UpdateMusicStream(mus_run);

        // GAME LOGIC
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld  = GetScreenToWorld2D(mouseScreen, camera); // makes mouse position relative to camera for better aiming instead of using GetMousePosition()

        // Tick player damage cooldown
        playerDmgCooldown = playerDmgCooldown - deltatime;

        // Weapon Switching temporary
        if (IsKeyPressed(KEY_ONE))   player1.currentweapon = pistol;
        if (IsKeyPressed(KEY_TWO))   player1.currentweapon = smg;
        if (IsKeyPressed(KEY_THREE)) player1.currentweapon = sniper;
        if (IsKeyPressed(KEY_FOUR))  player1.currentweapon = rifle;
        if (IsKeyPressed(KEY_FIVE))  player1.currentweapon = shotgun;

        // Movement of player
        // speed * deltatime makes movement based on frame time. in order to keep it consistent
        if(player1.health > 0){ // prevents movement when dead
        if (IsKeyDown(KEY_D)) { player1.x += player1.speed * deltatime; ismoving = true; }
        if (IsKeyDown(KEY_A)) { player1.x -= player1.speed * deltatime; ismoving = true; }
        if (IsKeyDown(KEY_W)) { player1.y -= player1.speed * deltatime; ismoving = true; }
        if (IsKeyDown(KEY_S)) { player1.y += player1.speed * deltatime; ismoving = true; }
        }

        // Collision for both maps
        if (currentState == STATE_MAP1)
        {
            if (player1.x < MAP1_BOUND_MIN) player1.x = MAP1_BOUND_MIN;
            if (player1.x > MAP1_BOUND_MAX) player1.x = MAP1_BOUND_MAX;
            if (player1.y < MAP1_BOUND_MIN) player1.y = MAP1_BOUND_MIN;
            if (player1.y > MAP1_BOUND_MAX) player1.y = MAP1_BOUND_MAX;
        }
        else // STATE_MAP2
        {
            if (player1.x < MAP2_BOUND_MIN) player1.x = MAP2_BOUND_MIN;
            if (player1.x > MAP2_BOUND_MAX) player1.x = MAP2_BOUND_MAX;
            if (player1.y < MAP2_BOUND_MIN) player1.y = MAP2_BOUND_MIN;
            if (player1.y > MAP2_BOUND_MAX) player1.y = MAP2_BOUND_MAX;
        }

        // Animation logic
        if (ismoving) {
            player1.frameTimer += deltatime;
            if (player1.frameTimer >= 0.15f) { // aka if enough time has passed, switch frame
                player1.currentframe = (player1.currentframe + 1) % 3; // Loop through 3 frames
                player1.frameTimer = 0;

            }
        } else {
            player1.currentframe = 0; // not moving
        }
        if (ismoving) {
            if (!IsMusicStreamPlaying(mus_run)) PlayMusicStream(mus_run);
                } else {
                    if (IsMusicStreamPlaying(mus_run)) StopMusicStream(mus_run);
                    player1.currentframe = 0;
                }


        // for camera movemnent
        Vector2 targetDestination = {
            player1.x * 0.80f + mouseWorld.x * 0.20f,
            player1.y * 0.80f + mouseWorld.y * 0.20f
        };

        // smoothning camra
        float smoothness = 5.0f * deltatime; 
        camera.target.x += (targetDestination.x - camera.target.x) * smoothness;
        camera.target.y += (targetDestination.y - camera.target.y) * smoothness;


        // shooting
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            if (currenttime - player1.currentweapon.lastfiretime >= player1.currentweapon.firerate)  // enough time since last shot
            {
                int shotCount = player1.currentweapon.isShotgun ? 6 : 1;  // shotgun 6 shots others 1 shot

                for (int shot = 0; shot < shotCount; shot++)
                {
                    for (int i = 0; i < max_bullets; i++)  // 6 times for shotgun 1 for other weapons
                    {
                        if (!bullets[i].active) // find inactive bullet slot
                        {
                            float baseAngle = atan2f(mouseWorld.y - player1.y, mouseWorld.x - player1.x);
                            // angle from player to mouse in radians using antan2f which gives us the angle via arc tangent of y/x,
                            // https://youtu.be/Nn41uuVfvwU?t=157

                            float finalAngle = baseAngle;
                            if (player1.currentweapon.isShotgun == true) { // for shotgun spread
                                float spread = (float)GetRandomValue(-10, 10);
                                finalAngle += spread * DEG2RAD; // convert spread to radians b/c that is what raylib uses and add to base angle
                            }
                            
                            // bullets array
                            bullets[i].active   = true;
                            bullets[i].hastrail = player1.currentweapon.isSniper; // mark for trail drawing
                            bullets[i].x        = player1.x;
                            bullets[i].y        = player1.y;
                            // Set initial trail point of starting
                            bullets[i].prevX = player1.x;
                            bullets[i].prevY = player1.y;

                            // final angle is the one we got above and this is it cos for x and sin for y to get the directional vector and then multiply by speed to get velocity of bullet
                            bullets[i].dx = cosf(finalAngle) * player1.currentweapon.bulletspeed;
                            bullets[i].dy = sinf(finalAngle) * player1.currentweapon.bulletspeed;

                            break; // breaks once appropriate exit is found
                        }
                    }
                }
                player1.currentweapon.lastfiretime = currenttime; // reset fire timer

                player1.currentweapon.lastfiretime = currenttime;

                // Play the right weapon sound
                if      (player1.currentweapon.isSniper)  PlaySound(snd_sniper);
                else if (player1.currentweapon.isShotgun) PlaySound(snd_shotgun);
                else if (strcmp(player1.currentweapon.name, "SMG")   == 0) PlaySound(snd_rifle);
                else if (strcmp(player1.currentweapon.name, "Rifle") == 0) PlaySound(snd_rifle);
                else if (strcmp(player1.currentweapon.name, "Pistol") == 0) PlaySound(snd_rifle); // pistol uses rifle sound
            }
        }

        // Update Bullets
        for (int i = 0; i < max_bullets; i++)
        {
            if (bullets[i].active) // finds the active bullets and updates them
            {
                // Save current position as previous before moving
                bullets[i].prevX = bullets[i].x;
                bullets[i].prevY = bullets[i].y;

                // move bullet based on velocity and deltatime for consistency across each frame
                bullets[i].x += bullets[i].dx * deltatime;
                bullets[i].y += bullets[i].dy * deltatime;

                if (bullets[i].x < -3000 || bullets[i].x > 4000 || 
                    bullets[i].y < -3000 || bullets[i].y > 4000)
                {
                    bullets[i].active = false;
                } //removes bullets when they are far away
            }
        }

        // wave system aka enemy spawning system
        if (phase == PHASE_FIGHTING && currentState == STATE_MAP1)
        {
            // if enemies need to be spawned
            if (needsSpawn)
            {
                needsSpawn = false;
                int slot   = 0;

                int blueCount   = roundConfig[currentRound][0];
                int yellowCount = roundConfig[currentRound][1];
                int goldCount   = roundConfig[currentRound][2];

                for (int n = 0; n < blueCount;   n++, slot++)  // calls function for each enemy
                    spawnEnemy(enemies, slot, &blue,   MAP1_BOUND_MIN, MAP1_BOUND_MAX);
                for (int n = 0; n < yellowCount; n++, slot++)
                    spawnEnemy(enemies, slot, &yellow, MAP1_BOUND_MIN, MAP1_BOUND_MAX);
                for (int n = 0; n < goldCount;   n++, slot++)
                    spawnEnemy(enemies, slot, &gold,   MAP1_BOUND_MIN, MAP1_BOUND_MAX);
            }

            // enemy animation
            enemyFrameTimer += deltatime;
            if (enemyFrameTimer >= 0.15f) {
                enemyFrame      = (enemyFrame + 1) % 3;
                enemyFrameTimer = 0.0f;
            }

            // Enemy AI
            for (int i = 0; i < MAX_ENEMIES; i++)
            {
                if (!enemies[i].active) continue;

                // Direction vector from enemy to player
                float toPlayerX = player1.x - enemies[i].x;
                float toPlayerY = player1.y - enemies[i].y;
                float dist      = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);
                // pythagorean theorem sqrt(x^2 + y^2)

                if (dist > 1.0f) { // avoid divide-by-zero when exactly on top
                    enemies[i].x += (toPlayerX / dist) * enemies[i].speed * deltatime;
                    enemies[i].y += (toPlayerY / dist) * enemies[i].speed * deltatime;
                }

                // Contact damage: only when touching and cooldown has expired
                if (dist < 35.0f && playerDmgCooldown <= 0.0f) {
                    player1.health    = player1.health - enemies[i].damage; // damage
                    playerDmgCooldown  = 0.6f; // some time so no instakill
                    PlaySound(snd_damage);
                }
            }

            // Bullets hitting enemy
            for (int b = 0; b < max_bullets; b++) {
                if (!bullets[b].active) continue; // skips inactive bullets

                for (int e = 0; e < MAX_ENEMIES; e++) { // check all enemy

                    if (!enemies[e].active) continue; // if enemy not active skip

                    float dx = bullets[b].x - enemies[e].x;
                    float dy = bullets[b].y - enemies[e].y;

                    // pythagorus theorem again
                    if (sqrtf(dx*dx + dy*dy) < 20.0f) {         // hit radius

                        enemies[e].health -= player1.currentweapon.damage;
                        bullets[b].active  = false; // bullet is consumed on hit

                        // Start the death linger instead of instantly removing the enemy
                        if (enemies[e].health <= 0) {
                            enemies[e].active    = false;
                            enemies[e].isDead    = true;
                            enemies[e].deathTimer = 0.0f;
                        }
                        break; // one bullet can only hit one enemy
                    }
                }
            }

            // Dead enemies show their corpse sprite for DEATH_LINGER_TIME seconds.
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].isDead) continue;
                enemies[i].deathTimer += deltatime;
                if (enemies[i].deathTimer >= DEATH_LINGER_TIME)
                    enemies[i].isDead = false; 
            }

            // if all enemies are dead goes to next round or boss fight
            int stillAround = 0;
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (enemies[i].active) stillAround = 1; // if any enemy alive it stays 1 and does not enter next stage

            if (stillAround == 0) {
                if (currentRound < 4) {
                    // More waves to go enter intermission stage then next round
                    phase           = PHASE_INTERMISSION;
                    intermissionTimer = 0.0f;
                } else {
                    phase        = PHASE_BOSS;
                    currentState = STATE_MAP2;
                    StopMusicStream(mus_phase1);
                    PlayMusicStream(mus_boss);   // ← add these two
                    
                    // places player 2 in map below boss
                    player1.x = MAP_PIXEL_SIZE / 2.0f;
                    player1.y = MAP_PIXEL_SIZE / 2.0f + 300.0f; 

                    // Activate boss at the north of the arena
                    boss.active = true;
                    boss.health = 500;
                    boss.x      = MAP_PIXEL_SIZE / 2.0f;
                    boss.y      = MAP2_BOUND_MIN + 120.0f;

                    // Clear stray player bullets so the arena starts clean
                    for (int i = 0; i < max_bullets; i++) bullets[i].active = false;
                }
            }

            // player dies
            if (player1.health <= 0 && !playerDeathPlaying) {
                playerDeathPlaying = true;
                playerDeathTimer   = 0.0f;
            }
        } // end of fighting loop

        // intermission break
        if (phase == PHASE_INTERMISSION)
        {
            intermissionTimer += deltatime;
            if (intermissionTimer >= INTERMISSION_DURATION) {
                currentRound++;     // advance to next round (0-indexed)
                needsSpawn = true;  // let the wave logic spawn enemies next frame
                phase      = PHASE_FIGHTING;
            }
        }

        // player is dead lets the sprite stay after death
        if (playerDeathPlaying) {
            playerDeathTimer += deltatime;
            if (playerDeathTimer >= DEATH_LINGER_TIME)
                phase = PHASE_GAMEOVER;
        }

        // BOSS
        // RoundPhase phase = PHASE_BOSS;
        // boss.active = true;
        // currentState  = STATE_MAP2;
        if (phase == PHASE_BOSS && boss.active)
        {
            // boss spawns in centre
            float arenaX      = MAP_PIXEL_SIZE / 2.0f;
            float arenaY      = MAP_PIXEL_SIZE / 2.0f;

            // small circle orbit where boss sits and moves it in a circle
            float offsetX     = boss.x - arenaX; // Arenax and y are 630 half of 1260
            float offsetY     = boss.y - arenaY;
            float orbitAngle  = atan2f(offsetY, offsetX) + 0.35f * deltatime; // slow rotation
            float orbitRadius = 240.0f;
            boss.x = arenaX + cosf(orbitAngle) * orbitRadius;
            boss.y = arenaY + sinf(orbitAngle) * orbitRadius;

            // boss animation
            bossFrameTimer += deltatime;
            if (bossFrameTimer >= 0.15f) {
                bossFrame      = (bossFrame + 1) % 3;
                bossFrameTimer = 0.0f;
            }

            // changes shooting pattern every 5 seconds
            bossPatternTimer += deltatime;
            if (bossPatternTimer >= 5.0f) {
                bossPattern      = (bossPattern + 1) % 3; // picks 1 to 3 for it
                bossPatternTimer = 0.0f;
                bossShootTimer   = 0.0f; // reset shot timer on pattern change
            }

            bossShootTimer += deltatime;

            // Pattern 0 spiral

            if (bossPattern == 0 && bossShootTimer >= 0.15f) {
                bossSpiralAngle += 22.5f * DEG2RAD;
                for (int shot = 0; shot < 8; shot++) {
                    float angle = bossSpiralAngle + shot * (360.0f / 8.0f) * DEG2RAD;
                    fireEnemyBullet(ebullets, MAX_ENEMY_BULLETS, boss.x, boss.y, angle, 200.0f);
                }
                // changes angle for every shot making a spiral pattern
                bossShootTimer = 0.0f;
            }

            // Pattern 1 aimed burst
            else if (bossPattern == 1 && bossShootTimer >= 0.55f) {
                float aimAngle = atan2f(player1.y - boss.y, player1.x - boss.x);
                for (int shot = -2; shot <= 2; shot++) { // 5 shots spread across 48 degrees
                    float angle = aimAngle + shot * 12.0f * DEG2RAD;
                    fireEnemyBullet(ebullets, MAX_ENEMY_BULLETS, boss.x, boss.y, angle, 200.0f);
                }
                bossShootTimer = 0.0f;
            }

            // Pattern 2 ring attack

            else if (bossPattern == 2 && bossShootTimer >= 0.85f) {
                for (int shot = 0; shot < 12; shot++) {
                    float angle = shot * (360.0f / 12.0f) * DEG2RAD;
                    fireEnemyBullet(ebullets, MAX_ENEMY_BULLETS, boss.x, boss.y, angle, 200.0f);
                }
                bossShootTimer = 0.0f;
            }

            // update enemy bullets
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                if (!ebullets[i].active) continue;
                ebullets[i].prevX = ebullets[i].x;
                ebullets[i].prevY = ebullets[i].y;
                ebullets[i].x    += ebullets[i].dx * deltatime;
                ebullets[i].y    += ebullets[i].dy * deltatime;

                // Deactivate once the bullet leaves the map bounds
                if (ebullets[i].x < -200 || ebullets[i].x > MAP_PIXEL_SIZE + 200 ||
                    ebullets[i].y < -200 || ebullets[i].y > MAP_PIXEL_SIZE + 200)
                    ebullets[i].active = false;
            }

            // bullets hitting boss
            for (int b = 0; b < max_bullets; b++) {
                if (!bullets[b].active) continue;
                float dx = bullets[b].x - boss.x;
                float dy = bullets[b].y - boss.y;
                if (sqrtf(dx*dx + dy*dy) < 32.0f) { // boss hit radius slightly larger
                    boss.health       -= player1.currentweapon.damage;
                    bullets[b].active  = false;
                }
            }

            // boss bullets hitting me
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                if (!ebullets[i].active) continue;
                float dx = ebullets[i].x - player1.x;
                float dy = ebullets[i].y - player1.y;
                if (sqrtf(dx*dx + dy*dy) < 18.0f && playerDmgCooldown <= 0.0f) {
                    player1.health     -= boss.damage;
                    ebullets[i].active  = false; // bullet is destroyed on hit
                    playerDmgCooldown   = 0.2f;
                }
            }

            // if you beat boss u win
            if (boss.health <= 0) { boss.active = false; phase = PHASE_WIN; }

            // Trigger death linger before game over to show death sprite
            if (player1.health <= 0 && !playerDeathPlaying) {
                playerDeathPlaying = true;
                playerDeathTimer   = 0.0f;
                PlaySound(snd_gameover);   // ← add this
                StopMusicStream(mus_phase1);
                StopMusicStream(mus_boss);
            }
        }

        // DRAWING STEP
        BeginDrawing();
            ClearBackground(BLACK); // makes stuff behind map black
            BeginMode2D(camera);

            // draw map white is basically transparent so we are just drawing the map
            if (currentState == STATE_MAP1)
            {
                DrawTexture(map1_texture, 0, 0, WHITE); // purple night desert
            }
            else // STATE_MAP2
            {
                DrawTexture(map2_texture, 0, 0, WHITE); // sandy day desert
            }

/*DrawTexturePro(texture, source, destination, origin, rotation, tint)
        this is the function we use to draw sprites
  Rectangle(x,y,width,height)
  DrawRectangle(x, y, width, height, color)
  DrawCircle(x, y, radius, color)
  DrawText("abc", x, y, font size, color)
*/
            
            // drawing enemies during waves
            if (phase == PHASE_FIGHTING || phase == PHASE_INTERMISSION)
            {
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    // corpse sprite
                    if (enemies[i].isDead) {
                        Texture2D* corpse = &enemies[i].deadsprite;
                        DrawTexturePro(*corpse,
                            (Rectangle){0, 0, (float)corpse->width, (float)corpse->height},
                            (Rectangle){enemies[i].x, enemies[i].y,
                                        (float)corpse->width  * 1.5f,
                                        (float)corpse->height * 1.5f},
                            (Vector2){(float)corpse->width  * 0.75f,
                                      (float)corpse->height * 0.75f},
                            0.0f, WHITE); continue; // skip live-enemy drawing below
                    }

                    if (!enemies[i].active) continue;
                    // enemies sprite 
                    Texture2D* frame = enemies[i].frames;
                    DrawTexturePro(frame[enemyFrame],
                        (Rectangle){0, 0, (float)frame[enemyFrame].width, (float)frame[enemyFrame].height},
                        (Rectangle){enemies[i].x, enemies[i].y,
                                    (float)frame[enemyFrame].width  * 1.5f,
                                    (float)frame[enemyFrame].height * 1.5f},
                        (Vector2){(float)frame[enemyFrame].width  * 0.75f,
                                  (float)frame[enemyFrame].height * 0.75f},
                        0.0f, WHITE);

                    // health bar above enemy
                    int barWidth = 40;
                    int maxHP    = (enemies[i].damage == 10) ? 50
                                 : (enemies[i].damage == 15) ? 75 : 100; // inferred by type
                    int fillWidth = (int)(barWidth * ((float)enemies[i].health / maxHP));
                    DrawRectangle((int)enemies[i].x - barWidth/2, (int)enemies[i].y - 32, barWidth,    5, DARKGRAY);
                    DrawRectangle((int)enemies[i].x - barWidth/2, (int)enemies[i].y - 32, fillWidth,   5, GREEN);
                }
            }

            // boss 3x scale 
            if (phase == PHASE_BOSS && boss.active) {
                Texture2D* big_frame = boss.frames;
                DrawTexturePro(big_frame[bossFrame],
                    (Rectangle){0, 0, (float)big_frame[bossFrame].width, (float)big_frame[bossFrame].height},
                    (Rectangle){boss.x, boss.y,
                                (float)big_frame[bossFrame].width  * 3.0f,
                                (float)big_frame[bossFrame].height * 3.0f},
                    (Vector2){(float)big_frame[bossFrame].width  * 1.5f,
                              (float)big_frame[bossFrame].height * 1.5f},
                    0.0f, RED);

                // World-space health bar directly above the boss sprite
                int bossBarW  = 90;
                int bossBarFill = (int)(bossBarW * ((float)boss.health / 500.0f));
                if (bossBarFill < 0) bossBarFill = 0;
                DrawRectangle((int)boss.x - bossBarW/2, (int)boss.y - 72, bossBarW,    8, DARKGRAY);
                DrawRectangle((int)boss.x - bossBarW/2, (int)boss.y - 72, bossBarFill, 8, MAROON);
            }

            // boss projectile very fancy
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                if (!ebullets[i].active) continue;
                DrawCircle((int)ebullets[i].x, (int)ebullets[i].y, 8, ORANGE);
                DrawCircle((int)ebullets[i].x, (int)ebullets[i].y, 4, YELLOW);
            }

            for (int i = 0; i < max_bullets; i++) { // draws each bullet
                if (bullets[i].active) {
                    if (bullets[i].hastrail) {
                        DrawLineEx((Vector2){bullets[i].prevX, bullets[i].prevY}, (Vector2){bullets[i].x, bullets[i].y}, 1.0f, RED); \
                        // DrawLineEx((Vector2){previous position}, (Vector 2){current position}, thickness, colour); line from player to mouse for sniper bullet trail
                    }
                    DrawCircle(bullets[i].x, bullets[i].y, 5, BLACK); // draws bullet
                }
            }

            // draw player
            // dead player
            if (playerDeathPlaying) {
                // Draw the player dead sprite centred on player position
                Texture2D dead = player1.deadsprite;
                DrawTexturePro(dead,
                    (Rectangle){0, 0, (float)dead.width, (float)dead.height},
                    (Rectangle){player1.x, player1.y, (float)dead.width * 2.0f, (float)dead.height * 2.0f},
                    (Vector2){(float)dead.width, (float)dead.height},
                    0.0f, WHITE);
            } else {

            // FLIPPING FOR CHARACTER DIRECTION
            float flip;
            if (mouseWorld.x < player1.x)  flip = -1.0f; // looiking left
            else                           flip =  1.0f; // Look Right

            Rectangle source_rectangle = { 0.0f, // Start x on texture
                                           0.0f, // Start y on texture
                                        (float)player1.walkingframes[player1.currentframe].width * flip, // width -1 mirrors it
                                        (float)player1.walkingframes[player1.currentframe].height };     // height same

            Rectangle destination_rectangle = { player1.x, // x and y position of player
                                                player1.y,
                                                (float)player1.walkingframes[player1.currentframe].width  * 2.0f,  // 2x size for visibility
                                                (float)player1.walkingframes[player1.currentframe].height * 2.0f };

            Vector2 origin = { (float)player1.walkingframes[player1.currentframe].width,
                               (float)player1.walkingframes[player1.currentframe].height };

            DrawTexturePro(player1.walkingframes[player1.currentframe], // what to draw
                           source_rectangle,              // which part of texture to use all of it i guess
                           destination_rectangle,         // how big to draw it and where the 2x one
                           origin,                        // center part of it
                           0.0f,                          // rotation (none)
                           WHITE);                        // WHITE = no tint, just draw the texture as is

            // Weapon drawing including FLIP functionality
            float angle = atan2f(mouseWorld.y - player1.y, mouseWorld.x - player1.x) * RAD2DEG; // angle from player to mouse

            float weaponFlipY;  // flips weapon on light side if needed same logic as above
            if (mouseWorld.x < player1.x) weaponFlipY = -1.0f;
            else                          weaponFlipY =  1.0f;

            Rectangle weapon_source = { 0.0f, 0.0f, (float)player1.currentweapon.weapon_texture.width, (float)player1.currentweapon.weapon_texture.height * weaponFlipY };

            Rectangle weapon_destination = { player1.x, player1.y + 5, // offset for gun
                                            (float)player1.currentweapon.weapon_texture.width  * 1.5f,
                                            (float)player1.currentweapon.weapon_texture.height * 1.5f }; // 1.5 scale

            Vector2 weapon_origon = { 0.0f, (float)(player1.currentweapon.weapon_texture.height * 1.5f) / 2.0f };

            DrawTexturePro(player1.currentweapon.weapon_texture, weapon_source, weapon_destination, weapon_origon, angle, WHITE);


            } 

            EndMode2D();

            // draw crosshair asset at screen position
            DrawTexture(player1.currentweapon.crosshair_texture, mouseScreen.x - player1.currentweapon.crosshair_texture.width/2, mouseScreen.y - player1.currentweapon.crosshair_texture.height/2, WHITE);

            // HUD improve laterr
            DrawText(TextFormat("Weapon: %s", player1.currentweapon.name), 10, 10, 20, DARKGRAY);
            DrawText("Hold Left Click to Shoot | Press 1-5 to switch", 10, 40, 20, LIGHTGRAY);

                
            //  Player health bar (bottom-left, always visible) 
            int hpFill = (int)(200 * ((float)player1.health / 100.0f));
            if (hpFill < 0) hpFill = 0;
            DrawRectangle(10, screen_height - 28, 200, 18, DARKGRAY);
            DrawRectangle(10, screen_height - 28, hpFill, 18,
                          player1.health > 40 ? GREEN : RED); // turns red when low
            DrawText(TextFormat("HP: %d", player1.health > 0 ? player1.health : 0),
                     10, screen_height - 50, 20, WHITE);

            // Round counter (top-right, shown during wave phases) 
            if (phase == PHASE_FIGHTING || phase == PHASE_INTERMISSION)
                DrawText(TextFormat("Round: %d / 5", currentRound + 1),
                         screen_width - 210, 10, 24, YELLOW);

            // Boss health bar (top-centre wide bar, shown during boss phase)
            if (phase == PHASE_BOSS && boss.active) {
                int bsFill = (int)(500 * ((float)boss.health / 500.0f));
                if (bsFill < 0) bsFill = 0;
                DrawRectangle(screen_width/2 - 250, 10, 500, 22, DARKGRAY);
                DrawRectangle(screen_width/2 - 250, 10, bsFill, 22, MAROON);
                DrawText("BOSS", screen_width/2 - 22, 13, 20, WHITE);

                // Show the current pattern so the player knows what to expect next
                const char* patNames[] = { "Spiral", "Aimed Burst", "Ring Blast" };
                DrawText(TextFormat("[ %s ]", patNames[bossPattern]),
                         screen_width/2 - 55, 40, 18, ORANGE);
            }

            // intermission show case
            if (phase == PHASE_INTERMISSION) {
                DrawRectangle(0, 0, screen_width, screen_height, (Color){0, 0, 0, 130});
                DrawText(TextFormat("ROUND %d CLEAR!", currentRound + 1),
                         screen_width/2 - 195, screen_height/2 - 36, 52, YELLOW);
                DrawText(TextFormat("Next round in %.0f...",
                         INTERMISSION_DURATION - intermissionTimer),
                         screen_width/2 - 130, screen_height/2 + 28, 28, WHITE);
            }

            // ─── Game Over overlay ─────────────────────────────────────────────
            if (phase == PHASE_GAMEOVER) {
                DrawRectangle(0, 0, screen_width, screen_height, (Color){0, 0, 0, 170});
                DrawText("GAME OVER", screen_width/2 - 180, screen_height/2 - 50, 72, RED);
                DrawText(TextFormat("Survived %d / 6 rounds", currentRound + 1),
                         screen_width/2 - 165, screen_height/2 + 40, 28, LIGHTGRAY);
                DrawText("Close the window to quit",
                         screen_width/2 - 130, screen_height/2 + 84, 22, GRAY);
            }

            // win overlay
            if (phase == PHASE_WIN) {
                DrawRectangle(0, 0, screen_width, screen_height, (Color){0, 0, 0, 170});
                DrawText("YOU WIN!", screen_width/2 - 150, screen_height/2 - 56, 80, GOLD);
                DrawText("The desert is safe for now...",
                         screen_width/2 - 210, screen_height/2 + 44, 28, WHITE);
                DrawText("Close the window to quit",
                         screen_width/2 - 130, screen_height/2 + 86, 22, GRAY);
            }

        EndDrawing();
    }

    // Unloading assets
    UnloadTexture(pistol.weapon_texture);
    UnloadTexture(pistol.crosshair_texture);
    UnloadTexture(smg.weapon_texture);
    UnloadTexture(smg.crosshair_texture);
    UnloadTexture(sniper.weapon_texture);
    UnloadTexture(sniper.crosshair_texture);
    UnloadTexture(rifle.weapon_texture);
    UnloadTexture(rifle.crosshair_texture);
    UnloadTexture(shotgun.weapon_texture);
    UnloadTexture(shotgun.crosshair_texture);
    UnloadTexture(player1.deadsprite);
    UnloadTexture(blue.deadsprite);
    UnloadTexture(yellow.deadsprite);
    UnloadTexture(gold.deadsprite);
    UnloadTexture(boss.deadsprite);

    for (int i = 0; i < 3; i++) {
        UnloadTexture(player1.walkingframes[i]);
        UnloadTexture(blue.frames[i]);
        UnloadTexture(yellow.frames[i]);
        UnloadTexture(gold.frames[i]);
        UnloadTexture(boss.frames[i]);
    }

    // Unload map textures
    UnloadTexture(map1_texture);
    UnloadTexture(map2_texture);

    UnloadSound(snd_rifle);
    UnloadSound(snd_smg);
    UnloadSound(snd_sniper);
    UnloadSound(snd_shotgun);
    UnloadSound(snd_damage);
    UnloadSound(snd_gameover);
    UnloadMusicStream(mus_phase1);
    UnloadMusicStream(mus_boss);
    UnloadMusicStream(mus_run);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}
