#include "simulator/code_simulator_container.h"
#include "simulator/code_simulator.h"

namespace whale {


CodeSimulatorContainer::CodeSimulatorContainer(InstructionSet target_isa) : simulator_(nullptr) {
    simulator_ = CodeSimulator::CreateCodeSimulator(target_isa);
}


CodeSimulatorContainer::~CodeSimulatorContainer() {
    delete simulator_;
}


}