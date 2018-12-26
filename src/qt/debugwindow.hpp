#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include <QWidget>
#include "../core/emulator.hpp"

class DebugWindow : public QWidget
{
    Q_OBJECT
    private:
        Emulator* e;

        uint32_t cursor;
    public:
        explicit DebugWindow(QWidget *parent = nullptr);

        void set_e(Emulator* e);
        void set_cursor(uint32_t c);

        void add_instr_breakpoint_ee(uint32_t addr);
        void refresh();
};

#endif // DEBUGWINDOW_HPP
