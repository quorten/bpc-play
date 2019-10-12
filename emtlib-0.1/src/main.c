/* POSIX startup */

int
main (int argc, char **argv)
{
  return 0;
}

/* It can be useful to define WinMain() as a thin wrapper for main()
   if you are compiling a GTK+ graphical user interface application.
   If you are compiling an exclusive command-line application, then
   the following code is unnecessary.  */

#ifdef _MSC_VER
#include <windows.h>

int WINAPI
WinMain (HINSTANCE hInstance,
	 HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  return main (__argc, __argv);
}
#endif
