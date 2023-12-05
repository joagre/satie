#include <stdio.h>
#include "vm.h"

uint32_t print_instruction(uint8_t* bytes) {
    // These variables are required by the GET_OPERAND macro
    uint8_t* operands = bytes + OPCODE_SIZE;
    uint32_t size = 0;

    switch (*bytes) {
    // Register machine instructions
    case OPCODE_JMPRNZE: {
        vm_register_t register_ = GET_OPERAND(vm_register_t);
        vm_address_t address = GET_OPERAND(vm_address_t);
        fprintf(stdout, "jmprnze r%d %d\n", register_, address);
        return size;
    }
    case OPCODE_JMPRINGT: {
        vm_register_t register_value = GET_OPERAND(vm_register_t);
        vm_immediate_value_t immediate_value =
            GET_OPERAND(vm_immediate_value_t);
        vm_address_t address = GET_OPERAND(vm_address_t);
        fprintf(stdout, "jmpringt r%d #%ld %d\n", register_value,
                immediate_value, address);
        return size;
    }
    case OPCODE_SUBRRI: {
        vm_register_t first_register = GET_OPERAND(vm_register_t);
        vm_register_t second_register = GET_OPERAND(vm_register_t);
        vm_immediate_value_t immediate_value =
            GET_OPERAND(vm_immediate_value_t);
        fprintf(stdout, "subrri r%d r%d #%ld\n", first_register,
                second_register, immediate_value);
        return size;
    }
    case OPCODE_SUBRSI: {
        vm_register_t register_value = GET_OPERAND(vm_register_t);
        vm_stack_offset_t stack_offset = GET_OPERAND(vm_stack_offset_t);
        vm_immediate_value_t immediate_value =
            GET_OPERAND(vm_immediate_value_t);
        fprintf(stdout, "subrsi r%d @%d #%ld\n", register_value, stack_offset,
                immediate_value);
        return size;
    }
    case OPCODE_ADDRRI: {
        vm_register_t first_register = GET_OPERAND(vm_register_t);
        vm_register_t second_register = GET_OPERAND(vm_register_t);
        vm_immediate_value_t immediate_value =
            GET_OPERAND(vm_immediate_value_t);
        fprintf(stdout, "addrri r%d r%d #%ld\n", first_register, second_register,
                immediate_value);
        return size;
    }
    case OPCODE_LOADRI: {
        vm_register_t register_ = GET_OPERAND(vm_register_t);
        vm_immediate_value_t immediate_value =
            GET_OPERAND(vm_immediate_value_t);
        fprintf(stdout, "loadri r%d #%ld\n", register_, immediate_value);
        return size;
    }
    case OPCODE_PUSHR: {
        vm_register_t register_ = GET_OPERAND(vm_register_t);
        fprintf(stdout, "pushr r%d\n", register_);
        return size;
    }
    case OPCODE_LOADRS: {
        vm_register_t register_ = GET_OPERAND(vm_register_t);
        vm_stack_offset_t stack_offset = GET_OPERAND(vm_stack_offset_t);
        fprintf(stdout, "loadrs r%d @%d\n", register_, stack_offset);
        return size;
    }
    case OPCODE_LOADRR: {
        vm_register_t first_register = GET_OPERAND(vm_register_t);
        vm_register_t second_register = GET_OPERAND(vm_register_t);
        fprintf(stdout, "loadrr r%d r%d\n", first_register, second_register);
        return size;
    }
    case OPCODE_RCALL: {
        vm_address_t address = GET_OPERAND(vm_address_t);
        fprintf(stdout, "rcall %d\n", address);
        return size;
    }
    case OPCODE_JMP: {
        vm_address_t address = GET_OPERAND(vm_address_t);
        fprintf(stdout, "jmp %d\n", address);
        return size;
    }
    // Stack machine instructions
    case OPCODE_PUSH: {
        vm_stack_value_t stack_value = GET_OPERAND(vm_stack_value_t);
        fprintf(stdout, "push %ld\n", stack_value);
        return size;
    }
    case OPCODE_PUSHS: {
        vm_data_length_t data_length = GET_OPERAND(vm_data_length_t);
        uint8_t* byte_string = operands + size;
        fprintf(stdout, "pushs \"%s\"\n", byte_string);
        return size + data_length;
    }
    case OPCODE_JUMP: {
        vm_address_t address = GET_OPERAND(vm_address_t);
        fprintf(stdout, "jump %d\n", address);
        return size;
    }
    case OPCODE_CJUMP: {
        vm_address_t address = GET_OPERAND(vm_address_t);
        fprintf(stdout, "cjump %d\n", address);
        return size;
    }
    case OPCODE_CALL: {
        vm_address_t address = GET_OPERAND(vm_address_t);
        vm_arity_t arity = GET_OPERAND(vm_arity_t);
        fprintf(stdout, "call %d %d\n", address, arity);
        return size;
    }
    case OPCODE_RET: {
        vm_return_mode_t return_mode = GET_OPERAND(vm_return_mode_t);
        if (return_mode == RETURN_MODE_COPY) {
            fprintf(stdout, "ret copy\n");
        } else {
            fprintf(stdout, "ret\n");
        }
        return size;
    }
    case OPCODE_SYS: {
        vm_system_call_t system_call = GET_OPERAND(vm_system_call_t);
        fprintf(stdout, "sys %d\n", system_call);
        return size;
    }
    case OPCODE_SPAWN: {
        vm_address_t address = GET_OPERAND(vm_address_t);
        vm_arity_t arity = GET_OPERAND(vm_arity_t);
        fprintf(stdout, "spawn %d %d\n", address, arity);
        return size;
    }
    default:
        opcode_info_t* opcode_info = opcode_to_opcode_info(*bytes);
        fprintf(stdout, "%s\n", opcode_info->string);
    }

    return 0;
}