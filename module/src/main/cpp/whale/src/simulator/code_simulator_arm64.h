#ifndef WHALE_SIMULATOR_CODE_SIMULATOR_ARM64_H_
#define WHALE_SIMULATOR_CODE_SIMULATOR_ARM64_H_

#include <cstdint>
#include "dbi/instruction_set.h"
#include "base/primitive_types.h"
#include "vixl/aarch64/decoder-aarch64.h"
#include "vixl/aarch64/simulator-aarch64.h"
#include "simulator/code_simulator.h"

namespace whale {
namespace arm64 {

class CodeSimulatorArm64 : public CodeSimulator {
 public:
    static CodeSimulatorArm64 *CreateCodeSimulatorArm64();

    ~CodeSimulatorArm64() override;

    void RunFrom(intptr_t code_buffer) override;

    bool GetCReturnBool() const override;

    int32_t GetCReturnInt32() const override;

    int64_t GetCReturnInt64() const override;

 private:
    CodeSimulatorArm64();

    vixl::aarch64::Decoder *decoder_;
    vixl::aarch64::Simulator *simulator_;

    static constexpr bool kCanSimulate = (kRuntimeISA != InstructionSet::kArm64);

    DISALLOW_COPY_AND_ASSIGN(CodeSimulatorArm64);
};

}
}  // namespace whale

#endif  // WHALE_SIMULATOR_CODE_SIMULATOR_ARM64_H_

