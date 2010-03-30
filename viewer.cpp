#include "viewer.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <GL/gl.h>
#include <GL/glut.h>
#include <assert.h>
#include "appwindow.hpp"

#define NUM_TEXTURES	9
#define DEFAULT_GAME_SPEED 50
#define WIDTH	16
#define HEIGHT 	10
using namespace std;

Viewer::Viewer()
{
	
	// Set all rotationAngles to 0
	rotationAngleX = 0;
	rotationAngleY = 0;
	rotationAngleZ = 0;
	rotationSpeed = 0;
	
	// Default draw mode is multicoloured
	currentDrawMode = Viewer::FACE;

	// Default scale factor is 1
	scaleFactor = 1;
	
	// Assume no buttons are held down at start
	shiftIsDown = false;
	mouseB1Down = false;
	mouseB2Down = false;
	mouseB3Down = false;
	rotateAboutX = false;
	rotateAboutY = false;
	rotateAboutZ = false;
	clickedButton = false;
	// 
	loadScreen = true;
	moveLightSource = false;
	activeTextureId = 0;
	loadTexture = true;
	loadBumpMapping = false;
	transluceny = false;
	moveLeft = false;
	moveRight = false;
	motionBlur = false;
	levelUpAnimation = false;
	singleSkinMode = false;
	disableSound = false;
	drawShadow = false;
	// Game starts at a slow pace of 500ms
	gameSpeed = DEFAULT_GAME_SPEED;
	
	// By default turn double buffer on
	doubleBuffer = true;
	
	gameOver = false;
	numTextures = 0;
	lightPos[0] = 4.6f;
	lightPos[1] = 6.79998f;
	lightPos[2] = 62.6f;
	lightPos[3] = 1.0f;

	
	Glib::RefPtr<Gdk::GL::Config> glconfig;
	
	// Ask for an OpenGL Setup with
	//  - red, green and blue component colour
	//  - a depth buffer to avoid things overlapping wrongly
	//  - double-buffered rendering to avoid tearing/flickering
	//	- Multisample rendering to smooth out edges
	glconfig = Gdk::GL::Config::create(	Gdk::GL::MODE_RGBA |
										Gdk::GL::MODE_DEPTH |
										Gdk::GL::MODE_DOUBLE |
										Gdk::GL::MODE_STENCIL);
	if (glconfig == 0) {
	  // If we can't get this configuration, die
	  abort();
	}

	// Accept the configuration
	set_gl_capability(glconfig);

	// Register the fact that we want to receive these events
	add_events(	Gdk::BUTTON1_MOTION_MASK	|
				Gdk::BUTTON2_MOTION_MASK    |
				Gdk::BUTTON3_MOTION_MASK    |
				Gdk::BUTTON_PRESS_MASK      | 
				Gdk::BUTTON_RELEASE_MASK    |
				Gdk::KEY_PRESS_MASK 		|
				Gdk::VISIBILITY_NOTIFY_MASK);
		
	// Create Game
	game = new Game(WIDTH, HEIGHT);
	game->setViewer(this);
	// Start game tick timer
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
//	clearBarTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::moveClearBar), 50);
}

Viewer::~Viewer()
{
	delete(game);
  // Nothing to do here right now.
}

void Viewer::invalidate()
{
  //Force a rerender
  Gtk::Allocation allocation = get_allocation();
  get_window()->invalidate_rect( allocation, false);
  
}

void Viewer::on_realize()
{
	// Do some OpenGL setup.
	// First, let the base class do whatever it needs to
	Gtk::GL::DrawingArea::on_realize();
	
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	
	if (!gldrawable)
		return;
	
	if (!gldrawable->gl_begin(get_gl_context()))
		return;
	
	texture = new GLuint[7];
	LoadGLTextures("playButton.bmp", playButtonTex);
	LoadGLTextures("playButtonClicked.bmp", playButtonClickedTex);
	LoadGLTextures("soundOn.bmp", soundOnTex);
	LoadGLTextures("soundOff.bmp", soundOffTex);
	LoadGLTextures("singleSkinMode.bmp", singleSkinModeTex);
	LoadGLTextures("singleSkinModeClicked.bmp", singleSkinModeClickedTex);
	LoadGLTextures("x1.bmp", texture[0]);
	LoadGLTextures("o1.bmp", texture[1]);
	LoadGLTextures("xLight1.bmp", texture[2]);
	LoadGLTextures("oLight1.bmp", texture[3]);
	LoadGLTextures("black.bmp", texture[4]);
	LoadGLTextures("normal.bmp", bumpMap);
	LoadGLTextures("floor.bmp", floorTexId);

	LoadGLTextures("background.bmp", backgroundTex);
	GenNormalizationCubeMap(256, cube);
	
	
	// Load music
	introMusic = sm.LoadSound("intro.ogg");
	backgroundMusic = sm.LoadSound("lumines.ogg");
	moveSound = sm.LoadSound("move.ogg");
	turnSound = sm.LoadSound("turn.ogg");
	sm.PlaySound(introMusic, -1);
	
	
	// Sphere for particles
	particleSphere = gluNewQuadric();
	sphereDisplayList = glGenLists(4);
	glNewList(sphereDisplayList, GL_COMPILE);
		GLUquadricObj *sphere = gluNewQuadric();
		gluQuadricNormals(sphere, GL_SMOOTH);	// Generate Smooth Normals For The Quad
		gluQuadricTexture(sphere, GL_TRUE);		// Enable Texture Coords For The Quad
		gluSphere(sphere, 1.0f, 32, 32);
		delete(sphere);
	glEndList();
	
	// Cubes for the game board
	texCubeDisplayList = sphereDisplayList + 1;
	glNewList(texCubeDisplayList, GL_COMPILE);
		glBegin(GL_QUADS);
		 	glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 1);

			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(1, 0, 1);

			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(1, 1, 1);

			glTexCoord2f(0.0f, 1.0f);	
			glVertex3d(0, 1, 1);
		glEnd();
		// top face
		glNormal3d(0, 1, 0);

		glBegin(GL_QUADS);
		 	glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 1, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(1, 1, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(1, 1, 1);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, 1, 1);
		glEnd();

		// left face
		glNormal3d(0, 0, -1);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(0, 1, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(0, 1, 1);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, 0, 1);
		glEnd();

		// bottom face
		glNormal3d(0, -1, 0);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(1, 0, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(1, 0, 1);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, 0, 1);
		glEnd();

		// right face
		glNormal3d(0, 0, 1);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(1, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(1, 1, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(1, 1, 1);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(1, 0, 1);
		glEnd();

		// Back of front face
		glNormal3d(-1, 0, 0);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(1, 0, 0);
			glTexCoord2f(1.0f, 1.0f);	
			glVertex3d(1, 1, 0);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, 1, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	glEndList();
	
	// Create display list for outline
	outlineDisplayList = texCubeDisplayList + 1;
	glNewList(outlineDisplayList, GL_COMPILE);
		glLineWidth (1.2);
		glBegin(GL_LINE_LOOP);
			glVertex3d(0, 0, 1);

			glVertex3d(1, 0, 1);

			glVertex3d(1, 1, 1);

			glVertex3d(0, 1, 1);
		glEnd();
		// top face
		glNormal3d(0, 1, 0);

		glBegin(GL_LINE_LOOP);
			glVertex3d(0, 1, 0);
			glVertex3d(1, 1, 0);
			glVertex3d(1, 1, 1);
			glVertex3d(0, 1, 1);
		glEnd();

		// left face
		glNormal3d(0, 0, -1);

		glBegin(GL_LINE_LOOP);
			glVertex3d(0, 0, 0);
			glVertex3d(0, 1, 0);
			glVertex3d(0, 1, 1);
			glVertex3d(0, 0, 1);
		glEnd();

		// bottom face
		glNormal3d(0, -1, 0);

		glBegin(GL_LINE_LOOP);
			glVertex3d(0, 0, 0);
			glVertex3d(1, 0, 0);
			glVertex3d(1, 0, 1);
			glVertex3d(0, 0, 1);
		glEnd();

		// right face
		glNormal3d(0, 0, 1);

		glBegin(GL_LINE_LOOP);
			glVertex3d(1, 0, 0);
			glVertex3d(1, 1, 0);
			glVertex3d(1, 1, 1);
			glVertex3d(1, 0, 1);
		glEnd();

		// Back of front face
		glNormal3d(-1, 0, 0);

		glBegin(GL_LINE_LOOP);
			glVertex3d(0, 0, 0);
			glVertex3d(1, 0, 0);
			glVertex3d(1, 1, 0);
			glVertex3d(0, 1, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	glEndList();
	
	// Reflection display list
	reflectCubeDisplayList = outlineDisplayList + 1;
	glNewList(reflectCubeDisplayList, GL_COMPILE);
		glBegin(GL_QUADS);
		 	glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 1);

			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(1, 0, 1);

			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(1, 1, 1);

			glTexCoord2f(0.0f, 1.0f);	
			glVertex3d(0, 1, 1);
		glEnd();
	glEndList();
		
	// Load default aniamtion
	readFile("head.txt");
		
	
	glClearDepth (1.0f);								
	glDepthFunc (GL_LEQUAL);							
	glEnable (GL_DEPTH_TEST);							
	glShadeModel (GL_SMOOTH);							
	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	
	
	

/*	// Basic line anti aliasing
	// Blendfunc is set wrong though. Need to figure out what is good
	glEnable (GL_LINE_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_COLOR, GL_DST_COLOR);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
*/
	glClearColor(1.0, 1.0, 1.0, 1.0);
	
	// Initialize random number generator
 	srand ( time(NULL) );

	gldrawable->gl_end();
}

bool Viewer::on_expose_event(GdkEventExpose* event)
{
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	
	if (!gldrawable) return false;

	if (!gldrawable->gl_begin(get_gl_context()))
		return false;
		
	// Decide which buffer to write to
	if (doubleBuffer)
		glDrawBuffer(GL_BACK);	
	else
		glDrawBuffer(GL_FRONT);
		
			
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Modify the current projection matrix so that we move the 
	// camera away from the origin.  We'll draw the game at the
	// origin, and we need to back up to see it.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glTranslated(-3.0, 5.0, -30.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// set up lighting (if necessary)
	// Followed the tutorial found http://www.falloutsoftware.com/tutorials/gl/gl8.htm
	// to implement lighting
	
	// Initialize lighting settings
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	
	// Scale and rotate the scene
	
	if (scaleFactor != 1)
		glScaled(scaleFactor, scaleFactor, scaleFactor);
		
	if (rotationAngleX != 0)
		glRotated(rotationAngleX, 1, 0, 0);
	
	if (rotationAngleY != 0)
		glRotated(rotationAngleY, 0, 1, 0);

	if (rotationAngleZ != 0)
		glTranslatef(rotationAngleZ, 0, 0);
	
	// Increment rotation angles for next render
	if ((mouseB1Down && !shiftIsDown) || rotateAboutX)
	{
		rotationAngleX += rotationSpeed;
		if (rotationAngleX > 360)
			rotationAngleX -= 360;
	}
	if ((mouseB2Down && !shiftIsDown) || rotateAboutY)
	{
		rotationAngleY += rotationSpeed;
		if (rotationAngleY > 360)
			rotationAngleY -= 360;
	}
	if ((mouseB3Down && !shiftIsDown) || rotateAboutZ)
	{
		rotationAngleZ += rotationSpeed;
		if (rotationAngleZ > 360)
			rotationAngleZ -= 360;
	}
	
	// You'll be drawing unit cubes, so the game will have width
	// 10 and height 24 (game = 20, stripe = 4).  Let's translate
	// the game so that we can draw it starting at (0,0) but have
	// it appear centered in the window.
	glTranslated(-7.5, -10.0, 7.0);
	
	if (loadScreen)
	{
		drawBackground();
		drawFloor();
		drawStartScreen(false);
		
		// We pushed a matrix onto the PROJECTION stack earlier, we 
		// need to pop it.

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		// Swap the contents of the front and back buffers so we see what we
		// just drew. This should only be done if double buffering is enabled.
		if (doubleBuffer)
			gldrawable->swap_buffers();
			
		gldrawable->gl_end();

		return true;
	}
	
	// Create one light source
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	//glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Define properties of light 
	float ambientLight0[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	float diffuseLight0[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specularLight0[] = { 0.6f, 0.6f, 0.6f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	
	if (!motionBlur)
	{	
		drawScene();
	}
	else
	{
		if (game->counter == 0)
		{
			glClear(GL_ACCUM_BUFFER_BIT);
			drawFallingBox();
			glAccum(GL_RETURN, 1.f);
		}
		else
		{
			drawScene();
		}
	}

	glColor3f(1.f, 1.f, 0.f);
	GLUquadricObj *quadratic;	
	quadratic=gluNewQuadric();			// Create A Pointer To The Quadric Object ( NEW )
	gluQuadricNormals(quadratic, GLU_SMOOTH);	// Create Smooth Normals ( NEW )
	gluQuadricTexture(quadratic, GL_TRUE);
	glPushMatrix();
		glTranslatef(lightPos[0], lightPos[1], lightPos[2]);
		gluSphere(quadratic,1.3f,32,32);
	glPopMatrix();
	
 	// We pushed a matrix onto the PROJECTION stack earlier, we 
	// need to pop it.

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Swap the contents of the front and back buffers so we see what we
	// just drew. This should only be done if double buffering is enabled.
	if (doubleBuffer)
		gldrawable->swap_buffers();

	gldrawable->gl_end();

	return true;
}

void Viewer::drawScene()
{
	if (drawShadow)
	{
		glDisable(GL_DEPTH_TEST);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1, 0xffffffff);

		drawFloor();

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_DEPTH_TEST);

		glStencilFunc(GL_EQUAL, 1, 0xffffffff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		drawShadowVolumes();
		silhouette.clear();
		drawFloor();	

		glDisable(GL_STENCIL_TEST);	
	}
	
	drawBackground();
	drawFloor();
	drawReflections();	
	drawGameboard();
	drawGrid();
	drawParticles();	
	drawBar();
	glDisable(GL_LIGHTING);
	drawAnimatables();
	glEnable(GL_LIGHTING);
	
	if (levelUpAnimation)
	{	
		levelUpAnimation = false;
		addFireworks(8 + rand()%4 - 2, 5 + rand()%4 - 2);
		addFireworks(3 + rand()%4 - 2, 3 + rand()%4 - 2);
		addFireworks(3 + rand()%4 - 2, 8 + rand()%4 - 2);
		addFireworks(8 + rand()%4 - 2, 2 + rand()%4 - 2);
		addFireworks(14 + rand()%4 - 2, 6 + rand()%4 - 2);
		addFireworks(16 + rand()%4 - 2, 8 + rand()%4 - 2);
	}
}

void Viewer::drawAnimatables()
{
	glPushMatrix();
		glTranslatef(16, 3, 0);
		glScalef(0.5, 0.5, 0.5);
		Point3D frame;
		float *scale;
		float *rotate;
		float *col;
		for (unsigned int i = 0;i<animatables.size();i++)
		{
			frame = animatables[i].frames[0];
			scale = animatables[i].scales[0];
			rotate = animatables[i].rotates[0];
			col = animatables[i].col;
			glPushMatrix();			
				glColor3f(col[0], col[1], col[2]);
				glRotatef(rotate[0], 1, 0, 0);
				glRotatef(rotate[1], 0, 1, 0);
				glRotatef(rotate[2], 0, 0, 1);
				glTranslatef(frame[0], frame[1], frame[2]);
				glScalef(scale[0], scale[1], scale[2]);
			
				// Draw something
				if (animatables[i].shapeType == 1)
				{
					glCallList(sphereDisplayList);
				}
				else
				{
					glBegin(GL_QUADS);
						glVertex3f(0, 0, 0);
						glVertex3f(1, 0, 0);
						glVertex3f(1, 1, 0);
						glVertex3f(0, 1, 0);
					glEnd();
				}

			
			
			glPopMatrix();
	
			animatables[i].frames.erase(animatables[i].frames.begin());
			animatables[i].scales.erase(animatables[i].scales.begin());
			animatables[i].rotates.erase(animatables[i].rotates.begin());
			if (animatables[i].loop)
			{
				animatables[i].frames.push_back(frame);
				animatables[i].scales.push_back(scale);
				animatables[i].rotates.push_back(rotate);
			}
				
			if (animatables[i].frames.size() == 0)
			{
				//animatables.erase(animatables.begin() + i);
				animatables.clear();
				readFile("head.txt");
			}
			
		}
	glPopMatrix();
}
void Viewer::drawBackground()
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backgroundTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-9, 0, -10);
		
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(-9, 17, -10);
		
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(29, 17, -10);
		
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(29, 0, -10);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}
/*
void Viewer::drawMoveBlur(int side)
{
	int r = game->py_;
	int c = game->px_;
	std::cout << r << ", " << c << std::endl;
	
		std::cout << game->get(r-1, c) << "\n";
		std::cout << game->get(r-side, c+side) << "\n";
		std::cout << game->get(r-side, c-side) << "\n";
			
//	c = c - side;
		
	glEnable(GL_BLEND);
	
	for (int i = 0;i<10;i++)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		drawCube(r-side, c + side * (i * 0.1), game->get(r-side, c+side), GL_QUADS);
		drawCube(r-side-1, c + side * (i * 0.1), game->get(r-side, c+side), GL_QUADS);
	}
	glDisable(GL_BLEND);
}
*/
void Viewer::drawReflections()
{
	/* Don't update color or depth. */
	drawFloor();

	  /* Draw reflected ninja, but only where floor is. */
	glPushMatrix();
		glRotatef(90, 1.0, 0, 0);
		glTranslatef(0, 1, -1.01);
		glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(0.7, 0.0, 0.0, 0.40);  /* 40% dark red floor color */
				drawGameboard(false);
				drawParticles(false);
		glDisable(GL_BLEND);
	glPopMatrix();
}
void Viewer::drawGrid()
{
	glColor3d(1, 0, 0);
	for (int i = 0;i<=WIDTH;i++)
	{
		glBegin(GL_LINES);
		glVertex3f(i, 0, 1);
		glVertex3f(i, HEIGHT, 1);
		glEnd();
	}
	
	for (int i = 0;i<=HEIGHT;i++)
	{
		glBegin(GL_LINES);
		glVertex3f(0, i, 1);
		glVertex3f(WIDTH, i, 1);
		glEnd();
	}
	
	float squareLength = 0.07;
	for (int i = 1;i<WIDTH;i++)
	{
		for (int j=1;j<HEIGHT;j++)
		{
			glBegin(GL_QUADS);
				glVertex3f(i - squareLength, j - squareLength, 1);
				glVertex3f(i + squareLength, j - squareLength, 1);
				glVertex3f(i + squareLength, j + squareLength, 1);
				glVertex3f(i - squareLength, j + squareLength, 1);
			glEnd();
		}
	}
	
	for (int i = 1;i<HEIGHT;i++)
	{
			glBegin(GL_QUADS);
				glVertex3f(0, i - squareLength, 1);
				glVertex3f(squareLength, i - squareLength, 1);
				glVertex3f(squareLength, i + squareLength, 1);
				glVertex3f(0, i + squareLength, 1);
			glEnd();
			
			glBegin(GL_QUADS);
				glVertex3f(WIDTH, i - squareLength, 1);
				glVertex3f(WIDTH - squareLength, i - squareLength, 1);
				glVertex3f(WIDTH - squareLength, i + squareLength, 1);
				glVertex3f(WIDTH, i + squareLength, 1);
			glEnd();
	}
	
	glLineWidth(1.5);
	float buffer = 1.f;
	glBegin(GL_LINES);
		glVertex3f(-buffer, 0, 1);
		glVertex3f(-buffer, HEIGHT, 1);
		
		glVertex3f(WIDTH + buffer, 0, 1);
		glVertex3f(WIDTH + buffer, HEIGHT, 1);
		
		glVertex3f(-buffer, 0, 1);
		glVertex3f(WIDTH + buffer, 0, 1);		
	glEnd();
}
void Viewer::drawParticles(bool step)
{
	for (unsigned int i = 0;i<game->blocksJustCleared.size();i++)
	{
		addParticleBox(game->blocksJustCleared[i].c, game->blocksJustCleared[i].r, game->blocksJustCleared[i].col);
	}
	game->blocksJustCleared.clear();
	glEnable(GL_BLEND);
	float mag;
	Point3D pos;
	float rad;
	float *colour;
	Vector3D velocity;
	Point3D col;
	float alpha;
	glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);
	for (unsigned int i = 0;i<particles.size();i++)
	{
		pos = particles[i]->getPos();
		rad = particles[i]->getRadius();
		alpha = particles[i]->getAlpha();
		
		if (particles[i]->getShape() == 0)
		{			
			mag = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
			mag = (1.f/mag);
			glColor4f(mag * pos[0], mag * pos[1], mag * pos[2], alpha);
			glPushMatrix();
				glTranslatef(pos[0], pos[1], pos[2]);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, texture[particles[i]->getColourIndex() - 1]);
				glScalef(rad, rad, 1);
				glCallList(reflectCubeDisplayList);
/*				glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3d(0, 0, 1);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3d(rad, 0, 1);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3d(rad, rad, 1);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3d(0, rad, 1);
				glEnd();*/
			glPopMatrix();
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);	
		}	
		else if (particles[i]->getShape() == 1 && step)
		{
			colour = particles[i]->getColour();
			glColor4f(colour[0], colour[1], colour[2], alpha);
			glPushMatrix();
				glTranslatef(pos[0], pos[1], pos[2]);
				glScalef(rad, rad, rad);
				glCallList(sphereDisplayList);
			glPopMatrix();	

			if (colour[1] > 1)
			{
				colour[1] = 1;
				colour[0] = 0;
			}
			
			if (colour[2] > 1)
				colour[2] = 1;
				
			if (colour[0] > 0)
			{
				colour[1] += 0.2;
			}
			else
				colour[2] += 0.2;
		}
		
		if (step && particles[i]->step(0.1))
		{
			particles.erase(particles.begin() + i);
		}
	}
	glDisable(GL_BLEND);
}
void Viewer::addParticleBox(float x, float y, int colour)
{
	Point3D pos(x, y, 0);
	float radius = 0.2f;
	float decay = 2.f;
	float n = 10.f;
	float empty[3];
	empty[0] = 0;
	empty[1] = 0;
	empty[2] = 0;
	for (int i = 0;i<n;i++)
	{
		for (int j = 0;j<n;j++)
		{
			Vector3D randVel(rand()%5 - 2.5f, rand()%5 - 2.5f, 0);
			Vector3D randAccel(rand()%5 - 2.5f, rand()%5 - 2.5f, 0);
			Particle *p = new Particle(pos, radius, randVel, decay, empty, randAccel, 0);
			p->setColourIndex(colour);
			particles.push_back(p);			
			pos[0] = x + (j / n);
		}
		pos[1] = pos[1] + 1.f/n;
	}
}

void Viewer::addFireworks(float x, float y)
{
	Point3D pos(x, y, 0);
	float radius = 0.2f;
	float decay = 2.5f;
	float n = 10.f;
	for (int i = 0;i<n;i++)
	{
		for (int j = 0;j<n;j++)
		{
			float *colour = (float *)malloc(sizeof(float) * 3);
			colour[0] = rand()%1000 + 1000;
			colour[1] = rand()%1000 + 1000;
			colour[2] = rand()%1000 + 1000;
			colour[0] /= 1000.f;
			colour[1] /= 1000.f;
			colour[2] /= 1000.f;
			
			colour[0] = 1;
			colour[1] = 0;
			colour[2] = 0;
			float a = rand()%10000 + 1000;
			float b = rand()%10000 + 1000;
			a /= 1000.f;
			b /= 1000.f;
			Vector3D randVel(a - 5.f, b - 5.f, 0);
			Vector3D randAccel(0, -9.8f + rand() % 5, 0);
			particles.push_back(new Particle(pos, radius, randVel, decay, colour, randAccel, 1));			
			//pos[0] = x + (j / n);
		}
	//	pos[1] = pos[1] + 1.f/n;
	}
}

void Viewer::drawRoom()							// Draw The Room (Box)
{
	glColor3d(0, 1, 0);
	glBegin(GL_QUADS);						// Begin Drawing Quads
		// Floor
		glNormal3f(0.0f, 1.0f, 0.0f);				// Normal Pointing Up
		glVertex3f(-4.0f,0.0f,-20.0f);			// Back Left
		glVertex3f(-4.0f,0.0f, 20.0f);			// Front Left
		glVertex3f( 20.0f,0.0f, 20.0f);			// Front Right
		glVertex3f( 20.0f,0.0f,-20.0f);			// Back Right
		// Ceiling
		glNormal3f(0.0f,-1.0f, 0.0f);				// Normal Point Down
		glVertex3f(-4.0f, 20.0f, 20.0f);			// Front Left
		glVertex3f(-4.0f, 20.0f,-20.0f);			// Back Left
		glVertex3f( 20.0f, 20.0f,-20.0f);			// Back Right
		glVertex3f( 20.0f, 20.0f, 20.0f);			// Front Right
		// Front Wall
		glNormal3f(0.0f, 0.0f, 1.0f);				// Normal Pointing Away From Viewer
		glVertex3f(-4.0f, 20.0f,-20.0f);			// Top Left
		glVertex3f(-4.0f, 0.0f,-20.0f);			// Bottom Left
		glVertex3f( 20.0f, 0.0f,-20.0f);			// Bottom Right
		glVertex3f( 20.0f, 20.0f,-20.0f);			// Top Right
/*		// Back Wall
		glNormal3f(0.0f, 0.0f,-1.0f);				// Normal Pointing Towards Viewer
		glVertex3f( 20.0f, 15.0f, 20.0f);			// Top Right
		glVertex3f( 20.0f,-15.0f, 20.0f);			// Bottom Right
		glVertex3f(-20.0f,-15.0f, 20.0f);			// Bottom Left
		glVertex3f(-20.0f, 15.0f, 20.0f);			// Top Left*/
		// Left Wall
		glNormal3f(1.0f, 0.0f, 0.0f);				// Normal Pointing Right
		glVertex3f(-4.0f, 20.0f, 20.0f);			// Top Front
		glVertex3f(-4.0f, 0.0f, 20.0f);			// Bottom Front
		glVertex3f(-4.0f, 0.0f,-20.0f);			// Bottom Back
		glVertex3f(-4.0f, 20.0f,-20.0f);			// Top Back
		// Right Wall
		glNormal3f(-1.0f, 0.0f, 0.0f);				// Normal Pointing Left
		glVertex3f( 20.0f, 20.0f,-20.0f);			// Top Back
		glVertex3f( 20.0f, 0.0f,-20.0f);			// Bottom Back
		glVertex3f( 20.0f, 0.0f, 20.0f);			// Bottom Front
		glVertex3f( 20.0f, 20.0f, 20.0f);			// Top Front
	glEnd();							// Done Drawing Quads
}


void Viewer::drawShadowVolumes()
{
	for (int i = HEIGHT+3;i>=0;i--) // row
	{
		for (int j = WIDTH - 1; j>=0;j--) // column
		{				
			
			if (game->get(i, j) != -1)
			{
				drawShadowCube (i, j, GL_QUADS );
				
			}
		}
	}
	
	
	Vector3D temp, temp2;
	Point3D temp3;
	Point3D lightPoint(lightPos[0], lightPos[1], lightPos[2]);
	glColor4d(0, 0, 0, 0.3);
	
	for (unsigned int i = 0;i<silhouette.size();i++)
	{
		
		temp =  silhouette[i].first - lightPoint;
		temp2 = silhouette[i].second - lightPoint;
		temp3 = silhouette[i].first +  100*temp;
			
			glBegin(GL_QUADS);			
			glVertex3d(silhouette[i].first[0], silhouette[i].first[1], silhouette[i].second[2]);
			glVertex3d(temp3[0], temp3[1], temp3[2]);
			temp3 = silhouette[i].second + 100*temp2;	
			glVertex3d(temp3[0], temp3[1], temp3[2]);
			glVertex3d(silhouette[i].second[0], silhouette[i].second[1], silhouette[i].second[2]);
			glEnd();
	}
	glColor4d(1, 1, 1, 1);
}

void Viewer::drawShadowCube(float y, float x, GLenum mode)
{
	Point3D a, b, c, d, e, f, g, h, temp3;
	Point3D lightPoint(lightPos[0], lightPos[1], lightPos[2]);
	Vector3D temp, temp2;
	Vector3D lightVec = lightPoint - Point3D(0, 0, 0);
	a = Point3D(x, y, 1);
	b = Point3D(1 + x, y, 1);
	c = Point3D(1 + x, 1 + y, 1);
	d = Point3D(x, 1 + y, 1);
	e = a;
	f = b;
	g = c;
	h = d;
	
	e[2] = 0;
	f[2] = 0;
	g[2] = 0;
	h[2] = 0;
	
	if (Vector3D(0, 0, 1).dot(lightVec) > 0)
	{
		silhouette.push_back(std::pair<Point3D, Point3D>(a, b ));
		silhouette.push_back(std::pair<Point3D, Point3D>(c, b ));
		silhouette.push_back(std::pair<Point3D, Point3D>(c, d ));
		silhouette.push_back(std::pair<Point3D, Point3D>(d, a));		
	}
	
	if (Vector3D(0, 1, 0).dot(lightVec) > 0)
	{
		silhouette.push_back(std::pair<Point3D, Point3D>(c, d) );
		silhouette.push_back(std::pair<Point3D, Point3D>(c, g) );
		silhouette.push_back(std::pair<Point3D, Point3D>(g, h) );
		silhouette.push_back(std::pair<Point3D, Point3D>(h, d) );		
	}
	

	
	if (Vector3D(1, 0, 0).dot(lightVec) > 0)
	{
		silhouette.push_back(std::pair<Point3D, Point3D>(b, f) );
		silhouette.push_back(std::pair<Point3D, Point3D>(c, g) );
		silhouette.push_back(std::pair<Point3D, Point3D>(b, c) );
		silhouette.push_back(std::pair<Point3D, Point3D>(g, f) );		
	}
	
	if (Vector3D(-1, 0, 0).dot(lightVec) > 0)
	{
		silhouette.push_back(std::pair<Point3D, Point3D>(a, e) );
		silhouette.push_back(std::pair<Point3D, Point3D>(d, h) );
		silhouette.push_back(std::pair<Point3D, Point3D>(a, d) );
		silhouette.push_back(std::pair<Point3D, Point3D>(h, e) );
		std::cerr << "left face " << Vector3D(-1, 0, 0).dot(lightVec) << "\n";		
	}
	
	if (Vector3D(0, -1, 0).dot(lightVec) > 0)
	{
		silhouette.push_back(std::pair<Point3D, Point3D>(b, a) );
		silhouette.push_back(std::pair<Point3D, Point3D>(b, f) );
		silhouette.push_back(std::pair<Point3D, Point3D>(f, e) );
		silhouette.push_back(std::pair<Point3D, Point3D>(e, a) );		
		std::cerr << "bottom face " << Vector3D(0, -1, 0).dot(lightVec) << "\n";
	}

	if (Vector3D(0, 0, -1).dot(lightVec) > 0)
	{
		silhouette.push_back(std::pair<Point3D, Point3D>(e, f) );
		silhouette.push_back(std::pair<Point3D, Point3D>(g, f) );
		silhouette.push_back(std::pair<Point3D, Point3D>(g, h) );
		silhouette.push_back(std::pair<Point3D, Point3D>(h, e) );
		std::cerr << "back face " << Vector3D(0, 0, -1).dot(lightVec) << "\n";		
	}
	for (unsigned int i = 0;i<silhouette.size();i++)
	{
		for (unsigned int j = i+1;j<silhouette.size();j++)
		{
			if (silhouette[i].first == silhouette[j].first && silhouette[i].second == silhouette[j].second)	
			{
			//	std::cerr << silhouette[i].first << "\t" << silhouette[j].first << "\t" << silhouette[i].second << "\t" << silhouette[j].second << std::endl;
				silhouette.erase (silhouette.begin() + i);
				silhouette.erase (silhouette.begin() + j-1);
				break;
			}
			else if (silhouette[i].first == silhouette[j].second && silhouette[i].second == silhouette[j].first)	
			{
			//	std::cerr << silhouette[i].first << "\t" << silhouette[j].second << "\t" << silhouette[i].second << "\t" << silhouette[j].first << std::endl;
				silhouette.erase (silhouette.begin() + i);
				silhouette.erase (silhouette.begin() + j-1);
				break; 
			}
		}
	}
	
}


void Viewer::drawBar()
{
		// Clear bar
	double clearBarPos = game->getClearBarPos();
	glBegin(GL_LINE_LOOP);
		glVertex3d(clearBarPos, 0, 0);
		glVertex3d(clearBarPos, 0, 1);
		glVertex3d(clearBarPos, HEIGHT, 1);
		glVertex3d(clearBarPos, HEIGHT, 0);
	glEnd();
	// int n = 1;
	// glEnable (GL_BLEND);	
	// for (int i = 0;i < n;i++)
	// {
	// 	glColor4f(0.f, 0.f, 0.f, i * 1.f / n);
	// 	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// 	glBegin(GL_LINE_LOOP);
	// 		glVertex3d(clearBarPos - i * 1.f / (n), 0, 0);
	// 		glVertex3d(clearBarPos - i * 1.f / (n), 0, 1);
	// 		glVertex3d(clearBarPos - i * 1.f / (n), HEIGHT, 1);
	// 		glVertex3d(clearBarPos - i * 1.f / (n), HEIGHT, 0);
	// 	glEnd();	
	// }
	// 
	// glDisable(GL_BLEND);
}

void Viewer::drawFallingBox()
{	
	int iter = 4;
	float iterFrac = 1.f/iter;
	for (int i = 0;i<iter;i++)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			drawScene();
		
		
		glPushMatrix();
	    	glTranslatef(0, i * iterFrac, 0);
			drawCube (game->py_ - 1, game->px_ + 1, game->get(game->py_ - 1, game->px_ + 1), GL_QUADS );
			drawCube (game->py_ - 1, game->px_ + 2, game->get(game->py_ - 1, game->px_ + 2), GL_QUADS );
			drawCube (game->py_ - 2, game->px_ + 1, game->get(game->py_ - 1, game->px_ + 1), GL_QUADS );
			drawCube (game->py_ - 2, game->px_ + 2, game->get(game->py_ - 1, game->px_ + 2), GL_QUADS );

			drawCube (game->py_ - 1, game->px_ + 1, 7, GL_LINE_LOOP );
			drawCube (game->py_ - 1, game->px_ + 2, 7, GL_LINE_LOOP );
			drawCube (game->py_ - 2, game->px_ + 1, 7, GL_LINE_LOOP );
			drawCube (game->py_ - 2, game->px_ + 2, 7, GL_LINE_LOOP );
	    glPopMatrix();
		glAccum(GL_ACCUM, iterFrac);
		
		//glDrawBuffer(GL_FRONT);
	//    glAccum(GL_RETURN, 1.0);
	//    glDrawBuffer(GL_BACK);
	}
}

void Viewer::drawFloor()
{
			// Draw Floor
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, floorTexId);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	
			glNormal3f(0.0, 1.0, 0.0);
			glColor3d(1, 1, 1);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0);
	  	glVertex3d(-100, 0, -100);
			glTexCoord2f(0.0, 10.0);
	  	glVertex3d(-100, 0, 100);
			glTexCoord2f(10.0, 10.0);
	  	glVertex3d(100, 0, 100);
			glTexCoord2f(10.0, 0.0);
	  	glVertex3d(100, 0, -100);
	  	glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);	
}
void Viewer::drawGameboard(bool draw3D)
{	
	for (int i = HEIGHT+3;i>=0;i--) // row
	{
		for (int j = WIDTH - 1; j>=0;j--) // column
		{				
			if(loadBumpMapping && game->get(i, j) != -1)
				drawBumpCube (i, j, game->get(i, j), draw3D );
			else if(game->get(i, j) != -1)
			{
				glPushMatrix();
					glTranslatef(j, i, 0);
					drawCube (i, j, game->get(i, j), GL_QUADS, draw3D );
				glPopMatrix();
			}
				
				
			// Draw outline for cube
			if (game->get(i, j) != -1)
			{
				glPushMatrix();
					glTranslatef(j, i, 0);
					drawCube(i, j, 7, GL_LINE_LOOP, draw3D);
				glPopMatrix();
			}

		}
	}	
	
	// Draw next piece
	int nextPieceCol[4];
	game->getNextPieceColour(nextPieceCol);
	glPushMatrix();
		glTranslatef(19, 2, 0);
		drawCube(2, 19, nextPieceCol[0], GL_QUADS, draw3D);
	glPopMatrix();
	glPushMatrix();
		glTranslatef(20, 2, 0);
		drawCube(2, 20, nextPieceCol[1], GL_QUADS, draw3D);
	glPopMatrix();
	glPushMatrix();
		glTranslatef(19, 1, 0);
		drawCube(1, 19, nextPieceCol[2], GL_QUADS, draw3D);
	glPopMatrix();
	glPushMatrix();
		glTranslatef(20, 1, 0);
		drawCube(1, 20, nextPieceCol[3], GL_QUADS, draw3D);
	glPopMatrix();
}
bool Viewer::on_configure_event(GdkEventConfigure* event)
{
  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

  if (!gldrawable) return false;
  
  if (!gldrawable->gl_begin(get_gl_context()))
    return false;

  // Set up perspective projection, using current size and aspect
  // ratio of display

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, event->width, event->height);
  gluPerspective(40.0, (GLfloat)event->width/(GLfloat)event->height, 0.1, 1000.0);

  // Reset to modelview matrix mode
  
  glMatrixMode(GL_MODELVIEW);

  gldrawable->gl_end();

  return true;
}

bool Viewer::on_button_press_event(GdkEventButton* event)
{
	startPos[0] = event->x;
	startPos[1] = event->y;
	mouseDownPos[0] = event->x;
	mouseDownPos[1] = event->y;
	if (loadScreen)
	{
		GLuint buff[16] = {0};
	 	GLint hits, view[4];

		glSelectBuffer(16,buff);
		glRenderMode(GL_SELECT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
			glLoadIdentity();
			glGetIntegerv(GL_VIEWPORT,view);
			gluPickMatrix(event->x, view[3] - event->y, 1, 1, view);			
			gluPerspective(40.0, (GLfloat)get_width()/(GLfloat)get_height(), 0.1, 1000.0);
			glTranslated(-3.0, 5.0, -30.0);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslated(-7.5, -10.0, 7.0);
			glInitNames();
			glPushName(0);
				drawStartScreen(true);
			glPopName();
			glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glFlush();

		// returning to normal rendering mode
		hits = glRenderMode(GL_RENDER);

		// if there are hits process them

		if (hits > 0)
		{
			GLuint names, *ptr, minZ,*ptrNames, numberOfNames;

			ptr = (GLuint *) buff;
			minZ = 0xffffffff;
			for (int i = 0; i < hits; i++) 
			{	
				names = *ptr;
				ptr++;
				if (*ptr < minZ) 
				{
					numberOfNames = names;
					minZ = *ptr;
					ptrNames = ptr+2;
				}

				ptr += names+2;
			}
			
			ptr = ptrNames;
			for (unsigned int j = 0; j < numberOfNames; j++,ptr++) 
			{

			}
			ptr--;

			if (*ptr == playButtonTex)
			{
				playButtonTex = playButtonClickedTex;				
				clickedButton = true;
			}
			else if (*ptr == soundOnTex)
			{
				GLuint temp;
				temp = soundOnTex;
				soundOnTex = soundOffTex;
				soundOffTex = temp;
				disableSound = !disableSound;
				if (!disableSound)
					sm.PlaySound(introMusic, -1);
				else
					sm.StopSound(introMusic);
			}
			else if (*ptr == singleSkinModeTex)
			{
				GLuint temp;
				temp = singleSkinModeTex;
				singleSkinModeTex = singleSkinModeClickedTex;
				singleSkinModeClickedTex = temp;
				clickedButton = true;
				singleSkinMode = true;
			}
			drawStartScreen(false);
			invalidate();
		}
	}
	// Stop rotating if a mosue button was clicked and the shift button is not down
	if ((rotateAboutX || rotateAboutY || rotateAboutZ) && !shiftIsDown)
	{
		rotationSpeed = 0;
		rotateTimer.disconnect();
	}
		
	// Set our appropriate flags to true
	if (event->button == 1)
		mouseB1Down = true;	
	if (event->button == 2)
		mouseB2Down = true;
	if (event->button == 3)
		mouseB3Down = true;

	// If shift is not held down we can stop rotating
	if (!shiftIsDown)
	{
		rotateAboutX = false;
		rotateAboutY = false;
		rotateAboutZ = false;
	}
	return true;
}

bool Viewer::on_button_release_event(GdkEventButton* event)
{
	startScalePos[0] = 0;
	startScalePos[1] = 0;
	if (clickedButton && loadScreen)
	{
		loadScreen = false;
		sm.StopSound(introMusic);
		if (!disableSound)
			sm.PlaySound(backgroundMusic, -1);
	}
		
	if (!shiftIsDown)
	{
		// Set the rotation speed based on how far the cursor has moved since the mouse down event
	
	}

	// Set the appropriate flags to true and false
	if (event->button == 1)
	{
		mouseB1Down = false;
		if (!shiftIsDown)
			rotateAboutX = true;
	}	
	if (event->button == 2)
	{
		mouseB2Down = false;
		if (!shiftIsDown)
			rotateAboutY = true;
	}
		
	if (event->button == 3)
	{
		mouseB3Down = false;
		if (!shiftIsDown)
			rotateAboutZ = true;
	}
		
	invalidate();
	return true;
}

bool Viewer::on_motion_notify_event(GdkEventMotion* event)
{
	double x2x1;
	
	if (moveLightSource)
	{
			// See how much we have moved
			x2x1 = event->x - startPos[0];
			x2x1 /= 10;
			if (mouseB1Down) // Rotate x
				lightPos[0] += x2x1;
			if (mouseB2Down) // Rotate y
				lightPos[1]  += x2x1;
			if (mouseB3Down) // Rotate z
				lightPos[2]  += x2x1;

			invalidate();
			return true;
	}
	if (shiftIsDown) // Start Scaling
	{
		// Store some initial values
		if (startScalePos[0] == 0 && startScalePos[1] == 0)
		{
			startScalePos[0] = event->x;
			startScalePos[1] = event->y;
			return true;
		}
		
		// See how much we have moved
		x2x1 = (event->x - startScalePos[0]);
		
		// Create a scale factor based on that value
		if (x2x1 != 0)
		{
			x2x1 /= 500;
			scaleFactor += x2x1;				
		}
		
		if (scaleFactor < 0.5)
			scaleFactor = 0.5;
			
		startScalePos[0] = event->x;
		startScalePos[1] = event->y;
		if (rotateTimer.connected())
			return true;
	}
	else // Start rotating
	{
		// Keep track of the time of the last motion event
		timeOfLastMotionEvent = event->time;
		
		// Determine change in distance. This is used to calculate the rotation angle
		x2x1 = event->x - startPos[0];
		x2x1 /= 10;
		
		if (mouseB1Down) // Rotate x
			rotationAngleX += x2x1;
		if (mouseB2Down) // Rotate y
			rotationAngleY += x2x1;
		if (mouseB3Down) // Rotate z
			rotationAngleZ += x2x1;
			
		// Reset the tickTimer
		if (!rotateTimer.connected())
			rotateTimer = Glib::signal_timeout().connect(sigc::bind(sigc::mem_fun(*this, &Viewer::on_expose_event), (GdkEventExpose *)NULL), 100);
	}
	
	// Store the position of the cursor
	startPos[0] = event->x;
	startPos[1] = event->y;
	invalidate();
	return true;
}

void Viewer::drawBumpCube(float y, float x, int colourId, bool draw3D)
{
	// Set The First Texture Unit To Normalize Our Vector From The Surface To The Light.
	// Set The Texture Environment Of The First Texture Unit To Replace It With The
	// Sampled Value Of The Normalization Cube Map.
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE) ;
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE) ;

	// Set The Second Unit To The Bump Map.
	// Set The Texture Environment Of The Second Texture Unit To Perform A Dot3
	// Operation With The Value Of The Previous Texture Unit (The Normalized
	// Vector Form The Surface To The Light) And The Sampled Texture Value (The
	// Normalized Normal Vector Of Our Bump Map).
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bumpMap);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB) ;
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS) ;
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE) ;

	// Set The Third Texture Unit To Our Texture.
	// Set The Texture Environment Of The Third Texture Unit To Modulate
	// (Multiply) The Result Of Our Dot3 Operation With The Texture Value.
	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[colourId - 1]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Now We Draw Our Object (Remember That We First Have To Calculate The
	// (UnNormalized) Vector From Each Vertex To Our Light).

	double innerXMin = 0;
	double innerYMin = 0;
	double innerXMax = 1;
	double innerYMax = 1;
	double zMax = 1;
	double zMin = 0;
	
	Point3D vertex_position, light_position;
	light_position[0] = lightPos[0];
	light_position[1] = lightPos[1];
	light_position[2] = lightPos[2];
	Vector3D vertex_to_light;
	glBegin(GL_QUADS);
		// lower left Vertex
		vertex_position = Point3D(innerXMin + x, innerYMin + y, zMax);
		vertex_to_light = light_position - vertex_position;
		glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
		glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
		glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 0.0f);
		glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


		vertex_position = Point3D(innerXMax + x, innerYMin + y, zMax);
		vertex_to_light = light_position - vertex_position;
		glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
		glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
		glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 0.0f);
		glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);

        vertex_position = Point3D(innerXMax + x, innerYMax + y, zMax);
		vertex_to_light = light_position - vertex_position;
		glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
		glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
		glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 1.0f);
		glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


		// upper left Vertex
		vertex_position = Point3D(innerXMin + x, innerYMax + y, zMax);
		vertex_to_light = light_position - vertex_position;
		glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
		glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
		glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 1.0f);
		glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);
	glEnd();
	
	if (draw3D)
	{
		glBegin(GL_QUADS);
			// lower left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMax + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			vertex_position = Point3D(innerXMax + x, innerYMax + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);

	        vertex_position = Point3D(innerXMax + x, innerYMax + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			// upper left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMax + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);
		glEnd();

		glBegin(GL_QUADS);
			// lower left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMin + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			vertex_position = Point3D(innerXMin + x, innerYMax + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);

	        vertex_position = Point3D(innerXMin + x, innerYMax + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			// upper left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMin + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);
		glEnd();

		glBegin(GL_QUADS);
			// lower left Vertex
			vertex_position = Point3D(innerXMax + x, innerYMin + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			vertex_position = Point3D(innerXMax + x, innerYMax + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);

	        vertex_position = Point3D(innerXMax + x, innerYMax + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			// upper left Vertex
			vertex_position = Point3D(innerXMax + x, innerYMin + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);
		glEnd();

		glBegin(GL_QUADS);
			// lower left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMin + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			vertex_position = Point3D(innerXMax + x, innerYMin + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);

	        vertex_position = Point3D(innerXMax + x, innerYMax + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			// upper left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMax + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);
		glEnd();

		glBegin(GL_QUADS);
			// lower left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMin + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			vertex_position = Point3D(innerXMax + x, innerYMin + y, zMin);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 0.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);

	        vertex_position = Point3D(innerXMax + x, innerYMin + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 1.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);


			// upper left Vertex
			vertex_position = Point3D(innerXMin + x, innerYMin + y, zMax);
			vertex_to_light = light_position - vertex_position;
			glMultiTexCoord3f(GL_TEXTURE0, vertex_to_light[0], vertex_to_light[1], vertex_to_light[2]);
			glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
			glMultiTexCoord2f(GL_TEXTURE2, 0.0f, 1.0f);
			glVertex3f(vertex_position[0], vertex_position[1], vertex_position[2]);
		glEnd();	
	}
	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);

	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
	glActiveTexture(GL_TEXTURE2);
//	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
}
void Viewer::drawCube(float y, float x, int colourId, GLenum mode, bool draw3D)
{
	if (mode == GL_LINE_LOOP)
		glLineWidth (1.2);
			
	double r, g, b;
	r = 0;
	g = 0;
	b = 0;
	switch (colourId)
	{
		case 0:	// blue
			r = 0.514;
			g = 0.839;
			b = 0.965;
			break;              
		case 1:	// purple       
			r = 0.553;          
			g = 0.6;            
			b = 0.796;          
			break;              
		case 2: // orange       
			r = 0.988;          
			g = 0.627;          
			b = 0.373;          
			break;              
		case 3:	// green        
			r = 0.69;           
			g = 0.835;          
			b = 0.529;          
			break;              
		case 4:	// red          
			r = 1.00;           
			g = 0.453;          
			b = 0.339;          
			break;              
		case 5:	// pink         
			r = 0.949;          
			g = 0.388;          
			b = 0.639;          
			break;              
		case 6:	// yellow       
			r = 1;              
			g = 0.792;          
			b = 0.204;          
			break;
		case 7:	// black
			r = 0;
			g = r;
			b = g;
			break;
		default:
			return;
	}
	
	double innerXMin = 0;
	double innerYMin = 0;
	double innerXMax = 1;
	double innerYMax = 1;
	double zMax = 1;
	double zMin = 0;
	
	if (transluceny)
	{
		glColor4f(1.0f,1.0f,1.0f,0.5f);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} 
	
	glNormal3d(1, 0, 0);
	
	if (loadTexture && colourId != 7)
	{		
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture[colourId - 1]);
	}

	else if (loadTexture && colourId == 7)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture[4]);	
	}	
	else
	{
		glColor3d(r, g, b);
	}
	

	if (mode == GL_LINE_LOOP && draw3D)
	{
		glCallList(outlineDisplayList);
	}
	else if (draw3D)
	{
		glCallList(texCubeDisplayList);
	}
	else if (!draw3D)
	{
		glCallList(reflectCubeDisplayList);
	}
/*

	if (draw3D)
	{
		// top face
		glNormal3d(0, 1, 0);

		glBegin(mode);
		 	glTexCoord2f(0.0f, 0.0f);
			glVertex3d(innerXMin + x, innerYMax + y, zMin);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(innerXMax + x, innerYMax + y, zMin);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(innerXMax + x, innerYMax + y, zMax);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(innerXMin + x, innerYMax + y, zMax);
		glEnd();

		// left face
		glNormal3d(0, 0, -1);

		glBegin(mode);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(innerXMin + x, innerYMin + y, zMin);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(innerXMin + x, innerYMax + y, zMin);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(innerXMin + x, innerYMax + y, zMax);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glEnd();

		// bottom face
		glNormal3d(0, -1, 0);

		glBegin(mode);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(innerXMin + x, innerYMin + y, zMin);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(innerXMax + x, innerYMin + y, zMin);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(innerXMax + x, innerYMin + y, zMax);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glEnd();

		// right face
		glNormal3d(0, 0, 1);

		glBegin(mode);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(innerXMax + x, innerYMin + y, zMin);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(innerXMax + x, innerYMax + y, zMin);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(innerXMax + x, innerYMax + y, zMax);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glEnd();

		// Back of front face
		glNormal3d(-1, 0, 0);

		glBegin(mode);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(innerXMin + x, innerYMin + y, zMin);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(innerXMax + x, innerYMin + y, zMin);
			glTexCoord2f(1.0f, 1.0f);	
			glVertex3d(innerXMax + x, innerYMax + y, zMin);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glEnd();
	
	}
*/
	glBindTexture(GL_TEXTURE_2D, 0);
	if (transluceny)
		glDisable(GL_BLEND);

}

void Viewer::startScale()
{
	shiftIsDown = true;
}

void Viewer::endScale()
{
	shiftIsDown = false;
	startScalePos[0] = 0;
	startScalePos[1] = 0;
}

void Viewer::setSpeed(Speed newSpeed)
{
	// Keep track of the current speed menu value
	speed = newSpeed;
	switch (newSpeed)
	{
		case SLOW:
			gameSpeed = 500;
			break;
		case MEDIUM:
			gameSpeed = 250;
			break;
		case FAST:
			gameSpeed = 100;
			break;
	}
	
	// Update game tick timer
	tickTimer.disconnect();
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
}

void Viewer::toggleBuffer() 
{ 
	// Toggle the value of doubleBuffer
	doubleBuffer = !doubleBuffer;
	invalidate();
}

void Viewer::toggleShadows()
{
	drawShadow = !drawShadow;
}

void Viewer::setDrawMode(DrawMode mode)
{
	currentDrawMode = mode;
	invalidate();
}

bool Viewer::on_key_press_event( GdkEventKey *ev )
{
	// Don't process movement keys if its game over
	if (gameOver)
		return true;
	
	if (loadScreen || !disableSound)
	{
		if (ev->keyval == GDK_Left || ev->keyval == GDK_Right)
			sm.PlaySound(moveSound);
		else if (ev->keyval == GDK_Up || ev->keyval == GDK_Down)
			sm.PlaySound(turnSound);		
	}

		
	int r, c;
	r = game->py_;
	c = game->px_;
	if (ev->keyval == GDK_Left)
		moveLeft = game->moveLeft();
	else if (ev->keyval == GDK_Right)
		moveRight = game->moveRight();
	else if (ev->keyval == GDK_Up)
		game->rotateCCW();
	else if (ev->keyval == GDK_Down)
		game->rotateCW();
	else if (ev->keyval == GDK_space)
		game->drop();	
	
	invalidate();
	return true;
}

bool Viewer::gameTick()
{
	if (loadScreen)
		return true;
	
	int cubesDeletedBeforeTick = game->getLinesCleared();
	int returnVal = game->tick();
	int cubesDeletedAfterTick = game->getLinesCleared();
	// String streams used to print score and lines cleared	
	std::stringstream scoreStream, linesStream, newTexStream; 
	std::string s;
	
	// Update the score
	scoreStream << game->getScore();
	scoreLabel->set_text("Score:\t" + scoreStream.str());
	
	if (!singleSkinMode && cubesDeletedBeforeTick/100 < cubesDeletedAfterTick/100)
	{
		int level = cubesDeletedAfterTick/100 + 1;
		if (level > NUM_TEXTURES)
			level = NUM_TEXTURES;
			
		newTexStream << level;
		s = "x" + newTexStream.str() + ".bmp";
		LoadGLTextures(s.c_str(), texture[0]);
		
		s = "o" + newTexStream.str() + ".bmp";
		LoadGLTextures(s.c_str(), texture[1]);

		s = "xLight" + newTexStream.str() + ".bmp";
		LoadGLTextures(s.c_str(), texture[2]);

		s = "oLight" + newTexStream.str() + ".bmp";
		LoadGLTextures(s.c_str(), texture[3]);
		
		s = "";
		
		levelUpAnimation = true;
	}
	// If a line was cleared update the linesCleared widget
	if (returnVal > 0)
	{
    	linesStream << game->getLinesCleared();
		linesClearedLabel->set_text("Deleted:\t" + linesStream.str());
	}
	

	if (game->getClearBarPos() >= WIDTH && game->numBlocksCleared > 5)
	{
		animatables.clear();
		readFile("headHappy.txt");	
	}
	
	if (game->getLinesCleared() / 10 > (DEFAULT_GAME_SPEED - gameSpeed) / 50 && gameSpeed > 75)
	{
		// Increase the game speed
		gameSpeed -= 50;
		
		// Update game tick timer
		tickTimer.disconnect();
		tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
	}
	
	if (returnVal < 0)
	{
		gameOver = true;
		animatables.clear();
		readFile("headSad.txt");
		tickTimer.disconnect();
		gameOverAnimTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::forceRender), gameSpeed);
	}
	
	invalidate();
	return true;
}

bool Viewer::forceRender()
{
	invalidate();
	return true;
}

void Viewer::resetView()
{
	// Reset all the rotations and scale factor
	rotationSpeed = 0;
	rotationAngleX = 0;
	rotationAngleY = 0;
	rotationAngleZ = 0;
	rotateAboutX = false;
	rotateAboutY = false;
	rotateAboutZ = false;
	rotateTimer.disconnect();
	
	scaleFactor = 1;
	invalidate();
}

void Viewer::newGame()
{
	gameOverAnimTimer.disconnect();
	animatables.clear();
	readFile("head.txt");
	gameOver = false;
	game->reset();
	
	// Restore gamespeed to whatever was set in the menu
	setSpeed(speed);
	
	// Reset the tickTimer
	tickTimer.disconnect();
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
	
	std::stringstream scoreStream, linesStream; 
	std::string s;
	
	// Update the score
	scoreStream << game->getScore();
	scoreLabel->set_text("Score:\t" + scoreStream.str());
	linesStream << game->getLinesCleared();
	linesClearedLabel->set_text("Lines Cleared:\t" + linesStream.str());
	
	
	// Load level 1 textures
	LoadGLTextures("x1.bmp", texture[0]);
	LoadGLTextures("o1.bmp", texture[1]);
	LoadGLTextures("xLight1.bmp", texture[2]);
	LoadGLTextures("oLight1.bmp", texture[3]);
	invalidate();
	
}

void Viewer::setScoreWidgets(Gtk::Label *score, Gtk::Label *linesCleared)
{
	scoreLabel = score;
	linesClearedLabel = linesCleared;
}

bool Viewer::moveClearBar()
{
	game->markBlocksForClearing();
	invalidate();
	return true;
}

void Viewer::drawStartScreen(bool pick)
{
	if (pick)
		glPushName(playButtonTex);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, playButtonTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3d(8, 8, 0);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3d(12, 8, 0);
		glTexCoord2f(1.0f, 1.0f);	
		glVertex3d(12, 10, 0);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3d(8, 10, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	if (pick)
	{
		glPopName();
		glPushName(soundOnTex);
	}
	
	glBindTexture(GL_TEXTURE_2D, soundOnTex);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3d(8, 5, 0);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3d(12, 5, 0);
		glTexCoord2f(1.0f, 1.0f);	
		glVertex3d(12, 7, 0);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3d(8, 7, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);	
	if (pick)
	{
		glPopName();
		glPushName(singleSkinModeTex);
	}
	
	glBindTexture(GL_TEXTURE_2D, singleSkinModeTex);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3d(8, 2, 0);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3d(12, 2, 0);
		glTexCoord2f(1.0f, 1.0f);	
		glVertex3d(12, 4, 0);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3d(8, 4, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);

	
	if (pick)
		glPopName();
}

// quick and dirty bitmap loader...for 24 bit bitmaps with 1 plane only.  
// See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/BMP.txt for more info.
int Viewer::ImageLoad(const char *filename, Image *image) {
    FILE *file;
    unsigned long size;                 // size of the image in bytes.
    unsigned long i;                    // standard counter.
    unsigned short int planes;          // number of planes in image (must be 1) 
    unsigned short int bpp;             // number of bits per pixel (must be 24)
    char temp;                          // temporary color storage for bgr-rgb conversion.

    // make sure the file is there.
    if ((file = fopen(filename, "rb"))==NULL)
    {
		printf("File Not Found : %s\n",filename);
		return 0;
    }
    
    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

    // read the width
    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
    }		
    printf("Width of %s: %lu\n", filename, image->sizeX);
    
    // read the height 
    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
    }
    	printf("Height of %s: %lu\n", filename, image->sizeY);
    
    // calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = image->sizeX * image->sizeY * 3;

    // read the planes
    if ((fread(&planes, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
    }
    if (planes != 1) {
		printf("Planes from %s is not 1: %u\n", filename, planes);
		return 0;
    }

    // read the bpp
    if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);
		return 0;
    }
    if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
    }
	
    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    // read the data. 
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
		printf("Error allocating memory for color-corrected image data");
		return 0;	
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
	temp = image->data[i];
	image->data[i] = image->data[i+2];
	image->data[i+2] = temp;
    }
    
    // we're done.
    return 1;
}
    
// Load Bitmaps And Convert To Textures
int  Viewer::LoadGLTextures(const char *filename, GLuint &texid) {	
    // Load Texture
    Image *image1;
    
    // allocate space for texture
    image1 = (Image *) malloc(sizeof(Image));
    if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
    }

    if (!ImageLoad(filename, image1)) {
	exit(1);
    }        
	
    // Create Texture	
	glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);   // 2d texture (x and y size)

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    // 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, 
    // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

	numTextures++;
	return (numTextures-1);
}


int Viewer::GenNormalizationCubeMap(unsigned int size, GLuint &texid)
{
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

	unsigned char* data = new unsigned char[size*size*3];

	float offset = 0.5f;
	float halfSize = size * 0.5f;
	Vector3D temp;
	unsigned int bytePtr = 0;

	for(unsigned int j=0; j<size; j++)
	{
		for(unsigned int i=0; i<size; i++)
		{
			temp[0] = halfSize;
			temp[1] = (j+offset-halfSize);
			temp[2] = -(i+offset-halfSize);
			temp.normalize();

			data[bytePtr] = (unsigned char)(temp[0] * 255.0f);
			data[bytePtr+1] = (unsigned char)(temp[1] * 255.0f);
			data[bytePtr+2] = (unsigned char)(temp[2] * 255.0f);

			bytePtr+=3;
		}
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
			0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	bytePtr = 0;
	for(unsigned int j=0; j<size; j++)
	{
		for(unsigned int i=0; i<size; i++)
		{
			temp[0] = -halfSize;
			temp[1] = (j+offset-halfSize);
			temp[2] = (i+offset-halfSize);
			temp.normalize();

			data[bytePtr] = (unsigned char)(temp[0] * 255.0f);
			data[bytePtr+1] = (unsigned char)(temp[1] * 255.0f);
			data[bytePtr+2] = (unsigned char)(temp[2] * 255.0f);

			bytePtr+=3;
		}
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	bytePtr = 0;
	for(unsigned int j=0; j<size; j++)
	{
		for(unsigned int i=0; i<size; i++)
		{
			temp[0] = i+offset-halfSize;
			temp[1] = -halfSize;
			temp[2] = j+offset-halfSize;
			temp.normalize();

			data[bytePtr] = (unsigned char)(temp[0] * 255.0f);
			data[bytePtr+1] = (unsigned char)(temp[1] * 255.0f);
			data[bytePtr+2] = (unsigned char)(temp[2] * 255.0f);

			bytePtr+=3;
		}
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
			0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	bytePtr = 0;
	for(unsigned int j=0; j<size; j++)
	{
		for(unsigned int i=0; i<size; i++)
		{
			temp[0] = i+offset-halfSize;
			temp[1] = halfSize;
			temp[2] = -(j+offset-halfSize);
			temp.normalize();

			data[bytePtr] = (unsigned char)(temp[0] * 255.0f);
			data[bytePtr+1] = (unsigned char)(temp[1] * 255.0f);
			data[bytePtr+2] = (unsigned char)(temp[2] * 255.0f);

			bytePtr+=3;
		}
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	bytePtr = 0;
	for(unsigned int j=0; j<size; j++)
	{
		for(unsigned int i=0; i<size; i++)
		{
			temp[0] = i+offset-halfSize;
			temp[1] = (j+offset-halfSize);
			temp[2] = halfSize;
			temp.normalize();

			data[bytePtr] = (unsigned char)(temp[0] * 255.0f);
			data[bytePtr+1] = (unsigned char)(temp[1] * 255.0f);
			data[bytePtr+2] = (unsigned char)(temp[2] * 255.0f);

			bytePtr+=3;
		}
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
			0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	bytePtr = 0;
	for(unsigned int j=0; j<size; j++)
	{
		for(unsigned int i=0; i<size; i++)
		{
			temp[0] = -(i+offset-halfSize);
			temp[1] = (j+offset-halfSize);
			temp[2] = -halfSize;
			temp.normalize();

			data[bytePtr] = (unsigned char)(temp[0] * 255.0f);
			data[bytePtr+1] = (unsigned char)(temp[1] * 255.0f);
			data[bytePtr+2] = (unsigned char)(temp[2] * 255.0f);

			bytePtr+=3;
		}
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
			0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	delete [] data;

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return true;
}

void Viewer::toggleTexture()
{
	loadTexture = !loadTexture;

//	if (!loadTexture)
//		glDisable(GL_TEXTURE_2D);		
}

void Viewer::toggleBumpMapping()
{
	loadBumpMapping = !loadBumpMapping;	
}

void Viewer::toggleTranslucency()
{
	transluceny = !transluceny;
}

void Viewer::pauseGame()
{
	if (tickTimer)
		tickTimer.disconnect();
	else
		tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
}

void Viewer::toggleMoveLightSource()
{
	moveLightSource = !moveLightSource;
}

void Viewer::toggleMotionBlur()
{
	motionBlur = !motionBlur;
}
void Viewer::readFile(char *filename)
{
	double xPos, yPos, zPos;
	int shapeType, numKeyFrames;
	int numIntermediaryFrames, loop;
	ifstream partsFile(filename);
	std::string name;
	if (partsFile.is_open() )
	{
		while (! partsFile.eof() )
		{
			float *col = (float *)malloc(sizeof(float) * 3);
			partsFile >> shapeType;
			partsFile >> numKeyFrames;
			partsFile >> loop;
			partsFile >> col[0];
			partsFile >> col[1];
			partsFile >> col[2];
			getline (partsFile, name);
			for (int i = 0;i<numKeyFrames;i++)
			{
				float *scaleFactor = (float *)malloc(sizeof(float) * 3);
				float *rotateFactor = (float *)malloc(sizeof(float) * 3);
				partsFile >> xPos;
				partsFile >> yPos;
				partsFile >> zPos;
				
				partsFile >> scaleFactor[0];
				partsFile >> scaleFactor[1];
				partsFile >> scaleFactor[2];
				
				partsFile >> rotateFactor[0];
				partsFile >> rotateFactor[1];
				partsFile >> rotateFactor[2];
				
				partsFile >> numIntermediaryFrames;
				Animatable anim;				
				if (i > 0)
				{

					Point3D finalPos(xPos, yPos, zPos);
					Point3D startPos = animatables.back().frames.back();
					float *startScale = animatables.back().scales.back();
					float *startRotate = animatables.back().rotates.back();
					
					float nFrac = 1.f/numIntermediaryFrames;
					Vector3D diff = finalPos - startPos;
					for (int j = 0;j<=numIntermediaryFrames;j++)
					{
						animatables.back().frames.push_back(startPos + j * nFrac * diff );
						
						float *diffScale = (float *)malloc(sizeof(float) * 3);
						float *diffRotate = (float *)malloc(sizeof(float) * 3);
						diffScale[0] = startScale[0] + j * nFrac * (scaleFactor[0] - startScale[0]);
						diffScale[1] = startScale[1] + j * nFrac * (scaleFactor[1] - startScale[1]);
						diffScale[2] = startScale[2] + j * nFrac * (scaleFactor[2] - startScale[2]);
						animatables.back().scales.push_back(diffScale);
						
						
						diffRotate[0] = startRotate[0] + j * nFrac * (rotateFactor[0] - startRotate[0]);
						diffRotate[1] = startRotate[1] + j * nFrac * (rotateFactor[1] - startRotate[1]);
						diffRotate[2] = startRotate[2] + j * nFrac * (rotateFactor[2] - startRotate[2]);

						animatables.back().rotates.push_back(diffRotate);
					}
				}
				else
				{
					anim.frames.push_back(Point3D(xPos, yPos, zPos));
					anim.scales.push_back(scaleFactor);
					anim.rotates.push_back(rotateFactor);
					anim.shapeType = shapeType;
					if (loop)
						anim.loop = true;
					else
						anim.loop = false;
					anim.col = col;
					animatables.push_back(anim);
				}
				
				getline (partsFile, name);
			}
			getline (partsFile, name);
		}
	}
}

void Viewer::toggleSound()
{
	disableSound = !disableSound;
	if (disableSound)
	{
		sm.StopSound(introMusic);
		sm.StopSound(backgroundMusic);
	}
	else if (loadScreen)
		sm.PlaySound(introMusic, -1);
	else
		sm.PlaySound(backgroundMusic, 1);
}