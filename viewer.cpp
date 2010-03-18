#include "viewer.hpp"
#include <iostream>
#include <sstream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <assert.h>
#include "appwindow.hpp"

#define DEFAULT_GAME_SPEED 50
#define WIDTH	16
#define HEIGHT 	10

Viewer::Viewer()
{
	
	// Set all rotationAngles to 0
	rotationAngleX = 0;
	rotationAngleY = 0;
	rotationAngleZ = 0;
	rotationSpeed = 0;
	
	// Default draw mode is multicoloured
	currentDrawMode = Viewer::MULTICOLOURED;

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
	
	// 
	loadScreen = false;
	
	activeTextureId = 0;
	loadTexture = false;
	
	// Game starts at a slow pace of 500ms
	gameSpeed = 500;
	
	// By default turn double buffer on
	doubleBuffer = false;
	
	gameOver = false;

	Glib::RefPtr<Gdk::GL::Config> glconfig;
	
	// Ask for an OpenGL Setup with
	//  - red, green and blue component colour
	//  - a depth buffer to avoid things overlapping wrongly
	//  - double-buffered rendering to avoid tearing/flickering
	//	- Multisample rendering to smooth out edges
	glconfig = Gdk::GL::Config::create(	Gdk::GL::MODE_RGB |
										Gdk::GL::MODE_DEPTH |
										Gdk::GL::MODE_DOUBLE );
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
	
	LoadGLTextures("256.bmp");
		
	// Just enable depth testing and set the background colour.
	glEnable(GL_DEPTH_TEST);

/*	// Basic line anti aliasing
	// Blendfunc is set wrong though. Need to figure out what is good
	glEnable (GL_LINE_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_COLOR, GL_DST_COLOR);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth (1.0);
*/
	glClearColor(0.7, 0.7, 1.0, 0.0);
	
	gldrawable->gl_end();
}

bool Viewer::on_expose_event(GdkEventExpose* event)
{
	Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
	
	if (!gldrawable) return false;

	if (!gldrawable->gl_begin(get_gl_context()))
		return false;

	if (loadTexture)
		glEnable(GL_TEXTURE_2D);
	// Decide which buffer to write to
	if (doubleBuffer)
		glDrawBuffer(GL_BACK);	
	else
		glDrawBuffer(GL_FRONT);
		
			
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	glEnable(GL_LIGHTING);
	
	// Create one light source
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Define properties of light 
	float ambientLight0[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float diffuseLight0[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specularLight0[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	float position0[] = { 5.0f, 0.0f, 0.0f, 1.0f };	
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	
	// Scale and rotate the scene
	
	if (scaleFactor != 1)
		glScaled(scaleFactor, scaleFactor, scaleFactor);
		
	if (rotationAngleX != 0)
		glRotated(rotationAngleX, 1, 0, 0);
	
	if (rotationAngleY != 0)
		glRotated(rotationAngleY, 0, 1, 0);

	if (rotationAngleZ != 0)
		glRotated(rotationAngleZ, 0, 0, 1);
	
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
	glTranslated(-5.0, -12.0, 0.0);
	
	// Create one light source
/*	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Define properties of light 
	float ambientLight0[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float diffuseLight0[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float specularLight0[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	float position0[] = { 20.0f, 5.0f, 2.0f, 1.0f };	
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);*/
	
	if (loadScreen)
	{
		draw_start_screen(false);
		
		// We pushed a matrix onto the PROJECTION stack earlier, we 
		// need to pop it.

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		// Swap the contents of the front and back buffers so we see what we
		// just drew. This should only be done if double buffering is enabled.
		if (doubleBuffer)
			gldrawable->swap_buffers();
		else
			glFlush();	

		gldrawable->gl_end();

		return true;
	}
	
	glBegin(GL_LINE_LOOP);
		glVertex3d(game->getClearBarPos(), 0, 0);
		glVertex3d(game->getClearBarPos(), 0, 1);
		glVertex3d(game->getClearBarPos(), HEIGHT, 1);
		glVertex3d(game->getClearBarPos(), HEIGHT, 0);
	glEnd();
	

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
	
	// Draw current state of Lumines
	if (currentDrawMode == Viewer::WIRE)
	{
		for (int i = HEIGHT + 3;i>=0;i--) // row
		{
			for (int j = WIDTH - 1; j>=0;j--) // column
			{
				drawCube (i, j, game->get(i, j), GL_LINE_LOOP );
			}
		}
	}
	else if (currentDrawMode == Viewer::MULTICOLOURED)
	{
		for (int i = HEIGHT + 3;i>=0;i--) // row
		{
			for (int j = WIDTH - 1; j>=0;j--) // column
			{	
				// Draw outline for cube
				if (game->get(i, j) != -1)
					drawCube(i, j, 7, GL_LINE_LOOP);
					
				drawCube (i, j, game->get(i, j), GL_QUADS, true );
			}
		}
	}
	else if (currentDrawMode == Viewer::FACE)
	{
		for (int i = HEIGHT+3;i>=0;i--) // row
		{
			for (int j = WIDTH - 1; j>=0;j--) // column
			{				
				// Draw outline for cube
				if (game->get(i, j) != -1)
					drawCube(i, j, 7, GL_LINE_LOOP);
					
				drawCube (i, j, game->get(i, j), GL_QUADS );
			}
		}	
	}	
	
	if (gameOver)
	{
		// Some game over animation
	}

 	// We pushed a matrix onto the PROJECTION stack earlier, we 
	// need to pop it.

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Swap the contents of the front and back buffers so we see what we
	// just drew. This should only be done if double buffering is enabled.
	if (doubleBuffer)
		gldrawable->swap_buffers();
	else
		glFlush();	
		
	gldrawable->gl_end();

	return true;
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
			draw_start_screen(true);
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
			loadScreen = false;
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

	if (!shiftIsDown)
	{
		// Set the rotation speed based on how far the cursor has moved since the mouse down event
		long difference = (long)timeOfLastMotionEvent - (long)event->time;
		if (difference > -50)
			rotationSpeed = (event->x - mouseDownPos[0]) / 10;	
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
		else if (scaleFactor > 2)
			scaleFactor = 2;
			
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

void Viewer::drawCube(int y, int x, int colourId, GLenum mode, bool multiColour)
{
	if (mode == GL_LINE_LOOP)
		glLineWidth (2);
	
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
	
	 
	glNormal3d(1, 0, 0);
	
	if (loadTexture)
		glBindTexture(GL_TEXTURE_2D, texture[activeTextureId]);

	glColor3d(r, g, b);
			
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
	glNormal3d(0, 0, 1);

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
	glNormal3d(0, 1, 0);

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
	glNormal3d(1, 0, 0);

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

void Viewer::draw_start_screen(bool pick)
{
	if (pick)
		glPushName(1);
	
/*		for (int i = HEIGHT + 3;i>=0;i--) // row
		{
			for (int j = WIDTH - 1; j>=0;j--) // column
			*/
//				glEnable(GL_POLYGON_SMOOTH); 
	drawCube (3, 0, 7, GL_LINE_LOOP );
	drawCube (4, 0, 7, GL_LINE_LOOP );
	drawCube (5, 0, 7, GL_LINE_LOOP );
	drawCube (6, 0, 7, GL_LINE_LOOP );
	drawCube (7, 0, 7, GL_LINE_LOOP );
	drawCube (7, 1, 7, GL_LINE_LOOP );
	drawCube (7, 2, 7, GL_LINE_LOOP );
	drawCube (5, 1, 7, GL_LINE_LOOP );
	drawCube (5, 2, 7, GL_LINE_LOOP );
	drawCube (6, 2, 7, GL_LINE_LOOP );
				
	drawCube (3, 0, 3, GL_QUADS );
	drawCube (4, 0, 3, GL_QUADS );
	drawCube (5, 0, 3, GL_QUADS );
	drawCube (6, 0, 3, GL_QUADS );
	drawCube (7, 0, 3, GL_QUADS );
	drawCube (7, 1, 3, GL_QUADS );
	drawCube (7, 2, 3, GL_QUADS );
	drawCube (5, 1, 3, GL_QUADS );
	drawCube (5, 2, 3, GL_QUADS );
	drawCube (6, 2, 3, GL_QUADS );
	
	
	drawCube(3, 4, 7, GL_LINE_LOOP);
	drawCube(4, 4, 7, GL_LINE_LOOP);
	drawCube(5, 4, 7, GL_LINE_LOOP);
	drawCube(6, 4, 7, GL_LINE_LOOP);
	drawCube(7, 4, 7, GL_LINE_LOOP);
	drawCube(3, 5, 7, GL_LINE_LOOP);
	drawCube(3, 6, 7, GL_LINE_LOOP);
	
	drawCube(3, 4, 4, GL_QUADS);
	drawCube(4, 4, 4, GL_QUADS);
	drawCube(5, 4, 4, GL_QUADS);
	drawCube(6, 4, 4, GL_QUADS);
	drawCube(7, 4, 4, GL_QUADS);
	drawCube(3, 5, 4, GL_QUADS);
	drawCube(3, 6, 4, GL_QUADS);
	
	drawCube(3, 8, 7, GL_LINE_LOOP);
	drawCube(4, 8, 7, GL_LINE_LOOP);
	drawCube(5, 8, 7, GL_LINE_LOOP);
	drawCube(6, 8, 7, GL_LINE_LOOP);
	drawCube(7, 9, 7, GL_LINE_LOOP);
	drawCube(4, 9,  7, GL_LINE_LOOP);
	drawCube(3, 10, 7, GL_LINE_LOOP);
	drawCube(4, 10, 7, GL_LINE_LOOP);
	drawCube(5, 10, 7, GL_LINE_LOOP);
	drawCube(6, 10, 7, GL_LINE_LOOP);
	
	drawCube(3, 8, 5, GL_QUADS);
	drawCube(4, 8, 5, GL_QUADS);
	drawCube(5, 8, 5, GL_QUADS);
	drawCube(6, 8, 5, GL_QUADS);
	drawCube(7, 9, 5, GL_QUADS);
	drawCube(4, 9, 5, GL_QUADS);
	drawCube(3, 10, 5,GL_QUADS);
	drawCube(4, 10, 5,GL_QUADS);
	drawCube(5, 10, 5,GL_QUADS);
	drawCube(6, 10, 5,GL_QUADS);
	
	drawCube(3, 13, 7, GL_LINE_LOOP);
	drawCube(4, 13, 7, GL_LINE_LOOP);
	drawCube(5, 13, 7, GL_LINE_LOOP);
	drawCube(6, 12, 7, GL_LINE_LOOP);
	drawCube(7, 12, 7, GL_LINE_LOOP);
	drawCube(6, 14, 7, GL_LINE_LOOP);
	drawCube(7, 14, 7, GL_LINE_LOOP);
	
	drawCube(3, 13, 6, GL_QUADS);
	drawCube(4, 13, 6, GL_QUADS);
	drawCube(5, 13, 6, GL_QUADS);
	drawCube(6, 12, 6, GL_QUADS);
	drawCube(7, 12, 6, GL_QUADS);
	drawCube(6, 14, 6, GL_QUADS);
	drawCube(7, 14, 6, GL_QUADS);
//		glDisable(GL_MULTISAMPLE_ARB);
	
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
void  Viewer::LoadGLTextures(char *filename) {	
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
    glGenTextures(1, &texture[0]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);   // 2d texture (x and y size)

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture

    // 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, 
    // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);
}

void Viewer::toggleTexture()
{
	loadTexture = !loadTexture;
	
	if (!loadTexture)
		glDisable(GL_TEXTURE_2D);
}