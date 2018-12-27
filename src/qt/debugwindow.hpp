#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include <QWidget>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>

#include "../core/emulator.hpp"

enum class DEBUG_SELECTION
{
    EE,
    IOP,
    VU0,
    VU1
};

class DebugWindow : public QWidget
{
    Q_OBJECT
    private:
        Emulator* e;

        DEBUG_SELECTION cpu_selection;
        bool emulation_running;

        QGroupBox* cpu_selection_box;
        QPushButton* ee_select_button;
        QPushButton* iop_select_button;
        QPushButton* vu0_select_button;
        QPushButton* vu1_select_button;

        QGroupBox* disasm_reg_box;
        QTableWidget* disasm_view;
        QTableWidget* reg_view;

        QTableWidget* mem_view;

        QGroupBox* button_box;
        QPushButton* step_button;
        QPushButton* toggle_run_button;

        uint32_t cursor;
        uint32_t ee_memory_cursor;
        uint32_t iop_memory_cursor;

        void create_cpu_selection();
        void create_tables();

        void insert_string(QTableWidget* table, int column, QString str);

        bool addr_valid(uint32_t addr);
        void update_toggle_run_button();
        void update_disassembly(bool scroll_to_center);
        void update_memory(bool scroll_to_center);
        void update_registers();
        void update_registers_ee();
        void update_registers_iop();
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
        void prompt_disasm_jump();
        void prompt_memory_jump();

        void select_ee();
        void select_iop();
};

#endif // DEBUGWINDOW_HPP
