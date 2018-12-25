#include "debugwindow.hpp"

DebugWindow::DebugWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Debugger");
    resize(800, 600);
}
