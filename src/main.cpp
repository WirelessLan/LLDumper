#include <fstream>

void WriteToFile(const std::string& jsonString, const std::string& filename) {
	std::ofstream outFile(filename);
	if (!outFile.is_open())
		return;

	outFile << jsonString;
	outFile.close();
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

void PrintForm(std::ostringstream& oss, RE::TESForm* a_form, std::uint32_t a_indent, bool a_comma, bool a_newLine) {
	std::string indent;
	for (std::uint32_t ii = 0; ii < a_indent; ii++)
		indent += "\t";

	oss << indent << "\"EditorID\": \"" << a_form->GetFormEditorID() << "\",\n"
		<< indent << "\"Plugin\": \"" << a_form->sourceFiles.array->front()->filename << "\",\n"
		<< indent << "\"FormID\": \"" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << a_form->formID << "\"";
	oss << std::dec << std::nouppercase;

	if (a_comma)
		oss << ",";

	if (a_newLine)
		oss << "\n";
}

void PrintEntry(std::ostringstream& oss, RE::LEVELED_OBJECT* a_entry, std::uint32_t a_indent) {
	std::string indent;
	for (std::uint32_t ii = 0; ii < a_indent; ii++)
		indent += "\t";

	oss << indent << "{\n"
		<< indent << "\t\"Level\": " << static_cast<std::uint32_t>(a_entry->level) << ",\n"
		<< indent << "\t\"Reference\": {\n";
	PrintForm(oss, a_entry->form, a_indent + 2, false, true);
	oss << indent << "\t},\n"
		<< indent << "\t\"Count\": " << a_entry->count << ",\n"
		<< indent << "\t\"ChanceNone\": " << static_cast<std::uint32_t>(a_entry->chanceNone) << "\n";

	oss << indent << "}";
}

void Dump(std::monostate, RE::TESForm* a_form) {
	if (!a_form) {
		RE::ConsoleLog::GetSingleton()->PrintLine("Form is null.");
		return;
	}

	RE::TESLeveledList* leveledList = a_form->As<RE::TESLeveledList>();
	if (!leveledList) {
		RE::ConsoleLog::GetSingleton()->PrintLine("Form is not a leveled list.");
		return;
	}

	std::ostringstream oss;

	oss << "{\n";

	PrintForm(oss, a_form, 1, true, true);

	oss << "\t\"BaseEntries\": [\n";
	for (std::uint32_t ii = 0; ii < static_cast<std::uint32_t>(leveledList->baseListCount); ii++) {
		PrintEntry(oss, &leveledList->leveledLists[ii], 2);
		if (ii < static_cast<std::uint32_t>(leveledList->baseListCount) - 1)
			oss << ",";
		oss << "\n";
	}
	oss << "\t],\n";

	oss << "\t\"ScriptAddedEntries\": [\n";
	for (std::uint32_t ii = 0; ii < static_cast<std::uint32_t>(leveledList->scriptListCount); ii++) {
		PrintEntry(oss, leveledList->scriptAddedLists[ii], 2);
		if (ii < static_cast<std::uint32_t>(leveledList->scriptListCount) - 1)
			oss << ",";
		oss << "\n";
	}
	oss << "\t]\n";

	oss << "}\n";

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
