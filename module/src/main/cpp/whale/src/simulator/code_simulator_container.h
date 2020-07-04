#ifndef WHALE_SIMULATOR_CODE_SIMULATOR_CONTAINER_H_
#define WHALE_SIMULATOR_CODE_SIMULATOR_CONTAINER_H_

#include <dbi/instruction_set.h>
#include "simulator/code_simulator.h"
#include "base/logging.h"

namespace whale {


class CodeSimulatorContainer {
 public:
    explicit CodeSimulatorContainer(InstructionSet target_isa);

    ~CodeSimulatorContainer();

    bool CanSimulate() const {
        return simulator_ != nullptr;
    }

    CodeSimulator *Get() {
        DCHECK(CanSimulate());
        return simulator_;
    }

    const CodeSimulator *Get() const {
        DCHECK(CanSimulate());
        return simulator_;
    }

 private:
    CodeSimulator *simulator_;

    DISALLOW_COPY_AND_ASSIGN(CodeSimulatorContainer);
};


}  // namespace whale


#endif  // WHALE_SIMULATOR_CODE_SIMULATOR_CONTAINER_H_


