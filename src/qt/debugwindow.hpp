#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include <QWidget>

class DebugWindow : public QWidget
{
    Q_OBJECT
    private:
    public:
        explicit DebugWindow(QWidget *parent = nullptr);
};

#endif // DEBUGWINDOW_HPP
