#include "debugwindow.hpp"
#include "../core/ee/emotiondisasm.hpp"

DebugWindow::DebugWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Debugger");
    resize(800, 600);
    e = nullptr;
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
    DebugInfo* info = e->get_debug_info();
    for (int i = 0; i < 10 * 4; i += 4)
    {
        uint32_t addr = cursor + i;
        uint32_t instr = info->ee->read32(addr);
        printf("[$%08X] $%08X - %s\n", addr, instr, EmotionDisasm::disasm_instr(instr, addr).c_str());
    }
}
