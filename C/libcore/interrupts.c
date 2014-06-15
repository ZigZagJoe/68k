#include <stdint.h>
#include <interrupts.h>

void default_interrupts() {
    __vectors.address_error = &exception_address_err;
    __vectors.bus_error = &exception_bus_error;
    __vectors.illegal_instr = &exception_illegal_inst;
    __vectors.uninitialized_isr = &exception_bad_isr;
    __vectors.int_spurious = &exception_spurious;
    __vectors.priv_violation = &exception_privilege;
    __vectors.trap[15] = &exception_trap;
}
