#include "sdt_builder.h"
#include "scripts_struct.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

bool SdtBuilder::load(const std::string& jsonPath, const std::string& csvPath) {
    std::cout << "JSON 파일 파싱 중...\n";
    if (from_json(jsonPath, fileOperators)) {
        std::cout << "JSON 파일 파싱 완료\nCSV 파일 파싱 중...\n";
        if (from_csv(csvPath, csvStrings)) {
            std::cout << "CSV 파일 파싱 완료\n";
            return true;
        }
    }
    return false;
}

bool SdtBuilder::build(const SdtBuildOptions& options) {
    results.clear();
    std::cout << "SDT 파일 빌드 중...\n";

    for (size_t i = 0; i < fileOperators.size(); ++i) {
        const std::string& filename = fileOperators[i].first;
        std::vector<OperatorData> ops = fileOperators[i].second;

        std::map<uint32_t, uint32_t> addrMap;
        uint32_t currentOffset = 0;

        // 주소 매핑 계산
        for (size_t j = 0; j < ops.size(); ++j) {
            addrMap[ops[j].address] = currentOffset;
            currentOffset += calculateOperatorSize(ops[j]);
        }

        // 분기 오퍼레이터 주소 업데이트
        updateBranchAddresses(ops, addrMap);

        std::vector<uint8_t> opData;
        for (size_t j = 0; j < ops.size(); ++j) {
            std::vector<uint8_t> encoded = encodeOperator(ops[j]);
            opData.insert(opData.end(), encoded.begin(), encoded.end());
        }

        // 헤더 구성
        std::vector<uint8_t> buffer;
        ScriptHeader header;
        header.header1 = 0x004C;
        header.header2 = 0x0046;
        header.totalFileSize = sizeof(ScriptHeader) + (int)opData.size();
        memset(header.blockOffsets, 0, sizeof(header.blockOffsets));
        header.blockOffsets[0] = 1;

        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&header),
            reinterpret_cast<uint8_t*>(&header) + sizeof(ScriptHeader));
        buffer.insert(buffer.end(), opData.begin(), opData.end());

        BuiltSdtFile file;
        file.filename = filename;
        file.binary = buffer;
        results.push_back(file);
    }
    std::cout << "SDT 파일 빌드 완료\n";

    return true;
}

bool SdtBuilder::save(const std::string& outputFolder) {
#ifdef _WIN32
    _mkdir(outputFolder.c_str());
#else
    mkdir(outputFolder.c_str(), 0755);
#endif

    for (size_t i = 0; i < results.size(); ++i) {
        std::string outPath = outputFolder + "/" + results[i].filename + ".SDT";
        std::ofstream out(outPath.c_str(), std::ios::binary);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(results[i].binary.data()), results[i].binary.size());
    }
    return true;
}

const std::vector<BuiltSdtFile>& SdtBuilder::getResults() const {
    return results;
}

std::string SdtBuilder::findTranslatedString(int id) {
    for (size_t i = 0; i < csvStrings.size(); ++i) {
        if (csvStrings[i].id == id) {
            return csvStrings[i].translated;
        }
    }
    return "";
}

void SdtBuilder::updateBranchAddresses(std::vector<OperatorData>& ops, const std::map<uint32_t, uint32_t>& addrMap) {
    for (size_t i = 0; i < ops.size(); ++i) {
        const std::string& name = ops[i].operatorName;
        if (name == "Goto") {
            if (ops[i].args.size() > 0) {
                int oldAddr = std::stoi(ops[i].args[0].value);
                if (addrMap.find(oldAddr) != addrMap.end()) {
                    ops[i].args[0].value = std::to_string(addrMap.at(oldAddr));
                }
                else {
                    std::cerr << "[경고] 잘못된 주소 값: [ \"" << name << "\", " <<
                        ops[i].args[0].value << "]\n";
                }
            }
        }
        else if (name == "IfR" || name == "IfV" || name == "IfElseR" || name == "IfElseV") {
            if (ops[i].args.size() > 3) {
                int oldAddr = std::stoi(ops[i].args[3].value);
                if (addrMap.find(oldAddr) != addrMap.end()) {
                    ops[i].args[3].value = std::to_string(addrMap.at(oldAddr));
                }
                else {
                    std::cerr << "[경고] 잘못된 주소 값: [ \"" << name  << "\", " << 
                        ops[i].args[0].value << ", " <<
                        ops[i].args[1].value << ", " <<
                        ops[i].args[2].value << ", " <<
                        ops[i].args[3].value << " ]\n";
                }
            }
        }
    }
}

uint32_t SdtBuilder::calculateOperatorSize(const OperatorData& op) {
    uint32_t size = 2; // opcode + argcode

    for (size_t i = 0; i < op.args.size(); ++i) {
        const OperatorArgument& arg = op.args[i];
        const ScriptOperatorFormat* fmt = &g_operatorFormats[static_cast<int>(getOperatorCode(op.operatorName))];
        ScriptArgumentType argType = fmt->args[i];

        switch (argType) {
        case ScriptArgumentType::Number:
            size += 1; // mode
            if (arg.valueType == 2) size += 1 + (uint32_t)arg.value.size(); // mode 2
            else size += 4; // mode 0 or 1
            break;
        case ScriptArgumentType::String:
        case ScriptArgumentType::String2:
            if (op.operatorName == "SetMessage" || op.operatorName == "SetMessage2" || op.operatorName == "SetMessageEx" ||
                op.operatorName == "AddMessage" || op.operatorName == "AddMessage2" ||
                op.operatorName == "SetSelectMes" || op.operatorName == "SetSelectMesEx") {
                int id = std::atoi(arg.value.c_str());
                std::string s = findTranslatedString(id);
                size += (argType == ScriptArgumentType::String) ? 1 : 2;
                size += (uint32_t)s.size();

                if (op.operatorName == "SetMessage" || op.operatorName == "SetMessage2" || op.operatorName == "SetMessageEx") {
                    colStatus = 0;
                    rowStatus = 0;
                    isHalf = false;
                }
                else if (op.operatorName == "SetSelectMes" || op.operatorName == "SetSelectMesEx") {
                    colStatus = 0;
                    rowStatus++;
                    isHalf = false;
                }
                calculateFlow(s);
                if (rowMax < rowStatus) {
                    std::cerr << "[경고] 스크립트 행 초과. ID: " << id << "(" << (1 + rowStatus) << "행, " << colStatus << "자)\n";
                }
            }
            else if (argType == ScriptArgumentType::String) {
                size += 1 + (uint32_t)arg.value.size();
            }
            else {
                size += 2 + (uint32_t)arg.value.size();
            }
            break;
        case ScriptArgumentType::Compare:
            size += 1 + 1 + 1;
            if (arg.valueType == 2) size += 1 + (uint32_t)arg.value.size();
            else size += 4;
            break;
        case ScriptArgumentType::Register:
        case ScriptArgumentType::AsciiChar:
        case ScriptArgumentType::AddValue:
        case ScriptArgumentType::CharValue:
            size += 1;
            break;
        case ScriptArgumentType::ShortValue:
        case ScriptArgumentType::Count:
        case ScriptArgumentType::VariableCount:
            size += 2;
            break;
        case ScriptArgumentType::LongValue:
            size += 4;
            break;
        default:
            break;
        }
    }

    return size;
}

const std::vector<uint16_t> SdtBuilder::fBytes = {
    0x20, 0x21, 0x2C, 0x2E, 0x3F, 0x5E,
    0x8140, 0x8141, 0x8142, 0x8148, 0x8149,
    0x816A, 0x8176, 0x8178,
    0xF040, 0xF041, 0xF042, 0xF043, 0xF044, 0xF045
};

std::vector<uint8_t> SdtBuilder::encodeOperator(const OperatorData& op) {
    std::vector<uint8_t> out;
    
    ScriptOperator code = getOperatorCode(op.operatorName);
    const ScriptOperatorFormat* fmt = &g_operatorFormats[static_cast<int>(code)];

    uint16_t opCode = static_cast<uint16_t>(code);
    out.push_back(opCode & 0xFF);
    out.push_back((opCode >> 8) & 0xFF);

    for (size_t i = 0; i < op.args.size(); ++i) {
        const OperatorArgument& arg = op.args[i];
        ScriptArgumentType argType = fmt->args[i];

        switch (argType) {
        case ScriptArgumentType::Number:
            out.push_back(arg.valueType);
            if (arg.valueType == 2) {
                uint8_t len = (uint8_t)arg.value.size();
                out.push_back(len);
                out.insert(out.end(), arg.value.begin(), arg.value.end());
            }
            else {
                int32_t v = std::atoi(arg.value.c_str());
                for (int j = 0; j < 4; ++j)
                    out.push_back((v >> (8 * j)) & 0xFF);
            }
            break;

        case ScriptArgumentType::String:
        case ScriptArgumentType::String2: {
            if (op.operatorName == "SetMessage" || op.operatorName == "SetMessage2" || op.operatorName == "SetMessageEx" ||
                op.operatorName == "AddMessage" || op.operatorName == "AddMessage2" ||
                op.operatorName == "SetSelectMes" || op.operatorName == "SetSelectMesEx") {
                int id = std::atoi(arg.value.c_str());
                std::string s = findTranslatedString(id);
                if (argType == ScriptArgumentType::String) {
                    out.push_back((uint8_t)s.size());
                }
                else {
                    uint16_t len = (uint16_t)s.size();
                    out.push_back(len & 0xFF);
                    out.push_back((len >> 8) & 0xFF);
                }
                out.insert(out.end(), s.begin(), s.end());
            }
            else {
                if (argType == ScriptArgumentType::String) {
                    out.push_back((uint8_t)arg.value.size());
                }
                else {
                    uint16_t len = (uint16_t)arg.value.size();
                    out.push_back(len & 0xFF);
                    out.push_back((len >> 8) & 0xFF);
                }
                out.insert(out.end(), arg.value.begin(), arg.value.end());
            }
            break;
        }

        case ScriptArgumentType::Compare:
            out.push_back(0); // dummy cmp1
            out.push_back(0); // dummy cmp2
            out.push_back(arg.valueType);
            if (arg.valueType == 2) {
                int id = std::atoi(arg.value.c_str());
                std::string s = findTranslatedString(id);
                out.push_back((uint8_t)s.size());
                out.insert(out.end(), s.begin(), s.end());
            }
            else {
                int32_t v = std::atoi(arg.value.c_str());
                for (int j = 0; j < 4; ++j)
                    out.push_back((v >> (8 * j)) & 0xFF);
            }
            break;

        case ScriptArgumentType::Register:
        case ScriptArgumentType::AsciiChar:
        case ScriptArgumentType::AddValue:
        case ScriptArgumentType::CharValue:
            out.push_back((uint8_t)std::atoi(arg.value.c_str()));
            break;

        case ScriptArgumentType::ShortValue:
        case ScriptArgumentType::Count:
        case ScriptArgumentType::VariableCount: {
            uint16_t v = (uint16_t)std::atoi(arg.value.c_str());
            out.push_back(v & 0xFF);
            out.push_back((v >> 8) & 0xFF);
            break;
        }

        case ScriptArgumentType::LongValue: {
            int32_t v = std::atoi(arg.value.c_str());
            for (int j = 0; j < 4; ++j)
                out.push_back((v >> (8 * j)) & 0xFF);
            break;
        }

        default:
            break;
        }
    }

    return out;
}


std::string SdtBuilder::removeControlCodes(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'k') {
            i += 2; // \k 삭제
        }
        else if (s[i] == '<') {
            if (i + 1 < s.size() && s[i + 1] == 'R') {
                // <R문자|루비문자>
                size_t pipe = s.find('|', i + 2);
                if (pipe != std::string::npos) {
                    // 문자 부분 추가
                    out.append(s.substr(i + 2, pipe - (i + 2)));
                    size_t gt = s.find('>', pipe + 1);
                    i = (gt == std::string::npos ? s.size() : gt + 1);
                }
                else {
                    // 형식 이상 시 태그 전체 제거
                    size_t gt = s.find('>', i + 1);
                    i = (gt == std::string::npos ? s.size() : gt + 1);
                }
            }
            else if (i + 1 < s.size() && std::isupper((unsigned char)s[i + 1])) {
                // <영문1자+숫자+문자>
                size_t p = i + 2;
                // 숫자 스킵
                while (p < s.size() && std::isdigit((unsigned char)s[p])) p++;
                // 태그 내부 텍스트 부분 추가
                size_t gt = s.find('>', p);
                if (gt != std::string::npos) {
                    out.append(s.substr(p, gt - p));
                    i = gt + 1;
                }
                else {
                    // '>' 없으면 텍스트 후반부 모두 추가
                    out.append(s.substr(p));
                    break;
                }
            }
            else {
                // 알 수 없는 태그, '<' 문자 자체 추가
                out.push_back(s[i]);
                i++;
            }
        }
        else {
            // 일반 문자
            out.push_back(s[i]);
            i++;
        }
    }
    return out;
}

void SdtBuilder::calculateFlow(const std::string& input) {
    // 제어코드 제거
    std::string proc = removeControlCodes(input);

    // MBCS 여부에 따라 코드 분리
    std::vector<uint16_t> codes;
    codes.reserve(proc.size());
    for (size_t i = 0; i < proc.size();) {
        unsigned char c = static_cast<unsigned char>(proc[i]);
        if (c >= 0x80 && i + 1 < proc.size()) {
            // 두 바이트 문자
            unsigned char c2 = static_cast<unsigned char>(proc[i + 1]);
            uint16_t code = static_cast<uint16_t>(c) << 8 | c2;
            codes.push_back(code);
            i += 2;
        }
        else {
            // 단일 바이트
            codes.push_back(static_cast<uint16_t>(c));
            i += 1;
        }
    }

    // 코드 순회하며 상태 계산
    int i = 0;
    bool isUserFeed = false;
    for (uint16_t code : codes) {

        // 자동 개행 \n 처리
        i++;
        if (isUserFeed) {
            isUserFeed = false;
            continue;
        }

        if ((code == 0x5C) && (static_cast<unsigned char>(codes[i]) == 'n')) {
            isHalf = false;
            if (!(isLinefeed && (colStatus == 0))) {
                rowStatus++;
            }
            isLinefeed = false;
            colStatus = 0;
            isUserFeed = true;
            continue;
        }

        // 반각 vs 전각 처리
        if (code < 0x80) {
            // 반각
            if (!isHalf) {
                colStatus++;
                isHalf = true;
            }
            else {
                isHalf = false;
                // colStatus 변화 없음
            }
        }
        else {
            // 전각
            // isHalf 유지
            colStatus++;
        }

        // colStatus == colMax일 때
        if (colStatus > colMax) {
            if (!(std::find(fBytes.begin(), fBytes.end(), codes[i]) != fBytes.end())) {
                isHalf = false;
                isLinefeed = true;
                colStatus = 0;
                rowStatus++;
                continue;
            }
        }

        // colStatus == 1일 때 특수 처리
        if ((colStatus <= 1) && isLinefeed) {
            if (code == 0x20 || code == 0x8140) {
                isHalf = false;
                colStatus = 0;
            }
        }
    }
}
