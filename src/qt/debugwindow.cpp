#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QInputDialog>

#include "debugwindow.hpp"
#include "../core/ee/emotiondisasm.hpp"

const int MAX_DISASM_INSTRS = 4096;

DebugWindow::DebugWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Debugger");
    resize(800, 600);
    e = nullptr;
    cursor = 0;

    emulation_running = true;

    disasm_view = new QTableWidget(MAX_DISASM_INSTRS, 1, this);
    disasm_view->horizontalHeader()->hide();
    disasm_view->verticalHeader()->hide();
    disasm_view->setColumnWidth(0, 400);
    disasm_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(disasm_view, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(instr_breakpoint_toggle(int,int)));

    mem_view = new QPlainTextEdit(this);
    mem_view->setReadOnly(true);

    //Set the font to the default monospaced system font
    const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    disasm_view->setFont(fixed_font);
    mem_view->setFont(fixed_font);

    step_button = new QPushButton(tr("&Step"), this);
    connect(step_button, SIGNAL(pressed()), this, SLOT(step()));

    toggle_run_button = new QPushButton(tr("&Break"), this);
    connect(toggle_run_button, SIGNAL(pressed()), this, SLOT(toggle_run()));

    mem_box = new QGroupBox();
    QVBoxLayout* mem_layout = new QVBoxLayout();
    mem_layout->addWidget(disasm_view);
    mem_layout->addWidget(mem_view);
    mem_box->setLayout(mem_layout);

    button_box = new QGroupBox();
    QVBoxLayout* button_layout = new QVBoxLayout();
    button_layout->addWidget(step_button);
    button_layout->addWidget(toggle_run_button);
    button_box->setLayout(button_layout);

    QHBoxLayout* main_layout = new QHBoxLayout();
    main_layout->addWidget(mem_box);
    main_layout->addWidget(button_box);
    setLayout(main_layout);
}

void DebugWindow::set_e(Emulator *e)
{
    this->e = e;
}

void DebugWindow::set_cursor(uint32_t c)
{
    cursor = c;
}

void DebugWindow::set_run_status(bool run)
{
    emulation_running = run;
    update_toggle_run_button();
}

void DebugWindow::refresh()
{
    DebugInfo* info = e->get_debug_info();

    cursor = info->ee->get_PC();
    update_toggle_run_button();
    update_disassembly(true);
}

void DebugWindow::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    switch (event->key())
    {
        case Qt::Key_G:
            if (event->modifiers() == Qt::ControlModifier && !emulation_running)
            {
                QString text = QInputDialog::getText(this, "Disassemble address", "Enter an address to disassemble");
                bool ok;
                uint32_t new_addr = text.toUInt(&ok, 16);
                if (ok)
                {
                    cursor = new_addr;
                    update_disassembly(true);
                }
            }
            break;
    }
}

void DebugWindow::update_toggle_run_button()
{
    if (emulation_running)
        toggle_run_button->setText("&Break");
    else
        toggle_run_button->setText("&Run");
}

void DebugWindow::update_disassembly(bool scroll_to_center)
{
    disasm_view->clear();
    DebugInfo* info = e->get_debug_info();

    for (int i = 0; i < MAX_DISASM_INSTRS * 4; i += 4)
    {
        uint32_t addr = cursor + (i - (MAX_DISASM_INSTRS * 2));
        uint32_t instr = info->ee->read32(addr);

        QString addr_str = QString("[%1] ").arg(QString::number(addr, 16), 8, '0');
        QString instr_str = QString("%1 ").arg(QString::number(instr, 16), 8, '0');
        QString dis_str = QString::fromStdString(EmotionDisasm::disasm_instr(instr, addr));

        if (addr == info->ee->get_PC())
            addr_str = "->" + addr_str;
        else
            addr_str = "  " + addr_str;

        QTableWidgetItem* text_item = new QTableWidgetItem(addr_str + instr_str + dis_str);
        //Change the color of the cell if it's a breakpoint
        for (int i = 0; i < info->ee_breakpoints.size(); i++)
        {
            if (info->ee_breakpoints[i].addr == addr)
                text_item->setBackground(Qt::red);
        }

        disasm_view->setItem(i / 4, 0, text_item);
        if (scroll_to_center && i == (MAX_DISASM_INSTRS) * 2)
            disasm_view->scrollToItem(text_item, QAbstractItemView::PositionAtCenter);
    }
}

void DebugWindow::step()
{
    e->step();
    refresh();
}

void DebugWindow::toggle_run()
{
    emulation_running = !emulation_running;
    if (emulation_running)
        refresh();
    else
        update_toggle_run_button();

    emit toggle_emulation_run(emulation_running);
}

//Used when clicking on a line of disassembly
void DebugWindow::instr_breakpoint_toggle(int row, int column)
{
    uint32_t addr = cursor + row * 4 - MAX_DISASM_INSTRS * 2;
    DebugInfo* info = e->get_debug_info();

    bool breakpoint_found = false;
    for (int i = 0; i < info->ee_breakpoints.size(); i++)
    {
        if (addr == info->ee_breakpoints[i].addr)
        {
            info->ee_breakpoints.erase(info->ee_breakpoints.begin() + i);
            breakpoint_found = true;
            break;
        }
    }

    if (!breakpoint_found)
    {
        Breakpoint_CPU breakpoint;
        breakpoint.addr = addr;
        info->ee_breakpoints.push_back(breakpoint);
    }

    update_disassembly(false);
}
