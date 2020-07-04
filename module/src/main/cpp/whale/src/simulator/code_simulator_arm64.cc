#include "vixl/aarch64/instructions-aarch64.h"
#include "simulator/code_simulator_arm64.h"
#include "base/logging.h"

namespace whale {
namespace arm64 {

using namespace vixl::aarch64;  // NOLINT(build/namespaces)

CodeSimulatorArm64* CodeSimulatorArm64::CreateCodeSimulatorArm64() {
    if (kCanSimulate) {
        return new CodeSimulatorArm64();
    } else {
        return nullptr;
    }
}

CodeSimulatorArm64::CodeSimulatorArm64()
        : CodeSimulator(), decoder_(nullptr), simulator_(nullptr) {
    CHECK(kCanSimulate);
    decoder_ = new Decoder();
    simulator_ = new Simulator(decoder_);
}

CodeSimulatorArm64::~CodeSimulatorArm64() {
    CHECK(kCanSimulate);
    delete simulator_;
    delete decoder_;
}

void CodeSimulatorArm64::RunFrom(intptr_t code_buffer) {
    CHECK(kCanSimulate);
    simulator_->RunFrom(reinterpret_cast<const Instruction*>(code_buffer));
}

bool CodeSimulatorArm64::GetCReturnBool() const {
    CHECK(kCanSimulate);
    return static_cast<bool>(simulator_->ReadWRegister(0));
}

int32_t CodeSimulatorArm64::GetCReturnInt32() const {
    CHECK(kCanSimulate);
    return simulator_->ReadWRegister(0);
}

int64_t CodeSimulatorArm64::GetCReturnInt64() const {
    CHECK(kCanSimulate);
    return simulator_->ReadXRegister(0);
}


}  // namespace arm64
}  // namespace whale

