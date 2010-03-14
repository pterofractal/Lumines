#ifndef CS488_VIEWER_HPP
#define CS488_VIEWER_HPP

#include "algebra.hpp"
#include <gtkmm.h>
#include <gtkglmm.h>
#include "game.hpp"

// The "main" OpenGL widget
class Viewer : public Gtk::GL::DrawingArea {
public:
	Viewer();
	virtual ~Viewer();
	enum DrawMode {
		WIRE,
		FACE,
		MULTICOLOURED
	};
	
	enum Speed {
		SLOW,
		MEDIUM,
		FAST
	};
	
	// A useful function that forces this widget to rerender. If you
	// want to render a new frame, do not call on_expose_event
	// directly. Instead call this, which will cause an on_expose_event
	// call when the time is right.
	void invalidate();
	void setDrawMode(DrawMode newDrawMode);
	
	void setRotationAngle (int angle);
	int getRotationAngle();
	
	void startScale();
	void endScale();
	
	void setSpeed(Speed newSpeed);
	
	void toggleBuffer();

	bool gameTick();
		
	virtual bool on_key_press_event( GdkEventKey *ev );
		
	void resetView();
	void newGame();
	
	void makeRasterFont();
	void printString(const char *s);
	
	void setScoreWidgets(Gtk::Label *score, Gtk::Label *linesCleared);

protected:

	// Events we implement
	// Note that we could use gtkmm's "signals and slots" mechanism
	// instead, but for many classes there's a convenient member
	// function one just needs to define that'll be called with the
	// event.

	// Called when GL is first initialized
	virtual void on_realize();
	// Called when our window needs to be redrawn
	virtual bool on_expose_event(GdkEventExpose* event);
	// Called when the window is resized
	virtual bool on_configure_event(GdkEventConfigure* event);
	// Called when a mouse button is pressed
	virtual bool on_button_press_event(GdkEventButton* event);
	// Called when a mouse button is released
	virtual bool on_button_release_event(GdkEventButton* event);
	// Called when the mouse moves
	virtual bool on_motion_notify_event(GdkEventMotion* event);
	


private:

	void drawCube(int y, int x, int colourId, GLenum mode, bool multiColour = false);
	
	DrawMode currentDrawMode;
	
	// The angle at which we are currently rotated
	double rotationAngleX, rotationAngleY, rotationAngleZ;

	// How fast we are rotating the window
	double rotationSpeed;
	
	// Flags to denote which mouse buttons are down
	bool mouseB1Down, mouseB2Down, mouseB3Down;
	
	// Flags to denote which axis to rotate around after mouse up event
	bool rotateAboutX, rotateAboutY, rotateAboutZ;
	
	// What factor we are scaling the game by currently
	double scaleFactor;
	
	
	Point2D startPos, mouseDownPos, startScalePos, endScalePos;
	
	// Flag used to denote that the shift key is held down
	bool shiftIsDown;
		
	// Flag that determines when to use doubleBuffer
	bool doubleBuffer;
	
	// Timer used to call the tick method
	sigc::connection tickTimer;
	
	// Timer for rotate
	sigc::connection rotateTimer;

	// Timer for persistant rotations
	guint32 timeOfLastMotionEvent;
	
	// Used to keep track of when to increase game speed
	int linesLeftTillNextLevel;
	
	// The number of milliseconds before the next game tick
	int gameSpeed;
	
	// Speed value set by menu
	Speed speed;
	
	// Pointer to the actual game
	Game *game;
	
	// Game over flag
	bool gameOver;
	
	// Lighting flag
	bool lightingFlag;
		
	// Label widgets
	Gtk::Label *scoreLabel, *linesClearedLabel;
};

#endif
