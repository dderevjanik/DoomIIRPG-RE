#include "GameYamlRegistrar.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace editor {

namespace {

std::string readFile(const std::string& path) {
	std::ifstream f(path);
	if (!f) throw std::runtime_error("cannot read " + path);
	std::ostringstream ss; ss << f.rdbuf();
	return ss.str();
}

void writeFile(const std::string& path, const std::string& content) {
	std::ofstream f(path);
	if (!f) throw std::runtime_error("cannot write " + path);
	f << content;
}

// Finds the 0-based line index of a section header like "  levels:" inside
// the `game:` block (indented 2 spaces). Returns -1 if not found.
int findSectionLine(const std::vector<std::string>& lines, const char* header) {
	// Match lines of form "^  <header>:\s*(#.*)?$"
	std::string pat = std::string("^  ") + header + R"(:\s*(#.*)?$)";
	std::regex rx(pat);
	for (size_t i = 0; i < lines.size(); ++i) {
		if (std::regex_match(lines[i], rx)) return int(i);
	}
	return -1;
}

// Within a section starting at `sectionLine`, scans subsequent lines that
// look like "    <int>: <value>" and returns the set of existing integer
// keys plus the index of the LAST such line. Stops at the first line that's
// dedented (indent < 4 spaces) or a blank line followed by another section.
struct SectionScan {
	std::vector<int> keys;
	int lastEntryLine = -1;
};
SectionScan scanIntKeyedSection(const std::vector<std::string>& lines, int sectionLine) {
	SectionScan s;
	std::regex entryRx(R"(^    (\d+):\s.*$)");
	for (size_t i = sectionLine + 1; i < lines.size(); ++i) {
		const std::string& ln = lines[i];
		// End of section: a line that's not indented ≥4 spaces and not blank.
		if (!ln.empty() && ln[0] != ' ') break;

		int leadSpaces = 0;
		while (leadSpaces < (int)ln.size() && ln[leadSpaces] == ' ') ++leadSpaces;
		if (!ln.empty() && leadSpaces < 4) {
			// Another section header at indent 2 — stop.
			if (ln.size() > 2 && ln[2] != ' ' && ln[2] != '#') break;
			continue;  // empty-ish blank line, keep going
		}

		std::smatch m;
		if (std::regex_match(ln, m, entryRx)) {
			s.keys.push_back(std::stoi(m[1].str()));
			s.lastEntryLine = int(i);
		}
	}
	return s;
}

}  // namespace

RegisterResult registerLevelInGameYaml(const std::string& gameYamlPath,
                                       int mapId,
                                       const std::string& levelDir,
                                       const std::string& stringsPath)
{
	RegisterResult r;
	std::string content;
	try { content = readFile(gameYamlPath); }
	catch (const std::exception& e) { r.error = e.what(); return r; }

	// Split into lines (preserving line endings would be ideal; for simplicity
	// we split on '\n' and re-emit with '\n').
	std::vector<std::string> lines;
	{
		std::istringstream ss(content);
		std::string ln;
		while (std::getline(ss, ln)) lines.push_back(ln);
	}

	// --- levels: ---
	int levelsLine = findSectionLine(lines, "levels");
	if (levelsLine < 0) { r.error = "no `  levels:` section in " + gameYamlPath; return r; }
	SectionScan levelsScan = scanIntKeyedSection(lines, levelsLine);
	bool hasLevel = std::find(levelsScan.keys.begin(), levelsScan.keys.end(), mapId)
	              != levelsScan.keys.end();
	if (!hasLevel) {
		std::string entry = "    " + std::to_string(mapId) + ": " + levelDir;
		int insertAt = (levelsScan.lastEntryLine >= 0) ? levelsScan.lastEntryLine + 1
		                                               : levelsLine + 1;
		lines.insert(lines.begin() + insertAt, entry);
		r.levelAppended = true;
	}

	// --- strings: ---  (indices are GROUP ids — pick max+1.)
	// Re-scan because indices shifted if we inserted above.
	int stringsLine = findSectionLine(lines, "strings");
	if (stringsLine < 0) { r.error = "no `  strings:` section"; return r; }
	SectionScan stringsScan = scanIntKeyedSection(lines, stringsLine);

	// Has our stringsPath already? Scan text for an entry matching it.
	bool hasStrings = false;
	int existingIdx = -1;
	for (size_t i = stringsLine + 1; i < lines.size(); ++i) {
		if (lines[i].find(stringsPath) != std::string::npos) {
			std::regex m(R"(^    (\d+):\s+(.*)$)");
			std::smatch sm;
			if (std::regex_match(lines[i], sm, m)) {
				existingIdx = std::stoi(sm[1].str());
				hasStrings = true;
			}
			break;
		}
	}

	if (!hasStrings) {
		int next = stringsScan.keys.empty() ? 0
		          : (*std::max_element(stringsScan.keys.begin(), stringsScan.keys.end()) + 1);
		std::string entry = "    " + std::to_string(next) + ": " + stringsPath;
		int insertAt = (stringsScan.lastEntryLine >= 0) ? stringsScan.lastEntryLine + 1
		                                                : stringsLine + 1;
		lines.insert(lines.begin() + insertAt, entry);
		r.stringsAppended = true;
		r.stringsGroupIndex = next;
	} else {
		r.stringsGroupIndex = existingIdx;
	}

	// Reassemble.
	std::string out;
	for (size_t i = 0; i < lines.size(); ++i) {
		out += lines[i];
		if (i + 1 < lines.size()) out += '\n';
	}
	// Preserve trailing newline if the original had one.
	if (!content.empty() && content.back() == '\n' && !out.empty() && out.back() != '\n') {
		out += '\n';
	}

	try { writeFile(gameYamlPath, out); }
	catch (const std::exception& e) { r.error = e.what(); return r; }

	r.ok = true;
	return r;
}

}  // namespace editor
