#pragma once
#include <Zydis/Formatter.h>

#include "PDBFile.h"

class CustomInstructionFormatter
{
public:
	CustomInstructionFormatter(uint64_t image_base, std::unordered_map<uint64_t, Symbol>& symbolMap);
	const ZydisFormatter* getFormatter() const { return &m_formatter; }

private:
	uint64_t m_image_base;
	std::unordered_map<uint64_t, Symbol>& m_symbols;
	ZydisFormatter m_formatter;

	void initialize();
	void setupHooks();


	ZydisFormatterFunc m_format_operand_imm;
	ZydisFormatterFunc m_print_address_abs;
	ZydisFormatterFunc m_print_address_rel;

	ZyanStatus hook_format_operand_imm(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);
	static ZyanStatus hook_format_operand_imm_thunk(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);

	ZyanStatus hook_print_address_abs(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);
	static ZyanStatus hook_print_address_abs_thunk(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);

	ZyanStatus hook_print_address_rel(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);
	static ZyanStatus hook_print_address_rel_thunk(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);
};
