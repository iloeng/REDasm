#include "assembler.h"
#include "../format.h"
#include <iomanip>
#include <sstream>

namespace REDasm {

AssemblerPlugin::AssemblerPlugin(): Plugin(), _endianness(Endianness::LittleEndian)
{
}

u32 AssemblerPlugin::flags() const
{
    return AssemblerFlags::None;
}

VMIL::Emulator *AssemblerPlugin::createEmulator(DisassemblerFunctions *disassembler) const
{
    RE_UNUSED(disassembler);
    return NULL;
}

Printer *AssemblerPlugin::createPrinter(DisassemblerFunctions *disassembler, SymbolTable *symboltable) const
{
    return new Printer(disassembler, symboltable);
}

void AssemblerPlugin::analyzeOperand(DisassemblerFunctions *disassembler, const InstructionPtr &instruction, const Operand &operand) const
{
    if(operand.is(OperandTypes::Register))
    {
        this->analyzeRegister(disassembler, instruction, operand);
        return;
    }

    SymbolTable* symboltable = disassembler->symbolTable();
    u64 value = operand.is(OperandTypes::Displacement) ? operand.mem.displacement : operand.u_value, opvalue = value;
    SymbolPtr symbol = symboltable->symbol(value);

    if(!symbol || (symbol && !symbol->is(SymbolTypes::Import))) // Don't try to dereference imports
    {
        if(operand.is(OperandTypes::Memory) && (operand.isRead() || instruction->is(InstructionTypes::Branch)))
        {
            if(disassembler->dereferencePointer(value, opvalue)) // Try to read pointed memory
                symboltable->createLocation(value, SymbolTypes::Data | SymbolTypes::Pointer); // Create Symbol for pointer
        }
    }

    const Segment* segment = disassembler->format()->segment(opvalue);

    if(!segment)
        return;

    if(instruction->is(InstructionTypes::Call) && instruction->hasTargets() && (operand.index == instruction->target_idx))
    {
        if(symbol && !symbol->isFunction()) // This symbol will be promoted to function
            symboltable->erase(opvalue);

        if(symboltable->createFunction(opvalue)) // This operand is the target
            disassembler->disassemble(opvalue);
    }
    else
    {
        bool wide = false;

        if(instruction->is(InstructionTypes::Jump))
        {
            if(!operand.is(OperandTypes::Displacement) || operand.mem.displacementOnly())
            {
                int dir = BRANCH_DIRECTION(instruction, opvalue);

                if(dir < 0)
                    instruction->cmt("Possible loop");
                else if(!dir)
                    instruction->cmt("Infinite loop");

                disassembler->updateInstruction(instruction);
                symboltable->createLocation(opvalue, SymbolTypes::Code);
            }
            else
                disassembler->checkJumpTable(instruction, operand);
        }
        else if(!segment->is(SegmentTypes::Bss) && (disassembler->locationIsString(opvalue, &wide) >= MIN_STRING))
        {
            if(wide)
            {
                symboltable->createWString(opvalue);
                instruction->cmt("UNICODE: " + disassembler->readWString(opvalue));
            }
            else
            {
                symboltable->createString(opvalue);
                instruction->cmt("STRING: " + disassembler->readString(opvalue));
            }

           disassembler->updateInstruction(instruction);
        }
        else
            symboltable->createLocation(opvalue, SymbolTypes::Data);
    }

    symbol = symboltable->symbol(opvalue);

    if(symbol)
        disassembler->pushReference(symbol, instruction->address);
}

bool AssemblerPlugin::decode(Buffer buffer, const InstructionPtr &instruction)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for(u64 i = 0; i < instruction->size; i++)
    {
        u8 b = buffer[i];
        ss << std::setw(2) << static_cast<size_t>(b);
    }

    instruction->bytes = ss.str();
    return false;
}

bool AssemblerPlugin::done(const InstructionPtr &instruction)
{
    if(this->_statestack.top() & AssemblerFlags::DelaySlot)
    {
        this->_statestack.top() &= ~AssemblerFlags::DelaySlot;
        return true;
    }

    if((instruction->is(InstructionTypes::Jump) && !instruction->is(InstructionTypes::Conditional)))
    {
        if(this->flags() & AssemblerFlags::DelaySlot)
        {
            this->_statestack.top() |= AssemblerFlags::DelaySlot;
            return false;
        }

        return true;
    }

    if(instruction->is(InstructionTypes::Stop))
    {
        this->_statestack.top() &= ~AssemblerFlags::DelaySlot;
        return true;
    }

    return false;
}

bool AssemblerPlugin::hasFlag(u32 flag) const
{
    return this->flags() & flag;
}

bool AssemblerPlugin::hasVMIL() const
{
    return this->hasFlag(AssemblerFlags::HasVMIL);
}

bool AssemblerPlugin::canEmulateVMIL() const
{
    return this->hasFlag(AssemblerFlags::EmulateVMIL);
}

endianness_t AssemblerPlugin::endianness() const
{
    return this->_endianness;
}

void AssemblerPlugin::setEndianness(endianness_t endianness)
{
    this->_endianness = endianness;
}

void AssemblerPlugin::pushState()
{
    this->_statestack.push(AssemblerFlags::None);
}

void AssemblerPlugin::popState()
{
    this->_statestack.pop();
}

void AssemblerPlugin::analyzeRegister(DisassemblerFunctions *disassembler, const InstructionPtr &instruction, const Operand &operand) const
{
    if(!disassembler->emulator() || !instruction->is(InstructionTypes::Branch) || !operand.is(OperandTypes::Register) || (operand.index != instruction->target_idx))
        return;

    address_t target = 0;

    if(!disassembler->emulator()->read(operand, target))
        return;

    REDasm::log("VMIL @ " + REDasm::hex(instruction->address) + " jump to " + REDasm::hex(target));

    if(!this->canEmulateVMIL())
    {
        instruction->cmt("=" + REDasm::hex(target));
        disassembler->updateInstruction(instruction);
        return;
    }

    if(!disassembler->disassemble(target))
        return;

    if(instruction->is(InstructionTypes::Call))
        disassembler->symbolTable()->createFunction(target);
    else
        disassembler->symbolTable()->createLocation(target, SymbolTypes::Code);

    SymbolPtr symbol = disassembler->symbolTable()->symbol(target);

    instruction->target(target);
    instruction->cmt("=" + symbol->name);
    disassembler->updateInstruction(instruction);
    disassembler->pushReference(symbol, instruction->address);
}

}