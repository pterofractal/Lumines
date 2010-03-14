#include "appwindow.hpp"
#include <gdk/gdkkeysyms.h>
#include <iostream>

AppWindow::AppWindow()
{
	set_title("488 Tetrominoes on the Wall");

	// A utility class for constructing things that go into menus, which
	// we'll set up next.
	using Gtk::Menu_Helpers::MenuElem;
	using Gtk::Menu_Helpers::RadioMenuElem;
	using Gtk::Menu_Helpers::CheckMenuElem;

	// Slots to connect to functions
	sigc::slot1<void, Viewer::DrawMode> draw_slot = sigc::mem_fun(m_viewer, &Viewer::setDrawMode);
	sigc::slot1<void, Viewer::Speed> speed_slot = sigc::mem_fun(m_viewer, &Viewer::setSpeed);
	sigc::slot0<void> buffer_slot = sigc::mem_fun(m_viewer, &Viewer::toggleBuffer);

	// Set up the application menu
	// The slot we use here just causes AppWindow::hide() on this,
	// which shuts down the application.
	m_menu_app.items().push_back(MenuElem("_New Game", Gtk::AccelKey("n"), sigc::mem_fun(m_viewer, &Viewer::newGame ) ) );
	m_menu_app.items().push_back(MenuElem("_Reset", Gtk::AccelKey("r"), sigc::mem_fun(m_viewer, &Viewer::resetView ) ) );
	m_menu_app.items().push_back(MenuElem("_Quit", Gtk::AccelKey("q"),
		sigc::mem_fun(*this, &AppWindow::hide)));
	
	m_menu_drawMode.items().push_back(MenuElem("_Wire-Frame", Gtk::AccelKey("w"), sigc::bind( draw_slot, Viewer::WIRE ) ) );
	m_menu_drawMode.items().push_back(MenuElem("_Face", Gtk::AccelKey("f"), sigc::bind( draw_slot, Viewer::FACE ) ) );
	m_menu_drawMode.items().push_back(MenuElem("_Multicoloured", Gtk::AccelKey("m"), sigc::bind( draw_slot, Viewer::MULTICOLOURED ) ) );

	m_menu_speed.items().push_back(RadioMenuElem(m_group_speed, "_Slow", sigc::bind( speed_slot, Viewer::SLOW ) ) );
	m_menu_speed.items().push_back(RadioMenuElem(m_group_speed, "_Medium", sigc::bind( speed_slot, Viewer::MEDIUM ) ) );
	m_menu_speed.items().push_back(RadioMenuElem(m_group_speed, "_Fast", sigc::bind( speed_slot, Viewer::FAST ) ) );

	m_menu_buffer.items().push_back(CheckMenuElem("_Double Buffer", Gtk::AccelKey("b"), buffer_slot ));
	
	// Set up the menu bar
	m_menubar.items().push_back(Gtk::Menu_Helpers::MenuElem("_File", m_menu_app));
	m_menubar.items().push_back(Gtk::Menu_Helpers::MenuElem("_Draw Mode", m_menu_drawMode));
	m_menubar.items().push_back(Gtk::Menu_Helpers::MenuElem("_Speed", m_menu_speed));
	m_menubar.items().push_back(Gtk::Menu_Helpers::MenuElem("_Buffer", m_menu_buffer));	
	
	// Set up the score label	
	scoreLabel.set_text("Score:\t0");
	linesClearedLabel.set_text("Lines Cleared:\t0");
	
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
	m_viewer.set_size_request(600, 900);
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


