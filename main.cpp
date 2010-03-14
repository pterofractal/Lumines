#include <gtkmm.h>
#include <gtkglmm.h>
#include "appwindow.hpp"

int main(int argc, char** argv)
{
  // Construct our main loop
  Gtk::Main kit(argc, argv);

  // Initialize OpenGL
  Gtk::GL::init(argc, argv);

  // Construct our (only) window
  AppWindow window;

  // And run the application!
  Gtk::Main::run(window);
}

