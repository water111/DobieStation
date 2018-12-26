#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QGroupBox>

#include "../core/emulator.hpp"

class DebugWindow : public QWidget
{
    Q_OBJECT
    private:
        Emulator* e;

        QGroupBox* mem_box;
        QGroupBox* button_box;
        QPlainTextEdit* disasm_view;
        QPlainTextEdit* mem_view;
        QPushButton* step_button;

        uint32_t cursor;
    public:
        explicit DebugWindow(QWidget *parent = nullptr);

        void set_e(Emulator* e);
        void set_cursor(uint32_t c);

        void add_instr_breakpoint_ee(uint32_t addr);
        void refresh();
    public slots:
        void step();
};

#endif // DEBUGWINDOW_HPP
