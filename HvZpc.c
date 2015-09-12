#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
//#include <C:\[The file path that holds glut.h]\glut.h>  //connects to glut.h 

//#define WAVEINCREMENT 5
#define HUMANSPEED 0.06
#define MELEEDELAY 400
#define MAXBULLETS 50
//#define FIREDELAY 360
#define BULLETSPEED 0.3
#define AMMOPACKSIZE 13

int waveIncrement = 0;
int fireDelay = 0;
int guntype = 0;
int rampageMode=0;
//gcc –o HvZ HvZ.c glut32.lib –lopengl32 –lglu32  <---compilation command in MinGW

//////////////////////////////////////////////////////////////////////////////////////////
int keyState[256]; //An array of "boolean values" to keep track of what keys are pressed//
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////
//LIST OF USEFUL KEYBOARD CODES //
//W:119     					//
//A:97							//	
//S:115							//
//D:100							//
//E:101							//
//////////////////////////////////

int wave = 0; 		//the current wave number


struct vector
{
	float x, y;
};
typedef struct vector vector;

//////////////////////////////////////////////////////////////////////////////
vector mouse;	  //always equals mouse location in pixels (divided by 100).// 
                    //corresponds exactly to openGL coordinates		        //
//////////////////////////////////////////////////////////////////////////////

vector shotFired; //a point where the last shot was fired

int gamemode=0;

struct zombie
{
	float size;
	float speed;
	int hearingRadius;
	int health;
	int class;
	int active; //Can be changed to health later
	vector position;
	vector direction; //Direction the zombie "wants" to go... kind of like an acceleration vector.
};
typedef struct zombie zombie;
zombie *walker;     //a pointer that can be used to reference zombies... feel free to get rid of the Walking Dead reference We could also have different categories of zombies, like a walker, or a "heavy" which would have more health
int zombiesActive;
int zombiesKilled;
int totalKilled=0;

void killZombie(int);
void hitZombie(int);

struct bullet
{
	int active;
	float x, y;
	float xDirection, yDirection;
	float unitx;
	float unity;
	float speed;
	float type;
	float originx;
	float originy;
};
typedef struct bullet bullet;
bullet* pbullet = NULL;	//pointer used to reference the bullets
int clip = 13;

	int meleeRequest;
	int fireRequest;
struct human
{
	int active;
	int ammo;
	vector position;
	vector direction;
	clock_t fireTime;
	clock_t meleeTime;
	int meleeActive;
};
typedef struct human human;
human *carl;

struct ammoPack
{
	int active;
	vector position;
};
typedef struct ammoPack pack;
pack *pAmmoPack;

void changeSize(int w, int h) 
{	//Changes the size of the screen. This is kind of irrelevant for us since we are in full screen
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if (h == 0)
		h = 1;

	float ratio =  w * 1.0 / h;

	// Use the Projection Matrix
	glMatrixMode(GL_PROJECTION);

	// Reset Matrix
	glLoadIdentity();

	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);

	// Set the correct perspective.
	//gluPerspective(45.0f, ratio, 0.1f, 100.0f);
	glOrtho(-9.6f, 9.6f, -5.4f, 5.4f, 0.0f, 10.0f); //This is what sets us in orthographic mode
	
	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}

vector unitVector(vector input)
{
	//Solves for the unit vector
	float mag;
	mag = sqrt(pow(input.x, 2) + pow(input.y, 2));
	input.x = (input.x/mag);
	input.y = (input.y/mag);
	return input;
}

vector scaleVector(int scalar, vector input)
{
	input.x*=scalar;
	input.y*=scalar;
	return input;
}

void processKeys()
{	//Processes WASD keys into the x and y coordinates
	if(keyState[119]) carl->position.y+=HUMANSPEED;
	if(carl->position.y>5.3) carl->position.y=5.3;
	if(keyState[97]) carl->position.x-=HUMANSPEED;
	if(carl->position.x<-9.6) carl->position.x=-9.6;
	if(keyState[115]) carl->position.y-=HUMANSPEED;
	if(carl->position.y<-5.3) carl->position.y=-5.3;
	if(keyState[100]) carl->position.x+=HUMANSPEED;
	if(carl->position.x>9.6) carl->position.x=9.6;
	/*if(keyState[112]) meleeRequest=1;
	else meleeRequest=0;
	if(keyState[111]) fireRequest=1;
	else fireRequest=0;*/
}

void createWave()
{	//Creates a wave of zombies whose size is dependent upon wave number. Also assigns initial location.
	wave++; //increment to the next wave
	walker = (zombie*)malloc(wave*waveIncrement*sizeof(zombie)); //allocates memory for all the zombies in the wave
	//Dwdsprintf("Wave %d created\n", wave);
	zombiesActive=wave*waveIncrement;
	
	clip+=(waveIncrement*(wave-1));
	pAmmoPack=(pack*)malloc(clip*sizeof(pack));
	pAmmoPack -> active = 0;
	
	int i;
	for(i=0;i<(wave*waveIncrement);i++) //Assigning initial locations, active flag, and direction
	{
		(walker+i)->active=1;
		(walker+i)->position.x=(((float)(rand()%1900)/100)-9.5);
		(walker+i)->position.y=((float)((rand()%200)+300)/100);
		(walker+i)->direction.x=0;
		(walker+i)->direction.y=0;
		int class = rand()%100;
		if(class<50) 
		{
			(walker+i)->class=1;
			(walker+i)->size=0.2;
			(walker+i)->speed=1;
			(walker+i)->hearingRadius=7;
			(walker+i)->health=1;
			
			
		}
		else if(class>=50&&class<75) //heavy
		{
			(walker+i)->class=2;
			(walker+i)->size=0.3;
			(walker+i)->speed=0.75;
			(walker+i)->hearingRadius=6;
			(walker+i)->health=5;
		}
		else //runner
		{
			(walker+i)->class=3;
			(walker+i)->size=0.15;
			(walker+i)->speed=1.5;
			(walker+i)->hearingRadius=10;
			(walker+i)->health=1;
		}
	}
	shotFired.x=0;
	shotFired.y=0;
	zombiesKilled=0;
}

void createBullet()
{	//Creates bullets
	pbullet = (bullet*)malloc(MAXBULLETS*sizeof(bullet)); //allocates memory for all the bullets
	//printf("Bullets Created\n");
	
	int i;
	for(i=0;i<(MAXBULLETS);i++) //Assigning initial locations, active flag, and direction
	{
		(pbullet+i)->active=0;
		(pbullet+i)->x=0;
		(pbullet+i)->y=0;
		(pbullet+i)->xDirection=0;
		(pbullet+i)->yDirection=0;
		(pbullet+i)->type=0;
		(pbullet+i)->originx = 0;
		(pbullet+i)->originy = 0;
	}
}

void createAmmoPack()
{
	float makeAmmo;
	//pAmmoPack=(pack*)malloc(clip*sizeof(pack)); //allocates memory for the ammo pack
	makeAmmo=(rand()%1000);
	if(makeAmmo<=3)
	{ //Send it to a different function that will keep redrawing the pack on the screen until picked up.
	//Once the pack is picked up, make active equal zero again. 
		pAmmoPack->active = 1;
		pAmmoPack->position.x=(((float)(rand()%(960))/100)-9.5);
		pAmmoPack->position.y=(((float)((rand()%560))/100)-5.4);
	}
}

void drawAmmoPack()
{
	glColor3f(1.0, 0.0, 1.0);
	glBegin(GL_QUADS);
		glVertex3f((pAmmoPack->position.x)-0.1, (pAmmoPack->position.y)-0.1, 4.0);
		glVertex3f((pAmmoPack->position.x)+0.1, (pAmmoPack->position.y)-0.1, 4.0);
		glVertex3f((pAmmoPack->position.x)+0.1, (pAmmoPack->position.y)+0.1, 4.0);
		glVertex3f((pAmmoPack->position.x)-0.1, (pAmmoPack->position.y)+0.1, 4.0);
	glEnd();
	//When the ammo is picked up makeAmmo=0 and (amount of ammo in pack is added to clip)
}

void drawbullet()	//calculates new position and draws new position 
{
	//printf("Bullets drawn");
	float speed = BULLETSPEED;
	int i;
	float length = 0.1f;
	for(i = 0; i<MAXBULLETS; i++)
	{
		if((pbullet + i)->active)
		{
			(pbullet + i)->x += speed*((pbullet+i)->unitx);
			(pbullet + i)->y += speed*((pbullet+i)->unity);
			glPushMatrix();
			//glTranslatef((pbullet + i)->x, (pbullet + i)->y, 2.00); // which z layer should I use? 
			glColor3f(1.0f, 1.0f, 0.0f);
			//glRectf(-0.025f, -0.1f, 0.0f, 0.0f);
			glLineWidth(3.0);
			glBegin(GL_LINES);
				glVertex3f((pbullet + i)->x, (pbullet + i)->y, 2.0f);
				glVertex3f((pbullet + i)->x+length*((pbullet+i)->unitx), (pbullet + i)->y+length*((pbullet+i)->unity), 2.0f);
			glEnd();
			glPopMatrix();
		}
		if (guntype ==0)
		{
			if(((pbullet + i)->x > 10.0f) || ((pbullet + i)->x < -10.0f) ||((pbullet + i)->y > 6.0f) || ((pbullet + i)->y < -6.0f))
			{
				(pbullet + i)->active = 0;
			}
		}
		else if (guntype == 1)
		{
			if ((sqrt(pow(((pbullet + i)->originx -(pbullet + i)->x),2)+pow(((pbullet+i)->originy -(pbullet + i)->y),2))>2.5) || ((pbullet + i)->x > 10.0f) || ((pbullet + i)->x < -10.0f) ||((pbullet + i)->y > 6.0f) || ((pbullet + i)->y < -6.0f))
			{
				(pbullet+i)->active = 0;
			}
		}
		if((pbullet+i)->active==0)
		{
			(pbullet+i)->x=0;
			(pbullet+i)->y=0;
			(pbullet+i)->xDirection=0;
			(pbullet+i)->yDirection=0;
			(pbullet+i)->type=0;
			(pbullet+i)->originx = 0;
			(pbullet+i)->originx = 0;
		}
		
	}	
	
	
}

void collision()
{
	int i;
	for(i=0; i<MAXBULLETS; i++)
	{
		if((pbullet+i)->active == 1)
		{
			int a;
			for(a=0; a<wave*waveIncrement; a++)
			{
				if((walker+a)->active == 1)
				{
					if (sqrt(pow(((walker+a)->position.x -(pbullet + i)->x),2)+pow(((walker+a)->position.y -(pbullet + i)->y),2))<(walker+a)->size+0.05) 
					{
						(walker+a)->health--;
						hitZombie(a);
						if(!(walker+a)->health) killZombie(a);
						(pbullet+i)->active = 0;
					}
				}
			}
		}
	}
}

void drawFilledCircle(float x, float y, float radius, int quality)
{	//This is just a function to draw a circle. I found it online @ https://gist.github.com/strife25/803118, edited it to work in C
	int i;
	int triangleAmount = quality; //# of triangles used to draw circle
	
	float twicePi = 2.0f * M_PI;
	
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(x, y); // center of circle
		for(i = 0; i <= triangleAmount;i++) 
		{ 
			glVertex2f(
		        x + (radius * cos(i *  twicePi / triangleAmount)), 
			    y + (radius * sin(i * twicePi / triangleAmount))
			);
		}
	glEnd();
}

void drawZombie(float x, float y, int i)
{	//draws one singular zombie. NOT drawZombies
	glPushMatrix();
	glTranslatef(x, y, 2.0f);
	//if((walker+i)->class==3) glColor3f(0.7f, 0.6f, 0.4f);
	//else if((walker+i)->class==2) glColor3f(0.6f, 0.75f, 0.3f);
	glColor3f(0.16f, 0.53f, 0.23f);
	drawFilledCircle(0.0f, 0.0f, (walker+i)->size, 15);
	glPopMatrix();
}

void hitZombie(int i)
{
	glPushMatrix();
	glTranslatef((walker+i)->position.x, (walker+i)->position.y, 2.1f);
	glColor3f(0.75f, 0.23f, 0.23f);
	drawFilledCircle(0.0f, 0.0f, (walker+i)->size+.05, 12);
	glPopMatrix();
}
void killZombie(int i)
{
	(walker+i)->active=0;
	zombiesActive--;
	zombiesKilled++;
	totalKilled++;
}

void drawZombies()
{
	int i;
	for(i=0;i<(wave*waveIncrement);i++)
	{
		if((walker+i)->active) drawZombie((walker+i)->position.x,(walker+i)->position.y, i);
	}
}

void calculateZombieMovement()
{	//generates direction vectors for each zombie
	int i;
	for(i=0;i<(wave*waveIncrement);i++)
	{
		
		if((walker+i)->active)
		{
			vector randomVector;
			randomVector.x=(((rand()%1000)-500));
			randomVector.y=(((rand()%1000)-500));
			randomVector=unitVector(randomVector);
			
			vector distance;
			distance.x=carl->position.x-(walker+i)->position.x;
			distance.y=carl->position.y-(walker+i)->position.y;
			if(sqrt(pow(distance.x,2)+pow(distance.y,2))<4||gamemode==2) //if see carl
			{
				if(sqrt(pow(distance.x,2)+pow(distance.y,2))<(0.5+(walker+i)->size)&&carl->meleeActive) //if hit by melee
				{
					(walker+i)->health--;
					hitZombie(i);
					if(!(walker+i)->health) killZombie(i);
					carl->meleeActive=0;
				}
				else if(sqrt(pow(distance.x,2)+pow(distance.y,2)<(walker+i)->size)) carl->active = 0;
				distance=unitVector(distance);
				distance=scaleVector(5, distance);
				(walker+i)->direction.x+=distance.x;
				(walker+i)->direction.y+=distance.y;
			}
			if((walker+i)->class==2&&(walker+i)->health<5)
			{
				distance=unitVector(distance);
				distance=scaleVector(5, distance);
				(walker+i)->direction.x+=distance.x;
				(walker+i)->direction.y+=distance.y;
			}
			else if(shotFired.x&&shotFired.y)//if a shot has been fired
			{
				distance.x=shotFired.x-(walker+i)->position.x;
				distance.y=shotFired.y-(walker+i)->position.y;
				if(sqrt(pow(distance.x,2)+pow(distance.y,2))<(walker+i)->hearingRadius)
				{
					distance=unitVector(distance);
					distance=scaleVector(5, distance);
					(walker+i)->direction.x+=distance.x;
					(walker+i)->direction.y+=distance.y;
				}
			}
			int j;
			int blocked=0;
			for(j=0;j<(wave*waveIncrement);j++) //collision avoidance with other zombies
			{
				if(j!=i)
				{
					distance.x=(walker+j)->position.x-(walker+i)->position.x;
					distance.y=(walker+j)->position.y-(walker+i)->position.y;
					if(sqrt(pow(distance.x,2)+pow(distance.y,2))<0.3)
					{
						blocked = 1;
						distance=unitVector(distance);
						distance=scaleVector(-1, distance);
						(walker+i)->direction.x+=distance.x;
						(walker+i)->direction.y+=distance.y;
					}
				}
			}
			if(!blocked)
			{
				(walker+i)->direction.x+=randomVector.x;
				(walker+i)->direction.y+=randomVector.y;
			}
			(walker+i)->direction=unitVector((walker+i)->direction);
			
			//printf("Walker's desired direction x: %.3f y: %.3f\n", (walker+i)->xDirection, (walker+i)->yDirection);
		}
	}
}

void moveZombies()
{
	int i;
	for(i=0;i<(wave*waveIncrement);i++)
	{
		if((walker+i)->active)
		{
			(walker+i)->position.x+=(((float)(walker+i)->speed*(walker+i)->direction.x)/17);//(((float)(rand()%1000)-500)/100000);
			(walker+i)->position.y+=(((float)(walker+i)->speed*(walker+i)->direction.y)/17);//(((float)(rand()%1000)-500)/100000);
			if((walker+i)->position.y>5.3) (walker+i)->position.y=5.3;
			else if((walker+i)->position.y<-5.3) (walker+i)->position.y=-5.3;
			if((walker+i)->position.x>9.5) (walker+i)->position.x=9.5;
			else if((walker+i)->position.x<-9.5) (walker+i)->position.x=-9.5;
			//printf("Walker location: x: %.3f y: %.3f\n", (walker+i)->x, (walker+i)->y);
		}
	}
}

void createHuman()
{
	carl=(human*)malloc(sizeof(human));
	carl->position.x=0;
	carl->position.y=-3;
	carl->direction.x=0;
	carl->direction.y=1;
	carl->active=1;
	carl->meleeTime=carl->fireTime=0;
	carl->meleeActive=0;
	//add ammo stuff here;
}

void drawHuman() {  //Draw the location and direction of the Human
	glPushMatrix();
	glColor3f(0.0f, 0.0f, 1.0f); //Color is Blue
	glTranslatef(carl->position.x, carl->position.y, 1.0f);//Takes key input from Processed Keys
	drawFilledCircle(0.0f, 0.0f, 0.2f, 12); //Draws a circle where the human is
	glColor3f(1.0f, 0.0f, 0.0f); //Color is Red
		glBegin(GL_LINES); //Draws the line of sight for the player
			glVertex3f(0.0f, 0.0f, 0.1f); //Draws the point in the center of the circle
			glVertex3f(0.2*mouse.x, 0.2*mouse.y, 0.1f); //Draws the point on the edge of the circle
		glEnd(); 
	glPopMatrix();
}

void humanAttack()
{
	if(meleeRequest&&(clock()-carl->meleeTime)>(MELEEDELAY*(CLOCKS_PER_SEC/1000)))
	{
		carl->meleeActive=1;
		carl->meleeTime=clock();
	}
	
	if(((clock()-carl->meleeTime)<(.2*CLOCKS_PER_SEC))&&(carl->meleeActive))
	{
		glPushMatrix();
		glColor3f(1.0f,0.0f,0.0f);
		glTranslatef(carl->position.x, carl->position.y, 0.5f);
		drawFilledCircle(0.0f, 0.0f, 0.5f, 24);
		glPopMatrix();
	}
	else carl->meleeActive=0;
	
	if(clip>0&&fireRequest&&(clock()-carl->fireTime)>(fireDelay*(CLOCKS_PER_SEC/1000)))
	{
		
		carl->fireTime=clock();
		
		
		if (guntype==0)
		{
			if(!rampageMode) clip--;
			int i;
			for(i=0;i<MAXBULLETS;i++)
			{
				if(!(pbullet+i)->active)
				{
					(pbullet + i)->active = 1;
					(pbullet + i)->unitx = mouse.x;
					(pbullet + i)->unity = mouse.y;
					(pbullet + i)->x = carl->position.x;
					(pbullet + i)->y = carl->position.y;
					shotFired.x=carl->position.x;
					shotFired.y=carl->position.y;
					(pbullet + i)->type = 0;
					break;
				}
			}
		}
		else if (guntype == 1)//shotgun///////////if we get time, we can use less code by making one function that accepts an offset angle to replace the repetitiveness here
		{
			carl->fireTime=clock();
			//dead on
			if(!rampageMode) clip-=3;
			if(clip<0) clip=0;
			int i;
			for(i=0;i<MAXBULLETS;i++)////////////Straight bullet
			{
				if(!(pbullet+i)->active)
				{
					(pbullet + i)->active = 1;
					(pbullet + i)->unitx = mouse.x;
					(pbullet + i)->unity = mouse.y;
					(pbullet + i)->x = carl->position.x;
					(pbullet + i)->y = carl->position.y;
					(pbullet + i)->type = 1;
					(pbullet + i)->originx = carl->position.x;
					(pbullet + i)->originy = carl->position.y;;
					shotFired.x=carl->position.x;
					shotFired.y=carl->position.y;
					(pbullet + i)->type = 0;
					break;
				}
			}
			for(i=0;i<MAXBULLETS;i++)	//other side bullet
			{
				if(!(pbullet+i)->active)	//one of the side bullets
				{
					(pbullet + i)->active = 1;
					(pbullet + i)->unitx = cos(75*M_PI/180-atan2(mouse.x, mouse.y));
					(pbullet + i)->unity = sin(75*M_PI/180-atan2(mouse.x, mouse.y));
					(pbullet + i)->x = carl->position.x;
					(pbullet + i)->y = carl->position.y;
					(pbullet + i)->type = 1;
					(pbullet + i)->originx = carl->position.x;
					(pbullet + i)->originy = carl->position.y;;
					shotFired.x=carl->position.x;
					shotFired.y=carl->position.y;
					break;
				}
			}
			for(i=0;i<MAXBULLETS;i++)	//other side bullet
			{
				if(!(pbullet+i)->active)
				{
					(pbullet + i)->active = 1;
					(pbullet + i)->unitx = cos(105*M_PI/180-atan2(mouse.x, mouse.y));
					(pbullet + i)->unity = sin(105*M_PI/180-atan2(mouse.x, mouse.y));
					(pbullet + i)->x = carl->position.x;
					(pbullet + i)->y = carl->position.y;
					(pbullet + i)->type = 1;
					(pbullet + i)->originx = carl->position.x;
					(pbullet + i)->originy = carl->position.y;;
					shotFired.x=carl->position.x;
					shotFired.y=carl->position.y;
					break;
				}
			}
		}
	}
	if((clock()-carl->fireTime)>5*CLOCKS_PER_SEC)
	{
		shotFired.x=0;
		shotFired.y=0;
	}
}

void detectAmmoPacks()
{
	if(pAmmoPack->active)
	{
		vector distance;
		distance.x=carl->position.x-pAmmoPack->position.x;
		distance.y=carl->position.y-pAmmoPack->position.y;
		if(sqrt(pow(distance.x,2)+pow(distance.y,2))<0.3)
		{
			clip+=AMMOPACKSIZE;
			pAmmoPack->active=0;
		}
	}
}

char* fitoa(int i, char b[33])
{//http://stackoverflow.com/questions/9655202/how-to-convert-integer-to-string-in-c
    char const digit[] = "0123456789";
	//char b[33];
    char* p = b;
    int shifter = i;
    if(i<0)
	{
        *p++ = '-';
        i *= -1;
    }
    do
	{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    } while(shifter);
    *p = '\0';
    do
	{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    } while(i);
    return b;
}

void drawStats(void *font, char *string, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *charPtr;
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text	
	for (charPtr=string; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}
void drawZAlive(void *font, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *cZombiesActive;
	char b[4];
	cZombiesActive = fitoa(zombiesActive, b);
	char *charPtr;
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text
			
	for (charPtr=cZombiesActive; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}
void drawZKilled(void *font, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *cZombiesKilled;
	char b[4];
	char *charPtr;
	cZombiesKilled = fitoa(zombiesKilled, b);
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text
	for (charPtr=cZombiesKilled; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}
void drawTotalKilled(void *font, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *cTotalKilled;
	char b[4];
	char *charPtr;
	cTotalKilled = fitoa(totalKilled, b);
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text	
	for (charPtr=cTotalKilled; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}
void drawClipCount(void *font, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *cClipCount;
	char b[4];
	cClipCount = fitoa(clip, b);
	char *charPtr;
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text
			
	for (charPtr=cClipCount; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}

void drawGameOver(void *font, char *string, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *charPtr;
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text	
	for (charPtr=string; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}

void drawLevel(void *font, float red, float green, float blue, float xstart, float ystart, float zstart)
{
	char *cLevel;
	char b[4];
	char *charPtr;
	cLevel = fitoa(wave, b);
	glColor3f(red,green,blue);
	glRasterPos3f(xstart,ystart,zstart); //Position your text	
	for (charPtr=cLevel; *charPtr; charPtr++) 
	{
		glutBitmapCharacter(font, *charPtr);
	}
}

void drawBox()
{
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
		glVertex3f(5.0f, 3.8f, 5.0f);
		glVertex3f(9.5f, 3.8f, 5.0f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(9.5f, 5.3f, 5.0f);
		glVertex3f(5.0f, 5.3f, 5.0f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(5.0f, 5.3f, 5.0f);
		glVertex3f(5.0f, 3.8f, 5.0f);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(9.5f, 5.3f, 5.0f);
		glVertex3f(9.5f, 3.8f, 5.0f);
	glEnd();
	glPopMatrix();
}

void renderScene(void) 
{
	float framerate = 33*(CLOCKS_PER_SEC/1000); //framerate in milliseconds * ticks in a second
	clock_t now, then;
	now=then=clock();
	if(keyState[102]) goto PAUSE;
	// Clear Color and Depth Buffers
	glClearColor(0, 0, 0, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Reset transformations
	glLoadIdentity();
	// Set the camera to look straight down, X is horizontal, Y is vertical. 10 "layers"
	gluLookAt(0.0f, 0.0f, 10.0f, 
			0.0f, 0.0f, 0.0f,
			0.0f, 1.0f,  0.0f);
	
	if(gamemode==0)
	{
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Press A to play normal mode", 1, 0, 0, -1.0, 0, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Press H to play hardcore mode", 1, 0, 0, -1.0, -0.2, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "", 1, 0, 0, -1.0, -0.4, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "---Play at own risk!---", 1, 0, 0, -1.0, -0.6, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "How to Play", 1, 0, 0, -9.5, 5.2, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Welcome survivor. If we are going to last the night we need to make quick work of these undead.", 1, 0, 0, -9.5, 5.0, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Here's a clip of ammo to get you started, you can find more as you go.", 1, 0, 0, -9.5, 4.8, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Here are the controls you need to know", 1, 0, 0, -9.5, 4.6, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "To Move: WASD", 1, 0, 0, -9.5, 4.4, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "To shoot: Left Mouse Button", 1, 0, 0, -9.5, 4.2, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "To aim: Mouse", 1, 0, 0, -9.5, 4.0, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "To use melee: Right Mouse Button", 1, 0, 0, -9.5, 3.8, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "To switch weapons: Q", 1, 0, 0, -9.5, 3.6, 5.0);
			
		if(keyState[97])
		{
			rampageMode=0;
			gamemode=1;
			fireDelay = 360;
			waveIncrement = 5;
		}
		if(keyState[104])
		{
			rampageMode=1;
			gamemode=1;
			fireDelay=100;
			wave=5;
			waveIncrement=15;
		}
	}
	else if(gamemode==1)
	{
		if(carl==NULL) createHuman();
		processKeys(); //Find human position. Right now would implicitly indicate human speed is dependent upon frame rate.
		humanAttack();
		
		if(walker==NULL) createWave();
		calculateZombieMovement();
		moveZombies();
		drawZombies();
		
		
		
		if(pbullet==NULL) createBullet();
		drawbullet();
		
		collision();
		
		detectAmmoPacks();
		drawHuman();
		
		if(!zombiesActive) 
		{
			free(walker);
			walker=NULL;
		}
		
		drawBox();
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Zombie Statistics", 1, 0, 0, 5.1, 5.1, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Zombies Alive: ", 1, 0, 0, 5.1, 4.8, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Zombies Killed:", 1, 0, 0, 5.1, 4.5, 5.0);
		drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Total Zombies Killed:", 1, 0, 0, 5.1, 4.2, 5.0);
		if(!rampageMode) drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Ammo:", 1, 0, 0, 5.1, 3.9, 5.0);
		else drawStats(GLUT_BITMAP_TIMES_ROMAN_24, "Ammo: UNLIMITED", 1, 0, 0, 5.1, 3.9, 5.0);
		drawZAlive(GLUT_BITMAP_TIMES_ROMAN_24, 1, 0, 0, 8.0, 4.8, 5.0);
		drawZKilled(GLUT_BITMAP_TIMES_ROMAN_24, 1, 0, 0, 8.0, 4.5, 5.0);
		drawTotalKilled(GLUT_BITMAP_TIMES_ROMAN_24, 1, 0, 0, 8.0, 4.2, 5.0);
		if(!rampageMode) drawClipCount(GLUT_BITMAP_TIMES_ROMAN_24, 1, 0, 0, 8.0, 3.9, 5.0);
		
		if(carl->active==0)
		{
			gamemode=2;
		}
		if(!rampageMode)
		{
			if(pAmmoPack->active==0) createAmmoPack();
			else drawAmmoPack();
		}
	}
	else if (gamemode==2)
	{
		drawGameOver(GLUT_BITMAP_TIMES_ROMAN_24, "GAME OVER", 1, 0, 0, -1.0, 0, 5.0);
		drawGameOver(GLUT_BITMAP_TIMES_ROMAN_24, "Level: ", 1, 0, 0, -0.7, -0.4, 5.0);
		drawGameOver(GLUT_BITMAP_TIMES_ROMAN_24, "Total Zombies Killed:", 1, 0, 0, -1.6, -0.8, 5.0);
		drawGameOver(GLUT_BITMAP_TIMES_ROMAN_24, "Press Q to return to Main Menu", 1, 0, 0, -1.8, -1.1, 5.0);
		drawLevel(GLUT_BITMAP_TIMES_ROMAN_24, 1, 0, 0, -0.1, -0.4, 5.0);
		drawTotalKilled(GLUT_BITMAP_TIMES_ROMAN_24, 1, 0, 0, 0.7, -0.8, 5.0);
				
		calculateZombieMovement();
		moveZombies();
		drawZombies();
		drawHuman();
		
		
		if(keyState[113])///key spacebar/A on controller
		{
			gamemode=0;
			clip=13;
			totalKilled=0;
			free(walker);
			free(carl);
			free(pbullet);
			pbullet=NULL;
			walker=NULL;
			carl=NULL;
			wave=0;
		}
	}
	
	//glutWarpPointer(1920/2, 1080/2); //resets the mouse to the middle of the screen. This gets us our rotational vector behaviour. Evidently Notepad++ auto correct is British
	glutSwapBuffers();
	PAUSE:do{
		now=clock();
	}while( (now-then) < framerate);
	
} 

void processNormalKeys(unsigned char key, int xx, int yy) 
{ 	//This function is called every time a standard key is pressed
	if(key == 27)
	{
		free(walker);
		exit(0); //Quits the program if the user clicks escape
	}
	keyState[key] = 1; //records a key as pressed
	if(keyState[113]) 
	{
		if(!guntype) guntype=1;
		else guntype=0;
	}
} 

void clearNormalKeys(unsigned char key, int xx, int yy)
{	//Called every time a key is let go.
	keyState[key] = 0; //records a key as not pressed
}

void pressKey(int key, int xx, int yy) 
{	//This function is for special keys like arrow keys. not used right now, was for camera movement
	/*
     switch (key) {
         case GLUT_KEY_UP	: deltaMove = 0.5f; break;
         case GLUT_KEY_DOWN : deltaMove = -0.5f; break;
		 case GLUT_KEY_LEFT : deltaAngle= -0.005f; break;
		 case GLUT_KEY_RIGHT : deltaAngle= 0.005f; break;
      }
	  */
} 

void releaseKey(int key, int x, int y) 
{	//Called when special key released 	
/*
        switch (key) {
             case GLUT_KEY_UP	:
             case GLUT_KEY_DOWN : deltaMove = 0;break;
			 case GLUT_KEY_LEFT : 
             case GLUT_KEY_RIGHT : deltaAngle = 0; break;

        }
		*/
} 

void mouseUpdate(int x, int y)
{	//Updates mouse location to mouse.x and mouse.y
	mouse.x = (float)(x-960)/100.0;
	mouse.y = -(float)(y-540)/100.0;
	mouse=unitVector(mouse);
}

void mousePressedUpdate(int button, int state, int x, int y)
{
	mouse.x = (float)(x-960)/100.0;
	mouse.y = -(float)(y-540)/100.0;
	mouse=unitVector(mouse);
	if(gamemode)
	{
		switch(button)
		{
			case GLUT_LEFT_BUTTON:
				if(state==GLUT_DOWN)
				{
					fireRequest=1;
				}
				else fireRequest=0;
				break;
			case GLUT_RIGHT_BUTTON:
				if(state==GLUT_DOWN)
				{
					meleeRequest=1;
				}	
				else meleeRequest=0;
				break;
		}
	}
}

int main(int argc, char **argv) {
	srand(time(NULL));
	// init GLUT and create window
	glutInit(&argc, argv);
	glutInitWindowPosition(0,0);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(300, 300);
	glutCreateWindow("Humans Vs. Zombies");
	glutSetCursor(GLUT_CURSOR_NONE);
	glutFullScreen();

	// register callbacks
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);

	glutKeyboardFunc(processNormalKeys);
	glutKeyboardUpFunc(clearNormalKeys);
	glutSpecialFunc(pressKey);
	glutSpecialUpFunc(releaseKey);

	// OpenGL init
	glEnable(GL_DEPTH_TEST);
	glutWarpPointer(1920/2, 1080/2); //initially puts the mouse in the origin
	glutPassiveMotionFunc(mouseUpdate);
	glutMotionFunc(mouseUpdate);
	glutMouseFunc(mousePressedUpdate);
	// enter GLUT event processing cycle
	glutMainLoop();
	
	return 1;
}
