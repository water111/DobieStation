#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include <QWidget>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>

#include "../core/emulator.hpp"

class DebugWindow : public QWidget
{
    Q_OBJECT
    private:
        Emulator* e;

        bool emulation_running;

        QGroupBox* mem_box;
        QGroupBox* button_box;
        QPlainTextEdit* mem_view;
        QTableWidget* disasm_view;
        QPushButton* step_button;
        QPushButton* toggle_run_button;

        uint32_t cursor;

        void update_toggle_run_button();
        void update_disassembly(bool scroll_to_center);
    public:
        explicit DebugWindow(QWidget *parent = nullptr);

        void set_e(Emulator* e);
        void set_cursor(uint32_t c);
        void set_run_status(bool run);

        void refresh();

        void keyPressEvent(QKeyEvent *event) override;
    signals:
        void toggle_emulation_run(bool);
    public slots:
        void step();
        void toggle_run();
        void instr_breakpoint_toggle(int row, int column);
};

#endif // DEBUGWINDOW_HPP
