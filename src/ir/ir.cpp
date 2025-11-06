#include "ir/ir.h"

namespace ns
{
    std::string lookup(Op op)
    {
        switch (op)
        {
        case Op::ADD:
            return "ADD";
        case Op::SUB:
            return "SUB";
        case Op::MUL:
            return "MUL";
        case Op::DIV:
            return "DIV";
        case Op::MOV:
            return "MOV";
        case Op::LDC:
            return "LDC";
        default:
            return "UNKNOWN";
        }
    }
}