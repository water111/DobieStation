#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QInputDialog>

#include "debugwindow.hpp"
#include "../core/ee/emotiondisasm.hpp"

const int MAX_DISASM_INSTRS = 16384;

DebugWindow::DebugWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Debugger");
    resize(800, 600);
    e = nullptr;
    cursor = 0;

    cpu_selection = DEBUG_SELECTION::EE;

    emulation_running = true;

    create_tables();

    step_button = new QPushButton(tr("&Step"), this);
    connect(step_button, SIGNAL(pressed()), this, SLOT(step()));

    toggle_run_button = new QPushButton(tr("&Break"), this);
    connect(toggle_run_button, SIGNAL(pressed()), this, SLOT(toggle_run()));

    create_cpu_selection();

    disasm_reg_box = new QGroupBox();
    QHBoxLayout* disasm_reg_layout = new QHBoxLayout();
    disasm_reg_layout->addWidget(disasm_view, 12);
    disasm_reg_layout->addWidget(reg_view, 10);
    disasm_reg_box->setLayout(disasm_reg_layout);

    button_box = new QGroupBox();
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->addWidget(step_button);
    button_layout->addWidget(toggle_run_button);
    button_box->setLayout(button_layout);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addWidget(cpu_selection_box, 1);
    main_layout->addWidget(disasm_reg_box, 10);
    main_layout->addWidget(mem_view, 1);
    main_layout->addWidget(button_box, 1);
    setLayout(main_layout);
}

void DebugWindow::create_cpu_selection()
{
    ee_select_button = new QPushButton(tr("EE"), this);
    iop_select_button = new QPushButton(tr("IOP"), this);
    vu0_select_button = new QPushButton(tr("VU0"), this);
    vu1_select_button = new QPushButton(tr("VU1"), this);

    connect(ee_select_button, SIGNAL(pressed()), this, SLOT(select_ee()));
    connect(iop_select_button, SIGNAL(pressed()), this, SLOT(select_iop()));
    cpu_selection_box = new QGroupBox();

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(ee_select_button);
    layout->addWidget(iop_select_button);
    layout->addWidget(vu0_select_button);
    layout->addWidget(vu1_select_button);
    cpu_selection_box->setLayout(layout);
}

void DebugWindow::create_tables()
{
    disasm_view = new QTableWidget(MAX_DISASM_INSTRS, 1, this);
    disasm_view->horizontalHeader()->hide();
    disasm_view->verticalHeader()->hide();
    disasm_view->setColumnWidth(0, 350);
    disasm_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    disasm_view->setShowGrid(false);

    QHeaderView *verticalHeader = disasm_view->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(16);

    reg_view = new QTableWidget(31, 1, this);
    reg_view->horizontalHeader()->hide();
    reg_view->verticalHeader()->hide();
    reg_view->setColumnWidth(0, 300);
    reg_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reg_view->setShowGrid(false);

    verticalHeader = reg_view->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(16);

    mem_view = new QTableWidget(5, 16, this);
    mem_view->horizontalHeader()->hide();
    mem_view->verticalHeader()->hide();
    mem_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mem_view->setShowGrid(false);

    verticalHeader = mem_view->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(16);

    connect(disasm_view, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(instr_breakpoint_toggle(int,int)));

    //Set the font to the default monospaced system font
    const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    disasm_view->setFont(fixed_font);
    reg_view->setFont(fixed_font);
    mem_view->setFont(fixed_font);
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

    switch (cpu_selection)
    {
        case DEBUG_SELECTION::EE:
            cursor = info->ee->get_PC();
            break;
        case DEBUG_SELECTION::IOP:
            cursor = info->iop->get_PC();
            break;
        default:
            cursor = 0;
            break;
    }
    update_toggle_run_button();
    update_disassembly(true);
    update_registers();
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

    uint32_t PC;
    std::vector<Breakpoint_CPU> *breakpoints;

    switch (cpu_selection)
    {
        case DEBUG_SELECTION::EE:
            PC = info->ee->get_PC();
            breakpoints = &info->ee_breakpoints;
            break;
        case DEBUG_SELECTION::IOP:
            PC = info->iop->get_PC();
            breakpoints = &info->iop_breakpoints;
            break;
        default:
            PC = 0;
            break;
    }

    for (int i = 0; i < MAX_DISASM_INSTRS * 4; i += 4)
    {
        uint32_t addr = cursor + (i - (MAX_DISASM_INSTRS * 2));

        uint32_t instr;
        switch (cpu_selection)
        {
            case DEBUG_SELECTION::EE:
                instr = info->ee->read32(addr);
                break;
            case DEBUG_SELECTION::IOP:
                instr = info->iop->read32(addr);
                break;
            default:
                instr = 0xFFFFFFFF;
        }

        QString addr_str = QString("[%1] ").arg(QString::number(addr, 16), 8, '0');
        QString instr_str = QString("%1 ").arg(QString::number(instr, 16), 8, '0');
        QString dis_str = QString::fromStdString(EmotionDisasm::disasm_instr(instr, addr));

        if (addr == PC)
            addr_str = "->" + addr_str;
        else
            addr_str = "  " + addr_str;

        QTableWidgetItem* text_item = new QTableWidgetItem(addr_str + instr_str + dis_str);

        //Change the color of the cell if it's a breakpoint
        for (int i = 0; i < breakpoints->size(); i++)
        {
            if (breakpoints->at(i).addr == addr)
                text_item->setBackground(Qt::red);
        }

        disasm_view->setItem(i / 4, 0, text_item);
        if (scroll_to_center && i == MAX_DISASM_INSTRS * 2)
            disasm_view->scrollToItem(text_item, QAbstractItemView::PositionAtCenter);
    }
}

void DebugWindow::update_registers()
{
    switch (cpu_selection)
    {
        case DEBUG_SELECTION::EE:
            update_registers_ee();
            break;
        case DEBUG_SELECTION::IOP:
            update_registers_iop();
            break;
        default:
            break;
    }
}

void DebugWindow::update_registers_ee()
{
    reg_view->clear();
    EmotionEngine* ee = e->get_debug_info()->ee;
    for (int i = 0; i < 31; i++)
    {
        QString reg_str = EmotionEngine::REG(i + 1);
        QString data_str;
        uint128_t data = ee->get_gpr<uint128_t>(i + 1);
        for (int j = 3; j >= 0; j--)
        {
            data_str += QString("%1").arg(QString::number(data._u32[j], 16), 8, '0');
            if (j)
                data_str += "_";
        }
        QTableWidgetItem* text_item = new QTableWidgetItem(reg_str + ":" + data_str);
        reg_view->setItem(i, 0, text_item);
    }
}

void DebugWindow::update_registers_iop()
{
    reg_view->clear();
    IOP* iop = e->get_debug_info()->iop;
    for (int i = 0; i < 31; i++)
    {
        QString reg_str = IOP::REG(i + 1);
        QString data_str = QString("%1").arg(QString::number(iop->get_gpr(i), 16), 8, '0');
        QTableWidgetItem* text_item = new QTableWidgetItem(reg_str + ":" + data_str);
        reg_view->setItem(i, 0, text_item);
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

    std::vector<Breakpoint_CPU>* breakpoints;

    switch (cpu_selection)
    {
        case DEBUG_SELECTION::EE:
            breakpoints = &info->ee_breakpoints;
            break;
        case DEBUG_SELECTION::IOP:
            breakpoints = &info->iop_breakpoints;
            break;
        default:
            breakpoints = nullptr;
            break;
    }

    bool breakpoint_found = false;
    for (int i = 0; i < breakpoints->size(); i++)
    {
        if (addr == breakpoints->at(i).addr)
        {
            breakpoints->erase(breakpoints->begin() + i);
            breakpoint_found = true;
            break;
        }
    }

    if (!breakpoint_found)
    {
        Breakpoint_CPU breakpoint;
        breakpoint.addr = addr;
        breakpoints->push_back(breakpoint);
    }

    update_disassembly(false);
}

void DebugWindow::select_ee()
{
    cpu_selection = DEBUG_SELECTION::EE;
    refresh();
}

void DebugWindow::select_iop()
{
    cpu_selection = DEBUG_SELECTION::IOP;
    refresh();
}
