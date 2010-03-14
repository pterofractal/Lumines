#ifndef APPWINDOW_HPP
#define APPWINDOW_HPP

#include <gtkmm.h>
#include "viewer.hpp"

class AppWindow : public Gtk::Window {
public:
  AppWindow();
	void updateScore(int newScore);
	void updateLinesCleared(int linesCleared);
  
protected:
	virtual bool on_key_press_event( GdkEventKey *ev );
	virtual bool  on_key_release_event (GdkEventKey *ev);

private:
	// A "vertical box" which holds everything in our window
	Gtk::VBox m_vbox;

	// The menubar, with all the menus at the top of the window
	Gtk::MenuBar m_menubar;

	// Each menu itself
	Gtk::Menu m_menu_app;
	Gtk::Menu m_menu_drawMode;
	Gtk::Menu m_menu_buffer;
	Gtk::Menu m_menu_speed;
	Gtk::RadioButtonGroup m_group_speed;
	// The main OpenGL area
	Viewer m_viewer;
	
	// Label widgets
	Gtk::Label scoreLabel, linesClearedLabel;
};
#endif
