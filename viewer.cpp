#include "viewer.hpp"
#include <iostream>
#include <sstream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <assert.h>
#include "appwindow.hpp"

#define DEFAULT_GAME_SPEED 500
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
										Gdk::GL::MODE_DOUBLE |
										Gdk::GL::MODE_MULTISAMPLE );
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
	game = new Game(10, 20);
	
	// Start game tick timer
	tickTimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Viewer::gameTick), gameSpeed);
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
	
	// Just enable depth testing and set the background colour.
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.7, 0.7, 1.0, 0.0);
	
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Modify the current projection matrix so that we move the 
	// camera away from the origin.  We'll draw the game at the
	// origin, and we need to back up to see it.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glTranslated(0.0, 0.0, -40.0);

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
	

	
	// Draw Border
	for (int y = -1;y< 20;y++)
	{
		drawCube(y, -1, 7, GL_LINE_LOOP);
		
		drawCube(y, 10, 7, GL_LINE_LOOP);
	}
	for (int x = 0;x < 10; x++)
	{
		drawCube (-1, x, 7, GL_LINE_LOOP);
	}
	
	// Draw current state of tetris
	if (currentDrawMode == Viewer::WIRE)
	{
		for (int i = 23;i>=0;i--) // row
		{
			for (int j = 9; j>=0;j--) // column
			{
				drawCube (i, j, game->get(i, j), GL_LINE_LOOP );
			}
		}
	}
	else if (currentDrawMode == Viewer::MULTICOLOURED)
	{
		for (int i = 23;i>=0;i--) // row
		{
			for (int j = 9; j>=0;j--) // column
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
		for (int i = 23;i>=0;i--) // row
		{
			for (int j = 9; j>=0;j--) // column
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
	
	// Front face
	glNormal3d(1, 0, 0);
		
	glColor3d(r, g, b);
	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();
	
	// top face
	glNormal3d(0, 1, 0);
	
	if (multiColour)
		glColor3d(g, r, b);

	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
	glEnd();
	
	// left face
	glNormal3d(0, 0, 1);
	
	if (multiColour)
		glColor3d(b, g, r);

	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMin);
		glVertex3d(innerXMin + x, innerYMax + y, zMax);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
	glEnd();
	
	// bottom face
	glNormal3d(0, 1, 0);
	
	if (multiColour)
		glColor3d(r, b, g);	

	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
		glVertex3d(innerXMin + x, innerYMin + y, zMax);
	glEnd();
	
	// right face
	glNormal3d(0, 0, 1);
	
	if (multiColour)
		glColor3d(b, r, g);
	
	glBegin(mode);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMax);
		glVertex3d(innerXMax + x, innerYMin + y, zMax);
	glEnd();
	
	// Back of front face
	glNormal3d(1, 0, 0);

	if (multiColour)
		glColor3d(g, b, r);

	glBegin(mode);
		glVertex3d(innerXMin + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMin + y, zMin);
		glVertex3d(innerXMax + x, innerYMax + y, zMin);
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
