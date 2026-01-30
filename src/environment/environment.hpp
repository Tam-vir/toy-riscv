#pragma once

class RISCV;

class Environment {
public:
    virtual ~Environment() = default;
    virtual void ecall(RISCV& cpu) = 0;
    virtual void ebreak(RISCV& cpu) = 0;
};
