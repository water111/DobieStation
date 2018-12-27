#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QInputDialog>

#include "debugwindow.hpp"
#include "../core/ee/emotiondisasm.hpp"

const int MAX_DISASM_INSTRS = 1024 * 16;
const int MAX_MEM_VIEW = 1024;

QString u32_to_hexstr(uint32_t value)
{
    return QString("%1").arg(QString::number(value, 16), 8, '0');
}

QString u128_to_hexstr(uint128_t value)
{
    QString str;
    for (int i = 3; i >= 0; i--)
    {
        str += u32_to_hexstr(value._u32[i]);
        if (i)
            str += "_";
    }
    return str;
}

void DebugWindow::insert_string(QTableWidget *table, int column, QString str)
{
    QTableWidgetItem* item = new QTableWidgetItem(str);
    table->insertRow(table->rowCount());
    table->setItem(table->rowCount() - 1, column, item);
}

DebugWindow::DebugWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Debugger");
    resize(800, 600);
    e = nullptr;
    cursor = 0;
    ee_memory_cursor = 0x00100000;
    iop_memory_cursor = 0x00100000;

    cpu_selection = DEBUG_SELECTION::EE;

    emulation_running = true;

    create_tables();

    step_button = new QPushButton(tr("Step"), this);
    connect(step_button, SIGNAL(pressed()), this, SLOT(step()));

    toggle_run_button = new QPushButton(tr("Break"), this);
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
    verticalHeader->setDefaultSectionSize(18);

    reg_view = new QTableWidget(31, 1, this);
    reg_view->horizontalHeader()->hide();
    reg_view->verticalHeader()->hide();
    reg_view->setColumnWidth(0, 300);
    reg_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reg_view->setShowGrid(false);

    verticalHeader = reg_view->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(18);

    mem_view = new QTableWidget(MAX_MEM_VIEW, 5, this);
    mem_view->horizontalHeader()->hide();
    mem_view->verticalHeader()->hide();
    mem_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mem_view->setShowGrid(false);

    verticalHeader = mem_view->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(18);

    QHeaderView *horizontalHeader = mem_view->horizontalHeader();
    horizontalHeader->setSectionResizeMode(QHeaderView::Fixed);
    horizontalHeader->setDefaultSectionSize(70);

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
    update_memory(true);
}

void DebugWindow::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    switch (event->key())
    {
        case Qt::Key_G:
            if (event->modifiers() == Qt::ControlModifier && !emulation_running)
                prompt_disasm_jump();
            break;
        case Qt::Key_M:
            if (event->modifiers() == Qt::ControlModifier && !emulation_running)
                prompt_memory_jump();
            break;
    }
}

bool DebugWindow::addr_valid(uint32_t addr)
{
    return true;
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
        QString addr_str = "[" + u32_to_hexstr(addr) + "] ";

        if (!addr_valid(addr))
        {
            QTableWidgetItem* invalid = new QTableWidgetItem(addr_str + "FFFFFFFF - ???");
            disasm_view->setItem(i / 4, 0, invalid);
            continue;
        }

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

        QString instr_str = u32_to_hexstr(instr) + " ";
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
        if (scroll_to_center && addr == cursor)
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
    reg_view->setRowCount(0);
    EmotionEngine* ee = e->get_debug_info()->ee;

    //GPRs
    for (int i = 0; i < 31; i++)
    {
        QString reg_str = EmotionEngine::REG(i + 1);
        QString gpr_str = u128_to_hexstr(ee->get_gpr<uint128_t>(i + 1));
        insert_string(reg_view, 0, reg_str + ":" + gpr_str);
    }

    //Special registers
    uint128_t lo_data, hi_data;
    lo_data._u64[0] = ee->get_LO();
    lo_data._u64[1] = ee->get_LO1();
    hi_data._u64[0] = ee->get_HI();
    hi_data._u64[1] = ee->get_HI1();

    QString lo_str, hi_str;
    lo_str = u128_to_hexstr(lo_data);
    hi_str = u128_to_hexstr(hi_data);

    insert_string(reg_view, 0, "lo:" + lo_str);
    insert_string(reg_view, 0, "hi:" + hi_str);

    QString sa_str = u32_to_hexstr(ee->get_SA());
    insert_string(reg_view, 0, "sa:" + sa_str);

    QString pc_str = u32_to_hexstr(ee->get_PC());
    insert_string(reg_view, 0, "pc:" + pc_str);

    //COP0 registers
    Cop0* ee_cop0 = e->get_debug_info()->ee_cop0;
    for (int i = 0; i < 32; i++)
    {
        QString reg_str = EmotionEngine::COP0_REG(i);
        if (reg_str == "---")
            continue;
        QString cop0_str;
        if (i == 25)
        {
            cop0_str = u32_to_hexstr(ee_cop0->PCCR) + "/";
            cop0_str += u32_to_hexstr(ee_cop0->PCR0) + "/";
            cop0_str += u32_to_hexstr(ee_cop0->PCR1);
        }
        else
            cop0_str = u32_to_hexstr(ee_cop0->mfc(i));
        insert_string(reg_view, 0, reg_str + ":" + cop0_str);
    }

    //FPRs
    Cop1* fpu = e->get_debug_info()->fpu;

    for (int i = 0; i < 32; i++)
    {
        QString reg_str = QString("f%1:").arg(QString::number(i));
        float fpr = fpu->get_gpr_f(i);
        QString fpr_str;
        fpr_str.setNum(fpr);
        insert_string(reg_view, 0, reg_str + fpr_str);
    }
}

void DebugWindow::update_registers_iop()
{
    reg_view->clear();
    reg_view->setRowCount(0);
    IOP* iop = e->get_debug_info()->iop;

    //GPRs
    for (int i = 0; i < 31; i++)
    {
        QString reg_str = IOP::REG(i + 1);
        QString data_str = u32_to_hexstr(iop->get_gpr(i));
        insert_string(reg_view, 0, reg_str + ":" + data_str);
    }

    //Special registers
    QString lo_str = u32_to_hexstr(iop->get_LO());
    insert_string(reg_view, 0, "lo:" + lo_str);

    QString hi_str = u32_to_hexstr(iop->get_HI());
    insert_string(reg_view, 0, "hi:" + hi_str);

    QString pc_str = u32_to_hexstr(iop->get_PC());
    insert_string(reg_view, 0, "pc:" + pc_str);
}

void DebugWindow::update_memory(bool scroll_to_center)
{
    uint32_t memory_cursor;
    switch (cpu_selection)
    {
        case DEBUG_SELECTION::EE:
            memory_cursor = ee_memory_cursor;
            break;
        case DEBUG_SELECTION::IOP:
            memory_cursor = iop_memory_cursor;
            break;
        default:
            return;
    }

    DebugInfo* info = e->get_debug_info();
    for (int i = 0; i < mem_view->rowCount(); i++)
    {
        uint32_t addr = memory_cursor + (i * 16) - (MAX_MEM_VIEW * 8);
        QTableWidgetItem* addr_item = new QTableWidgetItem("$" + u32_to_hexstr(addr));

        for (int j = 0; j < 4; j++)
        {
            uint32_t data = 0xFFFFFFFF;
            switch (cpu_selection)
            {
                case DEBUG_SELECTION::EE:
                    data = info->ee->read32(addr + (j * 4));
                    break;
                case DEBUG_SELECTION::IOP:
                    data = info->iop->read32(addr + (j * 4));
                    break;
            }
            QTableWidgetItem* data_item = new QTableWidgetItem(u32_to_hexstr(data));
            mem_view->setItem(i, j + 1, data_item);
        }

        mem_view->setItem(i, 0, addr_item);
        if (scroll_to_center && (addr & ~0xF) == (memory_cursor & ~0xF))
            mem_view->scrollToItem(addr_item, QAbstractItemView::PositionAtCenter);
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
    if (!emulation_running)
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

void DebugWindow::prompt_disasm_jump()
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

void DebugWindow::prompt_memory_jump()
{
    QString text = QInputDialog::getText(this, "Memory address", "Enter a memory address to display");
    bool ok;
    uint32_t new_addr = text.toUInt(&ok, 16);
    if (ok)
    {
        switch (cpu_selection)
        {
            case DEBUG_SELECTION::EE:
                ee_memory_cursor = new_addr;
                break;
            case DEBUG_SELECTION::IOP:
                iop_memory_cursor = new_addr;
                break;
            default:
                return;
        }
        update_memory(true);
    }
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
