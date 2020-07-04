#ifndef WHALE_SIMULATOR_CODE_SIMULATOR_H_
#define WHALE_SIMULATOR_CODE_SIMULATOR_H_

#include <cstdint>
#include "dbi/instruction_set.h"
#include "base/primitive_types.h"

namespace whale {

class CodeSimulator {
 public:
    CodeSimulator() = default;

    virtual ~CodeSimulator() = default;

    // Returns a null pointer if a simulator cannot be found for target_isa.
    static CodeSimulator *CreateCodeSimulator(InstructionSet target_isa);

    virtual void RunFrom(intptr_t code_buffer) = 0;

    // Get return value according to C ABI.
    virtual bool GetCReturnBool() const = 0;

    virtual s4 GetCReturnInt32() const = 0;

    virtual s8 GetCReturnInt64() const = 0;

 private:
    DISALLOW_COPY_AND_ASSIGN(CodeSimulator);
};

}  // namespace whale

#endif  // WHALE_SIMULATOR_CODE_SIMULATOR_H_

