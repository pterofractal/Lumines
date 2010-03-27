#ifndef CS488_VIEWER_HPP
#define CS488_VIEWER_HPP

#include "algebra.hpp"
#include <gtkmm.h>
#include <gtkglmm.h>
#include "game.hpp"
#include "SoundManager.hpp"
#include <map>
#include <vector>
#include "particle.hpp"
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
	void toggleTexture();
	void toggleBumpMapping();
	void toggleTranslucency();
	void toggleMoveLightSource();
	void toggleMotionBlur();
	void makeRasterFont();
	void printString(const char *s);
	
	void setScoreWidgets(Gtk::Label *score, Gtk::Label *linesCleared);
	bool moveClearBar();
	void pauseGame();
	void addParticleBox(float x, float y, int colour);
	void addFireworks(float x, float y);


	// Texture mapping stuff	
	/* storage for one texture  */
	int numTextures;
	GLuint *texture;

	/* Image type - contains height, width, and data */
	struct Image {
	    unsigned long sizeX;
	    unsigned long sizeY;
	    char *data;
	};
	typedef struct Image Image;
	
	int ImageLoad(const char *filename, Image *image);
	int LoadGLTextures(const char *filename, GLuint &texid);

	// Bump mapping stuff	
	int GenNormalizationCubeMap(unsigned int size, GLuint &texid);

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
	void drawScene(bool draw3D = true);
	void drawBar();
	void drawFallingBox();
	void drawFloor();
	void drawShadowVolumes();
	void drawShadowCube(float y, float x, GLenum mode);
	void drawRoom();
	void drawStartScreen(bool picking, GLuint texId);
	void drawParticles(bool step = true);
	void drawGrid();
	void drawReflections();
	void drawMoveBlur(int side); // 0 = right | 1 = left
	void drawBackground();
	
	void drawCube(float y, float x, int colourId, GLenum mode, bool draw3D = true);
	void drawBumpCube(float y, float x, int colourId, GLenum mode, bool draw3D = true);
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
	sigc::connection tickTimer, clearBarTimer;
	
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
	
	// Boot screen
	bool loadScreen;
	
	int activeTextureId;
	
	bool loadTexture;
	bool loadBumpMapping;
	bool transluceny;
	SoundManager sm;
	int backgroundMusic;
	int turnSound;
	int moveSound;
	GLuint square;
	float shadowProj[16];
	float lightPos[4];
	
	float planeNormal[4];
	bool drawingShadow;
	GLuint cube, bumpMap, floorTexId, playButtonTex, playButtonClickedTex, backgroundTex;
	bool clickedButton;
	std::vector< std::pair<Point3D, Point3D> > silhouette;
	std::vector< Particle *> particles;
	bool moveLeft;
	bool moveRight;
	bool moveLightSource;
	bool motionBlur;
	bool levelUpAnimation;
};

#endif
