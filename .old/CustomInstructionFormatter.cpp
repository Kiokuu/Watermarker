#include "CustomInstructionFormatter.h"

#include <iostream>

#include "Disassembler.h"
#include "Zycore/Format.h"
#include "Zydis/Utils.h"

CustomInstructionFormatter::CustomInstructionFormatter(uint64_t imageBase, std::unordered_map<uint64_t, Symbol>& symbolMap) : m_image_base(imageBase), m_symbols(symbolMap), m_formatter()
{
	initialize();
	setupHooks();
}

void CustomInstructionFormatter::initialize()
{
	ZydisFormatterInit(&m_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
}

void CustomInstructionFormatter::setupHooks()
{
	m_format_operand_imm = &hook_format_operand_imm_thunk;
	m_print_address_abs = &hook_print_address_abs_thunk;
	m_print_address_rel = &hook_print_address_rel_thunk;
	
	ZydisFormatterSetHook(&m_formatter, ZYDIS_FORMATTER_FUNC_FORMAT_OPERAND_PTR, (const void**)&m_format_operand_imm);
	ZydisFormatterSetHook(&m_formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void**)&m_print_address_abs);
	ZydisFormatterSetHook(&m_formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_REL, (const void**)&m_print_address_rel);
}


ZyanStatus CustomInstructionFormatter::hook_format_operand_imm(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	ZyanString* string;
	ZydisFormatterBufferGetString(buffer, &string);

	// Read the string
	const char* operandString;
	ZyanStringGetData(string, &operandString);

	// print string
	std::cout << operandString << std::endl;

	return m_format_operand_imm(formatter, buffer, context);
}

ZyanStatus CustomInstructionFormatter::hook_format_operand_imm_thunk(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	CustomInstructionFormatter* user_data = ((CustomUserData*)context->user_data)->formatter;
	return user_data->hook_format_operand_imm(formatter, buffer, context);
}


ZyanStatus CustomInstructionFormatter::hook_print_address_abs(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	// address -= context->runtime_address; // symbol table stores relative addresses
	// need image base...
	ZyanU64 address;
	ZydisCalcAbsoluteAddress(context->instruction, context->operand, context->runtime_address, &address);

	ZyanU64 offset = address - m_image_base;

	if(m_symbols.contains(offset))
	{
		const Symbol& symbol = m_symbols.at(offset);
		ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL);
		ZyanString* string;
		ZydisFormatterBufferGetString(buffer, &string);

		return ZyanStringAppendFormat(string, "%s", symbol.getName().c_str());
	}
	else
	{
		std::string newSymbolName = "someLabel_" + std::to_string(m_image_base + offset);
		m_symbols[offset] = Symbol(newSymbolName, 0, SymbolType::Label, true);
		std::cout << "Adding new symbol: " << newSymbolName << " at 0x" << std::hex << offset << std::dec << std::endl;

		ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL);
		ZyanString* string;
		ZydisFormatterBufferGetString(buffer, &string);

		return ZyanStringAppendFormat(string, "%s", newSymbolName.c_str());
	}
}

ZyanStatus CustomInstructionFormatter::hook_print_address_abs_thunk(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	CustomInstructionFormatter* user_data = ((CustomUserData*)context->user_data)->formatter;
	return user_data->hook_print_address_abs(formatter, buffer, context);
}

ZyanStatus CustomInstructionFormatter::hook_print_address_rel(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	// Todo: this could be horribly wrong, depends on what calcabsaddress does...
	ZyanU64 address;
	ZydisCalcAbsoluteAddress(context->instruction, context->operand, context->runtime_address, &address);

	ZyanU64 offset = address - m_image_base;

	if(m_symbols.contains(offset))
	{
		const Symbol& symbol = m_symbols.at(offset);
		ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL);
		ZyanString* string;
		ZydisFormatterBufferGetString(buffer, &string);

		return ZyanStringAppendFormat(string, "<%s>", symbol.getName().c_str());
	}
	else
	{
		std::string newSymbolName = "someLabel_" + std::to_string(m_image_base + offset);
		m_symbols[offset] = Symbol(newSymbolName, 0, SymbolType::Label, true);

		std::cout << "Adding new symbol: " << newSymbolName << " at 0x" << std::hex << offset << std::dec << std::endl;

		ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL);
		ZyanString* string;
		ZydisFormatterBufferGetString(buffer, &string);
		
		return ZyanStringAppendFormat(string, "%s", newSymbolName.c_str());
	}

	return m_print_address_rel(formatter, buffer, context);

}

ZyanStatus CustomInstructionFormatter::hook_print_address_rel_thunk(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	CustomInstructionFormatter* user_data = ((CustomUserData*)context->user_data)->formatter;
	return user_data->hook_print_address_rel(formatter, buffer, context);
}
