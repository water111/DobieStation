#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>

#include "debugwindow.hpp"
#include "../core/ee/emotiondisasm.hpp"

DebugWindow::DebugWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Debugger");
    resize(800, 600);
    e = nullptr;

    disasm_view = new QPlainTextEdit(this);
    disasm_view->setReadOnly(true);

    mem_view = new QPlainTextEdit(this);

    //Set the text views to a monospaced font
    const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    disasm_view->setFont(fixed_font);
    mem_view->setFont(fixed_font);

    step_button = new QPushButton(tr("&Step"), this);
    connect(step_button, SIGNAL(pressed()), this, SLOT(step()));

    mem_box = new QGroupBox();
    QVBoxLayout* mem_layout = new QVBoxLayout();
    mem_layout->addWidget(disasm_view);
    mem_layout->addWidget(mem_view);
    mem_box->setLayout(mem_layout);

    button_box = new QGroupBox();
    QVBoxLayout* button_layout = new QVBoxLayout();
    button_layout->addWidget(step_button);
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
    this->cursor = c;
}

void DebugWindow::add_instr_breakpoint_ee(uint32_t addr)
{
    DebugInfo* info = e->get_debug_info();

    //Check if the breakpoint has already been added
    for (unsigned int i = 0; i < info->ee_breakpoints.size(); i++)
    {
        if (info->ee_breakpoints[i].addr == addr)
            return;
    }

    Breakpoint_CPU breakpoint;
    breakpoint.addr = addr;
    info->ee_breakpoints.push_back(breakpoint);
}

void DebugWindow::refresh()
{
    disasm_view->clear();
    DebugInfo* info = e->get_debug_info();

    cursor = info->ee->get_PC();
    for (int i = -40; i < 40; i += 4)
    {
        uint32_t addr = cursor + i;
        uint32_t instr = info->ee->read32(addr);

        QString addr_str = QString("[%1] ").arg(QString::number(addr, 16), 8, '0');
        QString instr_str = QString("%1 - ").arg(QString::number(instr, 16), 8, '0');
        QString dis_str = QString::fromStdString(EmotionDisasm::disasm_instr(instr, addr));

        if (!i)
            addr_str = "->" + addr_str;
        else
            addr_str = "  " + addr_str;

        disasm_view->appendPlainText(addr_str + instr_str + dis_str);
    }
}

void DebugWindow::step()
{
    e->step();
    refresh();
}
