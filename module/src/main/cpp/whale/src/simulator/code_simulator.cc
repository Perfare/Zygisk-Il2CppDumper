#include "simulator/code_simulator.h"
#include "simulator/code_simulator_arm64.h"

namespace whale {

CodeSimulator *CodeSimulator::CreateCodeSimulator(InstructionSet target_isa) {
    switch (target_isa) {
        case InstructionSet::kArm64:
            return arm64::CodeSimulatorArm64::CreateCodeSimulatorArm64();
        default:
            return nullptr;
    }
}

CodeSimulator *CreateCodeSimulator(InstructionSet target_isa) {
    return CodeSimulator::CreateCodeSimulator(target_isa);
}

}  // namespace whale

