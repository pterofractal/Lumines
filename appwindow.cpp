#include "appwindow.hpp"
#include <gdk/gdkkeysyms.h>
#include <iostream>

AppWindow::AppWindow()
{
	set_title("Lumines");

	// A utility class for constructing things that go into menus, which
	// we'll set up next.
	using Gtk::Menu_Helpers::MenuElem;
	using Gtk::Menu_Helpers::RadioMenuElem;
	using Gtk::Menu_Helpers::CheckMenuElem;

	// Slots to connect to functions
	sigc::slot1<void, Viewer::DrawMode> draw_slot = sigc::mem_fun(m_viewer, &Viewer::setDrawMode);
	sigc::slot0<void> sound_slot = sigc::mem_fun(m_viewer, &Viewer::toggleSound);

	// Set up the application menu
	// The slot we use here just causes AppWindow::hide() on this,
	// which shuts down the application.
	m_menu_app.items().push_back(MenuElem("_New Game", Gtk::AccelKey("n"), sigc::mem_fun(m_viewer, &Viewer::newGame ) ) );
	m_menu_app.items().push_back(MenuElem("_Reset", Gtk::AccelKey("r"), sigc::mem_fun(m_viewer, &Viewer::resetView ) ) );
	m_menu_app.items().push_back(MenuElem("_Pause", Gtk::AccelKey("p"), sigc::mem_fun(m_viewer, &Viewer::pauseGame ) ) );
	m_menu_app.items().push_back(MenuElem("_Quit", Gtk::AccelKey("q"),
		sigc::mem_fun(*this, &AppWindow::hide)));
	
	m_menu_drawMode.items().push_back(MenuElem("_Textures", Gtk::AccelKey("t"), sigc::mem_fun(m_viewer, &Viewer::toggleTexture ) ) );
	m_menu_drawMode.items().push_back(MenuElem("_Bump Mapping", Gtk::AccelKey("b"), sigc::mem_fun(m_viewer, &Viewer::toggleBumpMapping ) ) );
	m_menu_drawMode.items().push_back(MenuElem("_Translucency", Gtk::AccelKey("u"), sigc::mem_fun(m_viewer, &Viewer::toggleTranslucency ) ) );
	m_menu_drawMode.items().push_back(MenuElem("_Move Light Source", Gtk::AccelKey("l"), sigc::mem_fun(m_viewer, &Viewer::toggleMoveLightSource ) ) );
	m_menu_drawMode.items().push_back(MenuElem("Motion _Blur", Gtk::AccelKey("m"), sigc::mem_fun(m_viewer, &Viewer::toggleMotionBlur ) ) );
	m_menu_drawMode.items().push_back(MenuElem("_Draw Shadow", Gtk::AccelKey("d"), sigc::mem_fun(m_viewer, &Viewer::toggleShadows ) ) );

	m_menu_drawMode.items().push_back(CheckMenuElem("_Enable Sound", Gtk::AccelKey("s"), sound_slot ));
	
	// Set up the menu bar
	m_menubar.items().push_back(Gtk::Menu_Helpers::MenuElem("_File", m_menu_app));
	m_menubar.items().push_back(Gtk::Menu_Helpers::MenuElem("_Game Settings", m_menu_drawMode));
	
	// Set up the score label	
	scoreLabel.set_text("Score:\t0");
	linesClearedLabel.set_text("Deleted:\t0");
	
	m_viewer.setScoreWidgets(&scoreLabel, &linesClearedLabel);
	
	// Pack in our widgets

	// First add the vertical box as our single "top" widget
	add(m_vbox);

	// Put the menubar on the top, and make it as small as possible
	m_vbox.pack_start(m_menubar, Gtk::PACK_SHRINK);
	m_vbox.pack_start(linesClearedLabel, Gtk::PACK_EXPAND_PADDING);
	m_vbox.pack_start(scoreLabel, Gtk::PACK_EXPAND_PADDING);

	// Put the viewer below the menubar. pack_start "grows" the widget
	// by default, so it'll take up the rest of the window.
	
	//m_viewer.set_size_request(300, 600);
	m_viewer.set_size_request(900, 600);
	m_vbox.pack_start(m_viewer);
	
	show_all();
}

bool AppWindow::on_key_press_event( GdkEventKey *ev )
{
        // This is a convenient place to handle non-shortcut
        // keys.  You'll want to look at ev->keyval.

	// An example key; delete and replace with the
	// keys you want to process
	// GDK_Left, _up, Right, Down
  	if (ev->keyval == GDK_Shift_L || ev->keyval == GDK_Shift_R)
	{
		m_viewer.startScale();
		return Gtk::Window::on_key_press_event( ev );
	}
	else
	{
		m_viewer.on_key_press_event(ev);
		return Gtk::Window::on_key_press_event( ev );
	}
}

bool AppWindow::on_key_release_event (GdkEventKey *ev)
{
	m_viewer.endScale();
	return Gtk::Window::on_key_release_event( ev );;
}

void AppWindow::updateScore(int newScore)
{
	scoreLabel.set_text("Score:\t" + newScore);
}


