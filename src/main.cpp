#include <fstream>

void WriteToFile(const std::string& a_json, const std::string& a_filename) {
	std::ofstream outFile(a_filename);
	if (!outFile.is_open())
		return;

	outFile << a_json;
	outFile.close();
}

std::string GetIndent(std::uint32_t a_indentLevel = 1) {
	std::string indent;
	for (std::uint32_t ii = 0; ii < a_indentLevel; ii++)
		indent += "\t";
	return indent;
}

std::string GetCurrentTimeString() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);

	std::tm now_tm = {};
	localtime_s(&now_tm, &now_c);

	std::ostringstream oss;
	oss << std::put_time(&now_tm, "%Y%m%d%H%M%S");

	return oss.str();
}

void PrintForm(std::ostringstream& a_oss, RE::TESForm* a_form, std::uint32_t a_indentLevel, bool a_comma) {
	std::string indent = GetIndent(a_indentLevel);

	a_oss << indent << "\"EditorID\": \"" << a_form->GetFormEditorID() << "\",\n"
		<< indent << "\"Plugin\": \"" << a_form->sourceFiles.array->front()->filename << "\",\n"
		<< indent << "\"FormID\": \"" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << a_form->formID << "\"";
	a_oss << std::dec << std::nouppercase;

	if (a_comma)
		a_oss << ",";

	a_oss << "\n";
}

void PrintLeveledListEntry(std::ostringstream& a_oss, RE::LEVELED_OBJECT* a_entry, std::uint32_t a_indentLevel, bool a_comma) {
	std::string indent = GetIndent(a_indentLevel);

	a_oss << indent << "{\n"
		<< indent << GetIndent() << "\"Level\": " << static_cast<std::uint32_t>(a_entry->level) << ",\n"
		<< indent << GetIndent() << "\"Reference\": {\n";

	PrintForm(a_oss, a_entry->form, a_indentLevel + 2, false);

	a_oss << indent << GetIndent() << "},\n"
		<< indent << GetIndent() << "\"Count\": " << a_entry->count << ",\n"
		<< indent << GetIndent() << "\"ChanceNone\": " << static_cast<std::uint32_t>(a_entry->chanceNone) << "\n";

	a_oss << indent << "}";

	if (a_comma)
		a_oss << ",";

	a_oss << "\n";
}

void PrintLeveledList(std::ostringstream& a_oss, RE::TESForm* a_levForm, std::uint32_t a_indentLevel, bool a_comma) {
	RE::TESLeveledList* levList = a_levForm->As<RE::TESLeveledList>();
	if (!levList)
		return;

	std::string indent = GetIndent(a_indentLevel);

	a_oss << indent << "{\n";

	PrintForm(a_oss, a_levForm, a_indentLevel + 1, true);

	a_oss << indent << GetIndent() << "\"ChanceNone\": " << static_cast<std::uint32_t>(levList->chanceNone) << ",\n";
	a_oss << indent << GetIndent() << "\"MaxCount\": " << static_cast<std::uint32_t>(levList->maxUseAllCount) << ",\n";

	a_oss << indent << GetIndent() << "\"Flags\": [\n";
	for (std::uint8_t ii = 1; ii < 8; ii *= 2) {
		if (!(levList->llFlags & ii))
			continue;

		std::string_view flagStr;

		switch (ii) {
		case 0x1:
			flagStr = "Calculate from all levels <= player's level";
			break;
		case 0x2:
			flagStr = "Calculate for each item in count";
			break;
		case 0x4:
			flagStr = "Use All";
			break;
		case 0x8:
			flagStr = "Unknown 3";
			break;
		case 0x10:
			flagStr = "Unknown 4";
			break;
		case 0x20:
			flagStr = "Unknown 5";
			break;
		case 0x40:
			flagStr = "Unknown 6";
			break;
		case 0x80:
			flagStr = "Unknown 7";
			break;
		}

		a_oss << indent << GetIndent(2) << "\"" << flagStr << "\"";

		if (levList->llFlags > ii * 2)
			a_oss << ",";

		a_oss << "\n";
	}
	a_oss << indent << GetIndent() << "],\n";

	if (levList->chanceGlobal) {
		a_oss << indent << GetIndent() << "\"UseGlobal\": {\n";
		PrintForm(a_oss, levList->chanceGlobal, a_indentLevel + 2, false);
		a_oss << indent << GetIndent() << "},\n";
	}

	a_oss << indent << GetIndent() << "\"BaseEntryCount\": " << static_cast<std::uint32_t>(levList->baseListCount) << ",\n"
		<< indent << GetIndent() << "\"BaseEntries\": [\n";
	for (std::uint32_t ii = 0; ii < static_cast<std::uint32_t>(levList->baseListCount); ii++) {
		if (ii < static_cast<std::uint32_t>(levList->baseListCount) - 1)
			PrintLeveledListEntry(a_oss, &levList->leveledLists[ii], a_indentLevel + 2, true);
		else
			PrintLeveledListEntry(a_oss, &levList->leveledLists[ii], a_indentLevel + 2, false);
	}
	a_oss << indent << GetIndent() << "],\n"
		<< indent << GetIndent() << "\"ScriptAddedEntryCount\": " << static_cast<std::uint32_t>(levList->scriptListCount) << ",\n"
		<< indent << GetIndent() << "\"ScriptAddedEntries\": [\n";
	for (std::uint32_t ii = 0; ii < static_cast<std::uint32_t>(levList->scriptListCount); ii++) {
		if (ii < static_cast<std::uint32_t>(levList->scriptListCount) - 1)
			PrintLeveledListEntry(a_oss, levList->scriptAddedLists[ii], a_indentLevel + 2, true);
		else
			PrintLeveledListEntry(a_oss, levList->scriptAddedLists[ii], a_indentLevel + 2, false);
	}
	a_oss << indent << GetIndent() << "]\n"
		<< indent << "}";

	if (a_comma)
		a_oss << ",";

	a_oss << "\n";
}

void PrintFormList(std::ostringstream& a_oss, RE::TESForm* a_frmListForm, std::uint32_t a_indentLevel, bool a_comma) {
	RE::BGSListForm* frmList = a_frmListForm->As<RE::BGSListForm>();
	if (!frmList)
		return;

	std::string indent = GetIndent(a_indentLevel);

	a_oss << indent << "{\n";

	PrintForm(a_oss, a_frmListForm, a_indentLevel + 1, true);

	a_oss << indent << GetIndent() << "\"FormCount\": " << frmList->arrayOfForms.size() << ",\n"
		<< indent << GetIndent() << "\"Forms\": [\n";
	for (std::uint32_t ii = 0; ii < frmList->arrayOfForms.size(); ii++) {
		a_oss << indent << GetIndent(2) << "{\n";
		PrintForm(a_oss, frmList->arrayOfForms[ii], a_indentLevel + 3, false);
		a_oss << indent << GetIndent(2) << "}";
		if (ii < frmList->arrayOfForms.size() - 1)
			a_oss << ",";
		a_oss << "\n";
	}
	a_oss << indent << GetIndent() << "]\n"
		<< indent << "}";

	if (a_comma)
		a_oss << ",";

	a_oss << "\n";

}

void Dump(std::monostate, RE::TESForm* a_form) {
	if (!a_form) {
		RE::ConsoleLog::GetSingleton()->PrintLine("Form is null.");
		return;
	}

	std::ostringstream oss;

	if (a_form->formType == RE::ENUM_FORM_ID::kLVLI || a_form->formType == RE::ENUM_FORM_ID::kLVLN || a_form->formType == RE::ENUM_FORM_ID::kLVSP)
		PrintLeveledList(oss, a_form, 0, false);
	else if (a_form->formType == RE::ENUM_FORM_ID::kFLST)
		PrintFormList(oss, a_form, 0, false);
	else {
		RE::ConsoleLog::GetSingleton()->PrintLine("Form is not a Leveled List or a FormID List.");
		return;
	}

	std::string filePath = "Data\\F4SE\\Plugins\\LLDumper\\LLDump_" + GetCurrentTimeString() + ".json";

	WriteToFile(oss.str(), filePath);

	RE::ConsoleLog::GetSingleton()->PrintLine("Dumped at %s", filePath.c_str());
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm) {
	a_vm->BindNativeMethod("LLDumper"sv, "Dump"sv, Dump);
	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface * a_f4se, F4SE::PluginInfo * a_info) {
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface * a_f4se) {
	F4SE::Init(a_f4se);

	const F4SE::PapyrusInterface* papyrus = F4SE::GetPapyrusInterface();
	if (papyrus)
		papyrus->Register(RegisterPapyrusFunctions);

	return true;
}
