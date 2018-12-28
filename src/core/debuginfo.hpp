#ifndef DEBUGINFO_HPP
#define DEBUGINFO_HPP
#include <vector>
#include "ee/emotion.hpp"
#include "iop/iop.hpp"

//All info a debugger needs access to
struct DebugInfo
{
    EmotionEngine* ee;
    IOP* iop;
    Cop0* ee_cop0;
    Cop1* fpu;
    VectorUnit* vu0, *vu1;
    std::vector<uint32_t> ee_instr_breakpoints, iop_instr_breakpoints;
    std::vector<uint32_t> ee_read_breakpoints, iop_read_breakpoints;
    std::vector<uint32_t> ee_write_breakpoints, iop_write_breakpoints;
};

#endif // DEBUGINFO_HPP
