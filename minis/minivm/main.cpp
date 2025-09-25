#include <print>

#include "minivm/vm.h"

static constexpr mini::VMInstruction example_halt[] = {
    mini::VMInstruction{.opcode = mini::VMOpCode::Interrupt, .d = static_cast<std::uint8_t>(mini::VMInterruptException::Halt)},
};

int main(int argc, char** argv) {
    mini::VMEngine engine{};
    engine.execute(example_halt);
    return 0;
}
