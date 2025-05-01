#include "json_converter.h"
#include <fstream>
#include <sstream>

bool to_json(const FileOperators& files, const std::string& outputFilename) {
    std::ostringstream oss;
    oss << "[\n";

    for (size_t f = 0; f < files.size(); ++f) {
        const auto& filePair = files[f];
        const std::string& filename = filePair.first;
        const std::vector<OperatorData>& ops = filePair.second;

        oss << "  [\n";
        oss << "    \"" << filename << "\",\n";

        for (size_t i = 0; i < ops.size(); ++i) {
            const auto& op = ops[i];
            oss << "    [ " << op.address << ", \"" << op.operatorName << "\"";
            for (const auto& arg : op.args) {
                if (arg.isString) {
                    oss << ", \"" << arg.value << "\"";
                }
                else {
                    oss << ", " << arg.value;
                }
            }
            oss << " ]";
            if (i + 1 < ops.size()) oss << ",";
            oss << "\n";
        }

        oss << "  ]";
        if (f + 1 < files.size()) oss << ",";
        oss << "\n";
    }

    oss << "]\n";

    std::ofstream out(outputFilename, std::ios::binary);
    if (!out) return false;
    out << oss.str();
    out.close();
    return true;
}

bool from_json(const std::string& inputFilename, FileOperators& files) {
    files.clear();
    std::ifstream in(inputFilename, std::ios::binary);
    if (!in) return false;

    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t i = 0, len = json.size();

    auto skip_space = [&]() {
        while (i < len && (json[i] == ' ' || json[i] == '\n' || json[i] == '\r' || json[i] == '\t')) ++i;
        };

    auto read_string = [&]() -> std::string {
        skip_space();
        if (i >= len || json[i] != '"') return "";
        ++i;
        std::string result;
        while (i < len && json[i] != '"') {
            if (json[i] == '\\' && (i + 1) < len) {
                result += json[i + 1];
                i += 2;
            }
            else {
                result += json[i++];
            }
        }
        if (i < len && json[i] == '"') ++i;
        return result;
        };

    auto read_number_string = [&]() -> std::string {
        skip_space();
        std::string result;
        while (i < len && ((json[i] >= '0' && json[i] <= '9') || json[i] == '-' || json[i] == '+')) {
            result += json[i++];
        }
        return result;
        };

    skip_space();
    if (i >= len || json[i] != '[') return false;
    ++i;

    while (true) {
        skip_space();
        if (i >= len) break;
        if (json[i] == ']') {
            ++i;
            break;
        }
        if (json[i] != '[') return false;
        ++i;

        std::string filename = read_string();
        skip_space();
        if (i >= len || json[i] != ',') return false;
        ++i;

        std::vector<OperatorData> ops;
        while (true) {
            skip_space();
            if (i >= len) return false;
            if (json[i] == ']') {
                ++i;
                break;
            }
            if (json[i] != '[') return false;
            ++i;

            OperatorData op;
            op.address = std::stoul(read_number_string());
            skip_space();
            if (i >= len || json[i] != ',') return false;
            ++i;
            op.operatorName = read_string();

            while (true) {
                skip_space();
                if (i >= len) return false;
                if (json[i] == ']') {
                    ++i;
                    break;
                }
                if (json[i] != ',') return false;
                ++i;
                skip_space();
                OperatorArgument arg;
                if (i < len && json[i] == '"') {
                    arg.isString = true;
                    arg.value = read_string();
                }
                else {
                    arg.isString = false;
                    arg.value = read_number_string();
                }
                op.args.push_back(arg);
            }

            ops.push_back(op);

            skip_space();
            if (i < len && json[i] == ',') ++i;
        }

        files.push_back(std::make_pair(filename, ops));

        skip_space();
        if (i < len && json[i] == ',') ++i;
    }

    in.close();
    return true;
}