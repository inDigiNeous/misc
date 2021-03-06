/*
 * list of things needed for a Real Game Engine:
 *
 * - Sprite handling. Loading, etc.
 * - Collision checking with general sprite structures, bounding boxes
 * - Palette loading and management
 *
 * list of things needed for Graphics Conversion Tools:
 *
 * - Palette management .. combine palettes etc
 *
 *---------------------------------------
 *
 * Flow of the game:
 *
 * - The car comes from the left.
 * - You try to destroy it .. with different weapons. Three (3) difficulties
 *   for each weapon.
 * - After succesfully destroying the car with one weapon, you advance to the
 *   next weapon, and try it again three times.
 *
 * Text between levels:
 *
 * "GET READY .." plays that annoying tune - "ANNIHILATE!"
 *
 * "YOU FAILED!"
 *
 * "NEXT LEVEL"
 *
 * "GET READY .."
 *
 * "ANNIHILATE"
 *
 * "NEW WEAPON!"
 *
 * Difficulty levels:
 *  - 1. Car comes slow, does not stop.
 *  - 2. Car comes slightly faster, might stop at the lights.
 *  - 3. Car comes fast, almost always stops at the lights.
 *
 * - First weapon: bazooka
 *   - Simply fires a missile. Able to launch 1 at a time.
 *
 * - Second weapon: grenade
 *   - Launches a grenade that has a slight delay in exploding, so that it is
 *   hard to have a direct explosion, but easy to throw it to the ground and
 *   wait for it to explode.
 *
 * - Third weapon: 
 *
 */

#include <stdio.h>
#include "lgl.h"
#include "posprintf.h"

#include "data_player.h"
#include "data_car.h"
#include "data_font.h"
#include "data_missile.h"
#include "data_explosion.h"

enum SPRITE_STATES
{
	WALK,
	STAND,
	STOP,
	DRIVE,
	EXPLODE,
	EXPLODING,
	DESTROYED,
	LAUNCH,
	FLY,
	IMPACT
};

enum GAME_STATES
{
	INIT,
	INTRO,
	START,
	PLAY,
	ADVANCE_LEVEL,
	NEW_WEAPON,
	FAIL,
	GAME_OVER
};

enum SPRITE_TYPES
{
	PLAYER,
	CAR_ICECREAM,
	MISSILE,
	EXPLOSION
};

enum CAR_DEFS
{
	CAR_H = 32,
	CAR_W = 64,
	EXP_H = 32,
	EXP_W = 32
};

enum GAME_DEFS
{
	GSH = FSH - 4,
	PLAYER_SPEED = TOFIX(6, GSH),
	DEF_PLAYER_LIVES = 3,
	PLAYER_DELAY = 10,
	EXPLOSION_DELAY = 2,
	MISSILE_SPEED = TOFIX(20, GSH),
	CAR_SPEED = TOFIX(8, GSH), 
	SCORE_DESTROY_CAR = 25,
	POS_SCORE_X = 26,
	POS_SCORE_Y = 1,
	POS_LEVEL_X = 7,
	POS_LEVEL_Y = POS_SCORE_Y,
	POS_LIVES_X = 26,
	POS_LIVES_Y = POS_SCORE_Y + 1
};

enum DIRECTIONS
{
	DIR_LEFT = -1,
	DIR_RIGHT = 1
};

typedef struct Car Car;
struct Car
{
	fixed x, y;
	fixed speed;
	int type;
	int dx;
	int state;
	int frame;
	int index;
	int moving;
};

typedef struct Player Player;
struct Player
{
	fixed x, y;
	int dir;
	int frame;
	int counter;
	int ammo;
	int state;
	int index;
	int score;
	int lives;
};

typedef struct Ammo Ammo;
struct Ammo
{
	fixed x, y;
	int dx, dy;
	int state;
	int index;
};

typedef struct Explosion Explosion;
struct Explosion
{
	fixed x, y;
	int dx, dy;
	int state;
	int index;
	int frame;
	int counter;
};

int frames;
int gamestate;

Player p1;
Car car;
Ammo m1;
Explosion e1;

void showIntro(void)
{
	putBgText(BG3, 1, 2, 0,
		"YOUR MISSION,\nSHOULD YOU ACCEPT IT:\n\n"
		"PROTECT THE SANITY OF YOUR\n"
		"FELLOW CITIZENS. WHEN YOU\n"
		"HEAR THE DEVILISH TUNE,\n"
		"GET YOUR ARMS READY AND\n"
		"BECOME THE CITY GUERRILLA\n"
		"OF THE DIGITAL CENTURY!\n\n\n\n\n"
		"PRESS START TO CONTINUE");

	//while (keyUp(KEY_ANY));

	clearBgText(BG3);
}

void initGameData(void)
{
	setVideoMode(VID_MODE0 | VID_ENABLE_OBJ | VID_ENABLE_BG3 | VID_LINEAR);

	dmaCopy(&TileMem4[TB_BG0][ASCII_START], data_font, 8 * 64);
	setupBg(BG3, 31, BG_SIZE_256x256, BG_WRAP | BG_PRI_1);
	PAL_BG_PTR[1] = COLOR_WHITE;

	/* First sub pal is for the player */
	dmaCopy(PAL_OBJ_PTR, data_player_pal, 5);
	/* Second sub pal is for the icecream car */
	dmaCopy(PAL_OBJ_PTR + 16, data_car_pal, 6);
	/* Third sub pal is for the missile */
	dmaCopy(PAL_OBJ_PTR + 32, data_missile_pal, 8);
	/* Fourth for the explosion */
	dmaCopy(PAL_OBJ_PTR + 48, data_explosion_pal, 2);

	/* Copy tile data for game objects */
	copySpriteTiles((u32 *)data_player[0], DATA_PLAYER_LEN, PLAYER);
	copySpriteTiles((u32 *)data_explosion[0], DATA_EXPLOSION_LEN, EXPLOSION);
	copySpriteTiles((u32 *)data_car, DATA_CAR_LEN, CAR_ICECREAM);
	copySpriteTiles((u32 *)data_missile, DATA_MISSILE_LEN, MISSILE);
}

/* initialize game objects, this is so small game that we can load all the
 * sprites manually and just assign every sprite to one game object */
void initGameObjects(void)
{
	int attr2;

	initSprites();
	/* Add the car */

	attr2 = spriteTileIndexes[CAR_ICECREAM] | OAM_A2_PAL_1 | OAM_A2_PRI_2;
	setSpriteAttr(spriteIndex, OAM_A0_WIDE, OAM_A1_SIZE_32, attr2);
	car.index = spriteIndex;
	spriteIndex++;

	/* Add the player */
	attr2 = spriteTileIndexes[PLAYER] | OAM_A2_PAL_0 | OAM_A2_PRI_0;
	setSpriteAttr(spriteIndex, OAM_A0_TALL, OAM_A1_SIZE_8, attr2);
	p1.index = spriteIndex;
	spriteIndex++;

	/* Add missile .. don't show */
	attr2 = spriteTileIndexes[MISSILE] | OAM_A2_PAL_2 | OAM_A2_PRI_1;
	setSpriteAttr(spriteIndex, OAM_A0_SQUARE, OAM_A1_SIZE_8, attr2);
	m1.index = spriteIndex;
	spriteIndex++;
	
	/* Add explosion .. don't show */
	attr2 = spriteTileIndexes[EXPLOSION] | OAM_A2_PAL_3 | OAM_A2_PRI_1;
	setSpriteAttr(spriteIndex, OAM_A0_SQUARE, OAM_A1_SIZE_32, attr2);
	e1.index = spriteIndex;
	spriteIndex++;
}

void resetGameObjects(void)
{
	car.x = TOFIX(260, FSH);
	car.y = TOFIX(80, FSH);
	car.speed = CAR_SPEED;
	car.state = DRIVE;
	setSpritePos(car.index, TOINT(car.x, FSH), TOINT(car.y, FSH));
	
	p1.x = TOFIX(104, FSH);
	p1.y = TOFIX(140, FSH);
	p1.counter = 0;
	p1.frame = 0;
	p1.score = 0;
	p1.lives = DEF_PLAYER_LIVES;
	p1.state = WALK;
	setSpritePos(p1.index, TOINT(p1.x, FSH), TOINT(p1.y, FSH));
	
	m1.x = TOFIX(240, FSH);
	m1.y = TOFIX(160, FSH);
	m1.state = DESTROYED;
	setSpritePos(m1.index, TOINT(m1.x, FSH), TOINT(m1.y, FSH));
	
	e1.x = TOFIX(240, FSH);
	e1.y = TOFIX(160, FSH);
	e1.state = DESTROYED;
	e1.frame = 0;
	e1.counter = 0;
	setSpritePos(e1.index, TOINT(e1.x, FSH), TOINT(e1.y, FSH));
}

void updateScore(int score, int lives, int level)
{
	char str[3];

	posprintf(str, "%d", score);
	putBgTile(BG3, POS_SCORE_X, POS_SCORE_Y, str[0]);
	putBgTile(BG3, POS_SCORE_X + 1, POS_SCORE_Y, str[1]);
	putBgTile(BG3, POS_SCORE_X + 2, POS_SCORE_Y, str[2]);
	
	posprintf(str, "%d", level);
	putBgTile(BG3, POS_LEVEL_X, POS_LEVEL_Y, str[0]);
	putBgTile(BG3, POS_LEVEL_X + 1, POS_LEVEL_Y, str[1]);

	posprintf(str, "%d", lives);
	putBgTile(BG3, POS_LIVES_X, POS_LIVES_Y, str[0]);
	putBgTile(BG3, POS_LIVES_X + 1, POS_LIVES_Y, str[1]);
}

void writeText(int bg, int x, int y, int delay, char *str)
{
	int c = 0;
	char *ptr = str;

	while (*ptr != '\0')
	{
		putBgTile(bg, x, y, *ptr);
		x++;
		ptr++;

		for (c=0; c < delay; c++)
			waitVsync();
	}
}

void play(void)
{
	/* Player stuff */
	if (p1.state == WALK)
	{
		p1.counter++;
		/* Animate */
		if (p1.counter > PLAYER_DELAY)
		{
			p1.frame++;
			p1.counter = 0;

			if (p1.frame > 1)
				p1.frame = 0;

			/* Alternate frames */
			dmaCopy(&TileMem4[TB_OBJ][0], \
				(u32 *)data_player[p1.frame], 16);
		}

		/* Move and check boundaries */
		p1.x += PLAYER_SPEED * p1.dir;
		if (TOINT(p1.x, FSH) > (SCREEN_W - 8))
			p1.x = TOFIX(SCREEN_W - 8, FSH);
		else if (TOINT(p1.x, FSH) < 0)
			p1.x = TOFIX(0, FSH);

		setSpritePos(p1.index, TOINT(p1.x, FSH), 
			TOINT(p1.y, FSH));
	}

	/* Handle input */
	if (keyDown(KEY_LEFT) || keyDown(KEY_RIGHT))
	{
		if (keyDown(KEY_LEFT))
			p1.dir = DIR_LEFT;
		else if (keyDown(KEY_RIGHT))
			p1.dir = DIR_RIGHT;

		if (keyDown(KEY_LEFT) && keyDown(KEY_RIGHT))
			p1.state = STAND;
		else
			p1.state = WALK;
	}
	else if (keyUp(KEY_LEFT) || keyUp(KEY_RIGHT))
		p1.state = STAND;

	/* Missile stuff */
	if (keyDown(KEY_A))
	{
		if (m1.state == DESTROYED)
			m1.state = LAUNCH;
	}
	if (m1.state == LAUNCH)
	{
		/* Missile launching */
		m1.x = p1.x + 10;
		m1.y = p1.y - 10;
		m1.state = FLY;
	}
	else if (m1.state == FLY)
	{
		/* Missile flying */
		m1.x = m1.x + MISSILE_SPEED;
		m1.y = m1.y - MISSILE_SPEED;

		/* Check boundaries */
		if (TOINT(m1.x, FSH) > SCREEN_W || 
			TOINT(m1.y, FSH) < -8)
			m1.state = DESTROYED;

		/* Check collision with car */
		if (TOINT(m1.y, FSH) < (TOINT(car.y, FSH) + 8) \
			&& TOINT(m1.y, FSH) > (TOINT(car.y, FSH)) \
			&& TOINT(m1.x, FSH) > (TOINT(car.x, FSH) + 2) \
			&& TOINT(m1.x, FSH) < (TOINT(car.x, FSH) + 29))
		{
			/* Explode car and explosion, destroy
			 * missile */
			car.state = EXPLODE;
			e1.state = EXPLODE;
			m1.state = DESTROYED;
		}

		setSpritePos(m1.index, \
			TOINT(m1.x, FSH), TOINT(m1.y, FSH));
	}
	/* Missile destroyed, hide */
	else if (m1.state == DESTROYED)
	{
		m1.x = TOFIX(SCREEN_W, FSH);
		m1.y = TOFIX(SCREEN_H, FSH);
		setSpritePos(m1.index, SCREEN_W, SCREEN_H);
	}

	/* Car stuff */
	if (car.state == DRIVE)
	{
		/* move and check if moved over the left border */
		car.x = car.x - car.speed;
		if (TOINT(car.x, FSH) < -32)
		{
			/* Car goes over left border, player
			 * loses a life ? */
			gamestate = FAIL;
		}

		setSpritePos(car.index, TOINT(car.x, FSH),
			TOINT(car.y, FSH));
	}
	else if (car.state == EXPLODE)
	{
		/* Move car offscreen */
		if (e1.state == DESTROYED)
		{
			/* Advance level */
			gamestate = ADVANCE_LEVEL;
		}
	}
	else if (car.state == DESTROYED)
	{
		car.state = DRIVE;
	}

	/* Explosion stuff */	
	if (e1.state == EXPLODE)
	{
		e1.frame = 0;
		e1.counter = 0;
		e1.state = EXPLODING;

		setSpritePos(e1.index, 
			TOINT(m1.x, FSH) - (EXP_W / 2) + 6,
			TOINT(m1.y, FSH) - (EXP_H / 2) + 2);
	}
	if (e1.state == EXPLODING)
	{
		e1.counter++;
		if (e1.counter > EXPLOSION_DELAY)
		{
			/* Alternate frames */
			if (e1.frame < DATA_EXPLOSION_FRAMES)
				dmaCopy(&TileMem4[TB_OBJ][2], \
					(u32 *)data_explosion[e1.frame], 
					DATA_EXPLOSION_LEN);

			e1.frame++;
			if (e1.frame > DATA_EXPLOSION_FRAMES)
				e1.state = DESTROYED;
			e1.counter = 0;
		}
	}
	/* Explosion destroyed, copy first frame and move
	 * offscreen */
	else if (e1.state == DESTROYED)
	{
		e1.frame = 0;
		dmaCopy(&TileMem4[TB_OBJ][2], \
			(u32 *)data_explosion[e1.frame],
			DATA_EXPLOSION_LEN);

		setSpritePos(e1.index, SCREEN_W, SCREEN_H);
		e1.x = SCREEN_W;
		e1.y = SCREEN_H;
	}

	waitVsync();
#ifdef USE_SPRITE_BUF
	updateSprites(spriteCount);
#endif
}

void gameDelay(int n)
{
	int i;

	for (i=0; i<n; i++)
		waitVsync();
}

int main(void)
{
	int level = 1;

	p1.state = STAND;
	car.state = DRIVE;

	gamestate = INIT;
	frames = 0;

	while (1)
	{
		frames++;

		switch (gamestate)
		{
		case INIT:
			initGameData();
			initGameObjects();
			gamestate = INTRO;
			break;
		case INTRO:
			hideSprites();
			showIntro();
			gamestate = START;
			break;
		case START:
			level = 1;
			resetGameObjects();
			clearBgText(BG3);
			putBgText(BG3, 1, 1, 0, "LEVEL             SCORE:");
			putBgText(BG3, 1, 2, 0, "WEAPON: BAZOOKA   LIVES:");
			updateScore(p1.score, p1.lives, level);
			writeText(BG3, 8, 10, 6, "GET READY ...");	
			gameDelay(30);
			//clearBgTextRow(BG3, 10);
			gamestate = PLAY;
			break;
		case GAME_OVER:
			setSpritePos(p1.index, SCREEN_W, SCREEN_H);
			clearBgText(BG3);
			writeText(BG3, 10, 8, 15, "GAME OVER");
			gameDelay(60);
			putBgText(BG3, 4, 10, 0, "PRESS START TO RESTART");

			while (keyUp(KEY_START));
			gamestate = START;
			break;
		case ADVANCE_LEVEL:
			level++;	
			p1.score += SCORE_DESTROY_CAR;
			updateScore(p1.score, p1.lives, level);
			
			/* Destroy car */
			setSpritePos(car.index, 280, 
				TOINT(car.y, FSH));
			car.x = TOFIX(280, FSH);
			car.state = DESTROYED;

			/* Increase car speed */
			car.speed = car.speed + TOFIX(1, FSH - 2);

			writeText(BG3, 2, 10, 6, "GOOD JOB!");
			gameDelay(30);
			//clearBgTextRow(BG3, 10);

			gamestate = PLAY;
			break;
		case FAIL:
			writeText(BG3, 2, 10, 8, "YOU FAILED!");
			gameDelay(15);
			//clearBgTextRow(BG3, 10);
			p1.lives--;
			updateScore(p1.score, p1.lives, level);
			
			car.x = TOFIX(280, FSH);

			if (p1.lives > 0)
				gamestate = PLAY;
			else
				gamestate = GAME_OVER;
			break;
		default: 
			play();
			break;
		}
	}

	return 0;
}

