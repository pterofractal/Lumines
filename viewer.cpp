#include "viewer.hpp"
#include <iostream>
#include <sstream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <assert.h>
#include "appwindow.hpp"

#define DEFAULT_GAME_SPEED 50
#define WIDTH	16
#define HEIGHT 	10
int SND_ID_1 = 0;
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
	loadScreen = false;
	
	activeTextureId = 0;
	loadTexture = true;
	loadBumpMapping = false;
	transluceny = false;
	// Game starts at a slow pace of 500ms
	gameSpeed = DEFAULT_GAME_SPEED;
	
	// By default turn double buffer on
	doubleBuffer = true;
	
	gameOver = false;
	numTextures = 0;
	lightPos[0] = 8.0f;
	lightPos[1] = 15.0f;
	lightPos[2] = 5.0f;
	lightPos[3] = 1.0f;
	
	
	Glib::RefPtr<Gdk::GL::Config> glconfig;
	
	// Ask for an OpenGL Setup with
	//  - red, green and blue component colour
	//  - a depth buffer to avoid things overlapping wrongly
	//  - double-buffered rendering to avoid tearing/flickering
	//	- Multisample rendering to smooth out edges
	glconfig = Gdk::GL::Config::create(	Gdk::GL::MODE_RGB |
										Gdk::GL::MODE_DEPTH |
										Gdk::GL::MODE_DOUBLE |
										Gdk::GL::MODE_ACCUM);
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
	LoadGLTextures("red.bmp", texture[0]);
	LoadGLTextures("blue.bmp", texture[1]);
	LoadGLTextures("lightRed.bmp", texture[2]);
	LoadGLTextures("lightBlue.bmp", texture[3]);
	LoadGLTextures("black.bmp", texture[4]);
	LoadGLTextures("normal.bmp", bumpMap);
	LoadGLTextures("floor.bmp", floorTexId);
	LoadGLTextures("playButton.bmp", playButtonTex);
	LoadGLTextures("playButtonClicked.bmp", playButtonClickedTex);
	std::cout << "normal\t" << bumpMap << "\n";
	
	
	// Load music
	backgroundMusic = sm.LoadSound("lumines.ogg");
	moveSound = sm.LoadSound("move.ogg");
	turnSound = sm.LoadSound("turn.ogg");
//	sm.PlaySound(backgroundMusic);


	GenNormalizationCubeMap(256, normalization_cube_map);
	// Just enable depth testing and set the background colour.
	//	glEnable(GL_DEPTH_TEST);
	
	
	
	
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
	glLineWidth (1.0);
*/
	glClearColor(0.7, 0.7, 1.0, 0.0);
	
	
	square = glGenLists(1);
	glNewList(square, GL_COMPILE);
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
	
	// Create one light source
	// Define properties of light 
/*	float ambientLight0[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float diffuseLight0[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specularLight0[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	float position0[] = { 5.0f, 0.0f, 0.0f, 1.0f };	
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	*/
	
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
	glTranslated(-5.0, -10.0, 10.0);
	
	// Create one light source
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Define properties of light 
	float ambientLight0[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float diffuseLight0[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specularLight0[] = { 0.6f, 0.6f, 0.6f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	
	if (loadScreen)
	{
		drawStartScreen(false, playButtonTex);
		
		// We pushed a matrix onto the PROJECTION stack earlier, we 
		// need to pop it.

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		// Swap the contents of the front and back buffers so we see what we
		// just drew. This should only be done if double buffering is enabled.
		if (doubleBuffer)
			gldrawable->swap_buffers();
/*		else
			glFlush();	*/

		gldrawable->gl_end();

		return true;
	}

/*	// Draw game
	drawFloor();
	drawScene(0);
//	
	
	
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f, 100.0f);

	glCullFace(GL_FRONT);
	glStencilFunc(GL_ALWAYS, 0x0, 0xff);
	glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
	drawShadowVolumes();

	glCullFace(GL_BACK);
	glStencilFunc(GL_ALWAYS, 0x0, 0xff);
	glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
	drawShadowVolumes();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
/*		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 1, 1, 0, 0, 1);
		glDisable(GL_DEPTH_TEST);

		glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
		glBegin(GL_QUADS);
			glVertex2i(0, 0);
			glVertex2i(0, 1);
			glVertex2i(1, 1);
			glVertex2i(1, 0);
		glEnd();

		glEnable(GL_DEPTH_TEST);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	glDisable(GL_STENCIL_TEST);
	
	drawFloor();*/
		
/*	glColor4f(0.7f, 0.4f, 0.0f, 1.0f);				// Set Color To An Orange
	glDisable(GL_LIGHTING);						// Disable Lighting
	glDepthMask(GL_FALSE);						// Disable Depth Mask
	glTranslatef(lightPos[0], lightPos[1], lightPos[2]);				// Translate To Light's Position
	
	GLUquadricObj *quadratic;	
	quadratic=gluNewQuadric();								// Notice We're Still In Local Coordinate System
	gluSphere(quadratic, 0.2f, 16, 8);					// Draw A Little Yellow Sphere (Represents Light)
//	glEnable(GL_LIGHTING);						// Enable Lighting
	glDepthMask(GL_TRUE);						// Enable Depth Mask*/

	
	GLUquadricObj *quadratic;	
	quadratic=gluNewQuadric();			// Create A Pointer To The Quadric Object ( NEW )
	gluQuadricNormals(quadratic, GLU_SMOOTH);	// Create Smooth Normals ( NEW )
	gluQuadricTexture(quadratic, GL_TRUE);
	glPushMatrix();
		glTranslatef(lightPos[0], lightPos[1], lightPos[2]);
		gluSphere(quadratic,1.3f,32,32);
	glPopMatrix();
	
	
	

//	drawShadowVolumes();	
//	silhouette.clear();

//	drawRoom();
	
	//drawScene(0);
	drawBar();
	
	if (game->counter == 15)
	{
		drawScene(0);
		drawFallingBox();		
	
		glAccum(GL_RETURN, 1.f);
	}
	else
		drawScene(0);
	

/*			for (int i = 16;i>=1;i--)
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//	drawBar(i);
				drawScene(i);
				glAccum(GL_ACCUM, 1.f/16);
			}
			glAccum(GL_RETURN, 1.f);	*/
	
		
 	// We pushed a matrix onto the PROJECTION stack earlier, we 
	// need to pop it.

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Swap the contents of the front and back buffers so we see what we
	// just drew. This should only be done if double buffering is enabled.
	if (doubleBuffer)
		gldrawable->swap_buffers();
/*	else
		glFlush();	*/
		
	gldrawable->gl_end();

	return true;
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
	float frac = 1.f/16;
	for (int i = HEIGHT+3;i>=0;i--) // row
	{
		for (int j = WIDTH - 1; j>=0;j--) // column
		{				
			
			bool condition = game->get(i, j) != -1 && i > 1	&&(	game->get(i - 2, j) == -1 && i == game->py_ - 1 && j == game->px_ + 1 
							 || 	game->get(i - 2, j) == -1 && i == game->py_ - 1 && j == game->px_ + 2);
			condition = condition || (i > 0 && game->get(i - 1, j) == -1 && i == game->py_ - 2 && j == game->px_ + 1 ||	game->get(i - 1, j) == -1 && i == game->py_ - 2 && j == game->px_ + 2);
			if (condition)
			{			
				drawShadowCube (i - game->counter * frac, j, GL_LINE_LOOP );
			
			}
			else if (game->get(i, j) != -1)
			{
				drawShadowCube (i, j, GL_QUADS );
				
			}
				

			// Draw outline for cube
		}
	}
	
	
	Vector3D temp, temp2;
	Point3D temp3;
	Point3D lightPoint(lightPos[0], lightPos[1], lightPos[2]);
	glColor4d(0, 0, 0, 0.3);
		glBegin(GL_QUADS);			
	for (int i = 0;i<silhouette.size();i++)
	{
		
		temp =  silhouette[i].first - lightPoint;
		temp2 = silhouette[i].second - lightPoint;
		temp3 = silhouette[i].first +  100*temp;
		

			glVertex3d(silhouette[i].first[0], silhouette[i].first[1], silhouette[i].second[2]);
			glVertex3d(temp3[0], temp3[1], temp3[2]);
			temp3 = silhouette[i].second + 100*temp2;	
			glVertex3d(temp3[0], temp3[1], temp3[2]);
			glVertex3d(silhouette[i].second[0], silhouette[i].second[1], silhouette[i].second[2]);
			glVertex3d(silhouette[i].first[0], silhouette[i].first[1], silhouette[i].second[2]);
	}
			glEnd();
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
	
/*	if (Vector3D(0, 1, 0).dot(lightVec) > 0)
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
	*/
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
	for (int i = 0;i<silhouette.size();i++)
	{
		for (int j = i+1;j<silhouette.size();j++)
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
		glBegin(GL_LINE_LOOP);
			glVertex3d(game->getClearBarPos(), 0, 0);
			glVertex3d(game->getClearBarPos(), 0, 1);
			glVertex3d(game->getClearBarPos(), HEIGHT, 1);
			glVertex3d(game->getClearBarPos(), HEIGHT, 0);
		glEnd();
}

void Viewer::drawFallingBox()
{
	
	int iter = 16;
	float iterFrac = 1.f/iter;
	for (int i = 0;i<iter;i++)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawCube (game->py_ - 1 + i * iterFrac, game->px_ + 1, game->get(game->py_ - 1, game->px_ + 1), GL_QUADS );
		drawCube (game->py_ - 1 + i * iterFrac, game->px_ + 2, game->get(game->py_ - 1, game->px_ + 2), GL_QUADS );
		drawCube (game->py_ - 2 + i * iterFrac, game->px_ + 1, game->get(game->py_ - 1, game->px_ + 1), GL_QUADS );
		drawCube (game->py_ - 2 + i * iterFrac, game->px_ + 2, game->get(game->py_ - 1, game->px_ + 2), GL_QUADS );
                                         
		drawCube (game->py_ - 1 + i * iterFrac, game->px_ + 1, 7, GL_LINE_LOOP );
		drawCube (game->py_ - 1 + i * iterFrac, game->px_ + 2, 7, GL_LINE_LOOP );
		drawCube (game->py_ - 2 + i * iterFrac, game->px_ + 1, 7, GL_LINE_LOOP );
		drawCube (game->py_ - 2 + i * iterFrac, game->px_ + 2, 7, GL_LINE_LOOP );
		glAccum(GL_ACCUM, iterFrac);
	}

}

void Viewer::drawFloor()
{
			// Draw Floor
		//	glEnable(GL_TEXTURE_2D);
	//		glBindTexture(GL_TEXTURE_2D, floorTexId);
	//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
			glColor3d(1, 1, 1);
			glBegin(GL_QUADS);
			glNormal3f(0.0, 1.0, 0.0);
	//		glTexCoord2f(0.0, 0.0);
			glVertex3d(-100, -1, -100);
	//		glTexCoord2f(0.0, 10.0);
			glVertex3d(-100, -1, 100);
	//		glTexCoord2f(10.0, 10.0);
			glVertex3d(100, -1, 100);
	//		glTexCoord2f(10.0, 0.0);
			glVertex3d(100, -1, -100);
			glEnd();
	//		glBindTexture(GL_TEXTURE_2D, 0);
	//		glDisable(GL_TEXTURE_2D);	
}
void Viewer::drawScene(int itesdfr)
{		

		// Draw Border
		for (int y = -1;y< HEIGHT;y++)
		{
			drawCube(y, -1, 7, GL_LINE_LOOP);

			drawCube(y, WIDTH, 7, GL_LINE_LOOP);
		}
		for (int x = 0;x < WIDTH; x++)
		{
			drawCube (-1, x, 7, GL_LINE_LOOP);
		}
		
	


		float frac = 1.f/16;
		for (int i = HEIGHT+3;i>=0;i--) // row
		{
			for (int j = WIDTH - 1; j>=0;j--) // column
			{				
				bool condition = i > 1	&&(	game->get(i - 2, j) == -1 && i == game->py_ - 1 && j == game->px_ + 1 
										|| 	game->get(i - 2, j) == -1 && i == game->py_ - 1 && j == game->px_ + 2);
				condition = condition || (i > 0 && game->get(i - 1, j) == -1 && i == game->py_ - 2 && j == game->px_ + 1 ||	game->get(i - 1, j) == -1 && i == game->py_ - 2 && j == game->px_ + 2);
				/*if (condition)
				{				
					drawCube (i - game->counter * frac, j, game->get(i, j), GL_QUADS );
				}
				else*/ if(game->get(i, j) != -1)
					drawCube (i, j, game->get(i, j), GL_QUADS );

				// Draw outline for cube
				/*if (game->get(i, j) != -1 && condition)
				{
				
					drawCube(i - game->counter * frac, j, 7, GL_LINE_LOOP);
				}
				else */if (game->get(i, j) != -1)
					drawCube(i, j, 7, GL_LINE_LOOP);
			}
		}	
				
		if (gameOver)
		{
			// Some game over animation
		}
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
		GLuint buff[128] = {0};
	 	GLint hits, view[4];

			glSelectBuffer(128,buff);
		glRenderMode(GL_SELECT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glTranslated(-3.0, 5.0, -30.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslated(-5.0, -12.0, 0.0);
		glInitNames();
		glPushName(0);
			drawStartScreen(true, playButtonTex);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glFlush();

		// returning to normal rendering mode
		hits = glRenderMode(GL_RENDER);

		// if there are hits process them

		std::cerr << "Hits " << hits << std::endl;

		if (hits > 0)
		{
			clickedButton = true;
			playButtonTex = playButtonClickedTex;
			drawStartScreen(false, playButtonClickedTex);
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
	if (clickedButton)
	{
		loadScreen = false;
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

void Viewer::drawCube(float y, float x, int colourId, GLenum mode, bool multiColour)
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
	

		
	glBegin(mode);
	 	glTexCoord2f(0.0f, 0.0f);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
		
		glTexCoord2f(1.0f, 0.0f);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);

		glTexCoord2f(1.0f, 1.0f);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);

		glTexCoord2f(0.0f, 1.0f);	
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();

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
	
/*	if (ev->keyval == GDK_Left || ev->keyval == GDK_Right)
		sm.PlaySound(moveSound);
	else if (ev->keyval == GDK_Up || ev->keyval == GDK_Down)
		sm.PlaySound(turnSound);*/
		
		
	if (ev->keyval == GDK_Left)
		game->moveLeft();
	else if (ev->keyval == GDK_Right)
		game->moveRight();
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
		
	int returnVal = game->tick();
	
	// String streams used to print score and lines cleared	
	std::stringstream scoreStream, linesStream; 
	std::string s;
	
	// Update the score
	scoreStream << game->getScore();
	scoreLabel->set_text("Score:\t" + scoreStream.str());
	
	// If a line was cleared update the linesCleared widget
	if (returnVal > 0)
	{
    	linesStream << game->getLinesCleared();
		linesClearedLabel->set_text("Lines Cleared:\t" + linesStream.str());
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
		tickTimer.disconnect();
		std::cerr << "Boo!";
	}
	
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
	
	invalidate();
}

void Viewer::setScoreWidgets(Gtk::Label *score, Gtk::Label *linesCleared)
{
	scoreLabel = score;
	linesClearedLabel = linesCleared;
}

bool Viewer::moveClearBar()
{
	game->moveClearBar();
	invalidate();
	return true;
}

void Viewer::drawStartScreen(bool pick, GLuint texId)
{
	if (pick)
		glPushName(1);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3d(5, 3, 1);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3d(13, 3, 1);
		glTexCoord2f(1.0f, 1.0f);	
		glVertex3d(13, 7, 1);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3d(5, 7, 1);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);

	
	if (pick)
		glPopName();
}

// quick and dirty bitmap loader...for 24 bit bitmaps with 1 plane only.  
// See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/BMP.txt for more info.
int Viewer::ImageLoad(char *filename, Image *image) {
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
int  Viewer::LoadGLTextures(char *filename, GLuint &texid) {	
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

void Viewer::toggleTexture()
{
	loadTexture = !loadTexture;

//	if (!loadTexture)
//		glDisable(GL_TEXTURE_2D);		
}

void Viewer::toggleBumpMapping()
{
//	loadBumpMapping = !loadBumpMapping;
	transluceny = !transluceny;
}

void Viewer::pauseGame()
{
		tickTimer.disconnect();
}

int Viewer::GenNormalizationCubeMap(unsigned int size, GLuint &texid)
{
	glGenTextures(1, &texid);
	std::cerr << texid << "\n";
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
	for(unsigned int  j=0; j<size; j++)
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
	for(unsigned int  j=0; j<size; j++)
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
	for(unsigned int  j=0; j<size; j++)
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
