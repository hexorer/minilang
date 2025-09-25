#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <variant>

static constexpr std::string_view _notes = R"(
registers:
0000: nul (selects value part of the instruction when appropriate)
0001: r1
0010: r2
0011: r3
0100: r4
0101: r5
0110: r6
0111: r7
1000: rbase
1001: rstack
1010: rzero
1011: rones
1100: ...
1101: ...
1110: rflags
1111: pc (read-only)

instructions:
0000|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: interrupt <?:type> <?:number> (all 0s for halt)
0001|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: add <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
0010|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: sub <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
0011|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: mul <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
0100|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: div <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
0101|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: and <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
0110|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: or <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
0111|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: xor <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
1001|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: load <d:size_specifier> <x:reg_dst>, <y:reg_location/val_location>
1010|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: store <d:size_specifier> <x:reg_src>, <y:reg_location/val_location>
1011|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: compare <d:reg_dst>, <x:reg_lhs>, <y:reg_rhs/value>
1100|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: branch <TODO>, <y:reg_abs_location/val_rel_location>
1101|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: mov <d:reg_dst>, <x:reg_src/value> [, <y[0]:L/R><y[1:3]:shift>]
1110|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: assign <d:reg_dst>, <value>[, <x,y:bit-range>]
1111|dddd|xxxx|yyyy|vvvv,vvvv,vvvv,vvvv: extended instructions... ((x << 4 + y) as extended opcode, d and v as params)
)";

namespace mini {

enum struct VMRegIdx : std::uint8_t {
    Nul = 0b0000,
    R1 = 0b0001,
    R2 = 0b0010,
    R3 = 0b0011,
    R4 = 0b0100,
    R5 = 0b0101,
    R6 = 0b0110,
    R7 = 0b0111,
    RBase = 0b1000,
    RStack = 0b1001,
    RZero = 0b1010,
    ROnes = 0b1011,
    Reserved12 = 0b1100,
    Reserved13 = 0b1101,
    RFlags = 0b1110,
    PC = 0b1111,
    // 4-bits only. END.
};
static_assert(static_cast<std::size_t>(VMRegIdx::PC) == 15);

enum struct VMOpCode : std::uint8_t {
    Interrupt = 0b0000,
    Add = 0b0001,
    Sub = 0b0010,
    Mul = 0b0011,
    Div = 0b0100,
    And = 0b0101,
    Or = 0b0110,
    Xor = 0b0111,
    Load = 0b1001,
    Store = 0b1010,
    Compare = 0b1011,
    Branch = 0b1100,
    Move = 0b1101,
    Assign = 0b1110,
    Extended = 0b1111,
    // 4-bits only. END.
};

struct alignas(std::uint32_t) VMInstruction {
    VMOpCode opcode : 4;
    std::uint8_t d : 4;
    std::uint8_t x : 4;
    std::uint8_t y : 4;
    std::uint16_t v : 16;
};

struct VMRFlagOffset {
    static constexpr std::uint8_t overflow = 0;
};

struct VMRegisterSet {
    union {
        std::uint64_t arr[16] = {0};
        struct {
            std::uint64_t nul;
            std::uint64_t r1;
            std::uint64_t r2;
            std::uint64_t r3;
            std::uint64_t r4;
            std::uint64_t r5;
            std::uint64_t r6;
            std::uint64_t r7;
            std::uint64_t rbase;
            std::uint64_t rstack;
            std::uint64_t rzero;
            std::uint64_t rones;
            std::uint64_t reserved12;
            std::uint64_t reserved13;
            std::uint64_t rflags;
            std::uint64_t pc;
        };
    };
};

struct VMInterruptRoutineTable {
    std::uint64_t routines[16] = {0};
};

struct VMMemorySegment {
    explicit VMMemorySegment(std::size_t size) : ptr{std::make_unique<std::uint8_t[]>(size)}, size{size} {}
    std::unique_ptr<std::uint8_t[]> ptr{};
    const std::size_t size{};
};

struct VMConfig {
    // TODO
};

struct VMContext {
    VMRegisterSet regs;
    VMInterruptRoutineTable irt;
    VMMemorySegment stack;
    bool terminate = false;
};

enum struct VMInterruptException : std::uint8_t {
    Halt = 0,
    IllegalInstruction = 1,
    SegmentationFault = 2,
};

class VMEngine {
public:
    VMEngine() {
        _context.regs.nul = 0;
        _context.regs.r1 = 0;
        _context.regs.r2 = 0;
        _context.regs.r3 = 0;
        _context.regs.r4 = 0;
        _context.regs.r5 = 0;
        _context.regs.r6 = 0;
        _context.regs.r7 = 0;
        _context.regs.rstack = (std::uint64_t)(_context.stack.ptr.get());
        _context.regs.rbase = _context.regs.rstack;
        _context.regs.rflags = 0;
        _context.regs.rzero = 0;
        _context.regs.rones = 0xfffffffffffffffful;
        _context.regs.pc = 0;
    }

    void execute(std::span<const VMInstruction> instructions) {
        while (!_context.terminate) {
            if (_context.regs.pc >= instructions.size()) {
                _context.regs.pc = static_cast<std::uint8_t>(VMInterruptException::SegmentationFault);
                continue;
            }

            const VMInstruction instruction = instructions[_context.regs.pc];
            switch (instruction.opcode) {
                case VMOpCode::Interrupt: ins_interrupt(_context, instruction); break;
                case VMOpCode::Add: ins_add(_context, instruction); break;
                case VMOpCode::Sub: ins_sub(_context, instruction); break;
                case VMOpCode::Mul: ins_mul(_context, instruction); break;
                case VMOpCode::Div: ins_div(_context, instruction); break;
                case VMOpCode::And: ins_and(_context, instruction); break;
                case VMOpCode::Or: ins_or(_context, instruction); break;
                case VMOpCode::Xor: ins_xor(_context, instruction); break;
                case VMOpCode::Load: ins_load(_context, instruction); break;
                case VMOpCode::Store: ins_store(_context, instruction); break;
                case VMOpCode::Compare: ins_compare(_context, instruction); break;
                case VMOpCode::Branch: ins_branch(_context, instruction); break;
                case VMOpCode::Move: ins_move(_context, instruction); break;
                case VMOpCode::Assign: ins_assign(_context, instruction); break;
                case VMOpCode::Extended: ins_extended(_context, instruction); break;
            }
        }
    }

private:
    static void ins_interrupt(VMContext& context, VMInstruction instruction) {
        if (instruction.d == 0) {
            context.terminate = true;
            return;
        }
        context.regs.pc = context.irt.routines[instruction.d];
    }

    static void ins_add(VMContext& context, VMInstruction instruction) {
        // const std::uint64_t reg_x = context.regs.arr[instruction.x];
        // const std::uint64_t reg_y = context.regs.arr[instruction.y];
        // std::uint64_t& reg_d = context.regs.arr[instruction.d];
        // reg_d = reg_x + reg_y;
        // context.regs.rflags = ((reg_d < reg_x || reg_d < reg_y) ? (1ul << VMRFlagOffset::overflow) : 0);
        ++context.regs.pc;
    }

    static void ins_sub(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_mul(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_div(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_and(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_or(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_xor(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_load(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_store(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_compare(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_branch(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_move(VMContext& context, VMInstruction instruction) {
        // const bool shift_right = context.regs.arr[instruction.y] & 0b1000;
        // const std::uint8_t shift_count = context.regs.arr[instruction.y] & 0b0111;
        // const std::uint64_t reg_x = context.regs.arr[instruction.x];
        // context.regs.arr[instruction.d] = shift_right ? (reg_x << shift_count) : (reg_x >> shift_count);
        ++context.regs.pc;
    }

    static void ins_assign(VMContext& context, VMInstruction instruction) {
        // TODO
        ++context.regs.pc;
    }

    static void ins_extended(VMContext& context, VMInstruction instruction) {
        // do nothing.
        ++context.regs.pc;
    }

    VMConfig _config{};
    VMContext _context{
        .regs = VMRegisterSet{.arr = {0}},
        .irt = VMInterruptRoutineTable{.routines = {0}},
        .stack = VMMemorySegment{8 * 1024 * 1024},
    };
};

} // namespace mini
