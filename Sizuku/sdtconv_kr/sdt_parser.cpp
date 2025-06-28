#include "sdt_parser.h"
#include "scripts_struct.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <io.h>
#include <algorithm>

extern const CharactorInfo g_charactorList[];

namespace {
    bool is_directory(const std::string& path) {
        struct stat s;
        if (stat(path.c_str(), &s) == 0) {
            return (s.st_mode & S_IFDIR) != 0;
        }
        return false;
    }

    std::vector<std::string> find_sdt_files(const std::string& folder) {
        std::vector<std::string> files;
        std::string pattern = folder + "\\*.SDT";

        struct _finddata_t c_file;
        intptr_t hFile = _findfirst(pattern.c_str(), &c_file);
        if (hFile == -1L) {
            return files;
        }

        do {
            files.push_back(folder + "\\" + c_file.name);
        } while (_findnext(hFile, &c_file) == 0);

        _findclose(hFile);
        return files;
    }

    std::string find_charactor_name(int code) {
        for (int i = 0; g_charactorList[i].code != -1; ++i) {
            if (g_charactorList[i].code == code) {
                return g_charactorList[i].name;
            }
        }
        return "";
    }
}

void SdtParser::reset() {
    parsedFiles.clear();
    parsedStrings.clear();
    currentStringId = 1;
    currentCharactorName.clear();
}

bool SdtParser::parseFolder(const std::string& folderPath) {
    reset();
    auto files = find_sdt_files(folderPath);
    if (files.empty()) {
        std::cerr << "[오류] SDT 파일을 찾을 수 없음.\n";
        return false;
    }

    for (const auto& file : files) {
        if (!parseFile(file)) {
            std::cerr << "[경고] 파일 파싱 실패: " << file << "\n";
        }
    }
    return true;
}

bool SdtParser::parseFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "[오류] 파일 열기 실패: " << filepath << "\n";
        return false;
    }
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0) {
        std::cerr << "[정보] 비어있는 파일: " << filepath << ".sdt\n";
        return true;
    }
    file.seekg(0, std::ios::beg);

    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of('.');
    std::string filenameOnly = filepath.substr(
        lastSlash != std::string::npos ? lastSlash + 1 : 0,
        (lastDot != std::string::npos ? lastDot : filepath.length()) - (lastSlash != std::string::npos ? lastSlash + 1 : 0)
    );

    ScriptHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file) {
        std::cerr << "[오류] 헤더 읽기 실패.\n";
        return false;
    }

    ParsedSdtFile parsed;
    parsed.filename = filenameOnly;

    while (file.peek() != EOF) {
        std::streampos pos = file.tellg();
        uint32_t address = static_cast<uint32_t>(pos) - static_cast<uint32_t>(sizeof(ScriptHeader));

        uint16_t opcode = 0;
        file.read(reinterpret_cast<char*>(&opcode), sizeof(opcode));
        if (!file) break;

        if (!(((static_cast<uint32_t>(ScriptOperator::Start) < opcode) && (opcode < static_cast<uint32_t>(ScriptOperator::OpEnd))) ||
            ((static_cast<uint32_t>(ScriptOperator::B) <= opcode) && (opcode < static_cast<uint32_t>(ScriptOperator::OpMax))))) {
            std::cerr << "[경고] 부적절한 오퍼레이터 코드. [" << filenameOnly << ":" << address << "]\n";
            break;
        }

        ParsedOperator parsedOp;
        parsedOp.address = address;
        parsedOp.name = g_operatorList[opcode].name;

        const ScriptOperatorFormat& format = g_operatorFormats[opcode];

        for (int paramIndex = 0; paramIndex < 15; ++paramIndex) {
            if (format.args[paramIndex] == ScriptArgumentType::NotUsed) {
                break;
            }

            OperatorParam param;
            ScriptArgumentType argType = format.args[paramIndex];

            switch (argType) {
            case ScriptArgumentType::AsciiChar:
            case ScriptArgumentType::Register:
            case ScriptArgumentType::AddValue:
            case ScriptArgumentType::CharValue: {
                uint8_t temp = 0;
                file.read(reinterpret_cast<char*>(&temp), sizeof(temp));
                param.valueType = 1;
                param.intValue = temp;
                parsedOp.params.push_back(param);
                break;
            }
            case ScriptArgumentType::Count:
            case ScriptArgumentType::VariableCount:
            case ScriptArgumentType::ShortValue: {
                uint16_t temp = 0;
                file.read(reinterpret_cast<char*>(&temp), sizeof(temp));
                param.valueType = 1;
                param.intValue = temp;
                parsedOp.params.push_back(param);
                break;
            }
            case ScriptArgumentType::LongValue: {
                int32_t temp = 0;
                file.read(reinterpret_cast<char*>(&temp), sizeof(temp));
                param.valueType = 1;
                param.intValue = temp;
                parsedOp.params.push_back(param);
                break;
            }
            case ScriptArgumentType::Number: {
                uint8_t mode = 0;
                file.read(reinterpret_cast<char*>(&mode), sizeof(mode));
                if (mode == 1) {
                    int32_t num = 0;
                    file.read(reinterpret_cast<char*>(&num), sizeof(num));
                    param.valueType = 1;
                    param.intValue = num;
                }
                else if (mode == 2) {
                    uint8_t length = 0;
                    file.read(reinterpret_cast<char*>(&length), sizeof(length));
                    if (length > 0 && length < 256) {
                        file.read(param.strValue, length);
                        param.strValue[length] = '\0';
                        param.valueType = 2;
                    }
                    else {
                        file.seekg(length, std::ios::cur);
                        param.valueType = 2;
                        param.strValue[0] = '\0';
                    }
                }
                else if (mode == 0) {
                    uint32_t num = 0;
                    file.read(reinterpret_cast<char*>(&num), sizeof(num));
                    param.valueType = 0;
                    param.uintValue = num;
                }
                parsedOp.params.push_back(param);
                break;
            }
            case ScriptArgumentType::Compare: {
                uint8_t ch1 = 0, ch2 = 0, mode = 0;
                file.read(reinterpret_cast<char*>(&ch1), sizeof(ch1));
                file.read(reinterpret_cast<char*>(&ch2), sizeof(ch2));
                file.read(reinterpret_cast<char*>(&mode), sizeof(mode));

                OperatorParam param1, param2, param3;
                param1.valueType = 1;
                param1.intValue = ch1;
                param2.valueType = 1;
                param2.intValue = ch2;

                if (mode == 1) {
                    int32_t num = 0;
                    file.read(reinterpret_cast<char*>(&num), sizeof(num));
                    param3.valueType = 1;
                    param3.intValue = num;
                }
                else if (mode == 2) {
                    uint8_t length = 0;
                    file.read(reinterpret_cast<char*>(&length), sizeof(length));
                    if (length > 0 && length < 256) {
                        file.read(param3.strValue, length);
                        param3.strValue[length] = '\0';
                        param3.valueType = 2;
                    }
                    else {
                        file.seekg(length, std::ios::cur);
                        param3.valueType = 2;
                        param3.strValue[0] = '\0';
                    }
                }
                else if (mode == 0) {
                    uint32_t num = 0;
                    file.read(reinterpret_cast<char*>(&num), sizeof(num));
                    param3.valueType = 0;
                    param3.uintValue = num;
                }

                parsedOp.params.push_back(param1);
                parsedOp.params.push_back(param2);
                parsedOp.params.push_back(param3);
                break;
            }
            case ScriptArgumentType::String:
            case ScriptArgumentType::String2: {
                uint16_t strLength = 0;
                if (argType == ScriptArgumentType::String) {
                    uint8_t len8 = 0;
                    file.read(reinterpret_cast<char*>(&len8), sizeof(len8));
                    strLength = len8;
                }
                else {
                    file.read(reinterpret_cast<char*>(&strLength), sizeof(strLength));
                }

                if ((argType == ScriptArgumentType::String && strLength >= 0 && strLength < 256) ||
                    (argType == ScriptArgumentType::String2 && strLength >= 0)) {

                    std::vector<char> buffer(strLength);
                    file.read(buffer.data(), strLength);

                    if (parsedOp.name == "SetMessage" || parsedOp.name == "SetMessage2" ||
                        parsedOp.name == "SetMessageEx" || parsedOp.name == "AddMessage" ||
                        parsedOp.name == "AddMessage2" || parsedOp.name == "SetSelectMes" ||
                        parsedOp.name == "SetSelectMesEx") {

                        ParsedString newStr;
                        newStr.filename = filenameOnly;
                        newStr.id = currentStringId++;

                        if (parsedOp.name == "SetSelectMes" || parsedOp.name == "SetSelectMesEx") {
                            newStr.charactorName = "선택지";
                        }
                        else {
                            newStr.charactorName = currentCharactorName.empty() ? "" : currentCharactorName;
                            currentCharactorName.clear();
                        }
                        newStr.originalText = std::string(buffer.begin(), buffer.end());
                        //newStr.translatedText = newStr.originalText;
                        parsedStrings.push_back(newStr);

                        param.valueType = 1;
                        param.intValue = newStr.id;
                        parsedOp.params.push_back(param);
                    }
                    else {
                        param.valueType = 2;
                        size_t copyLen = std::min<size_t>(strLength, 255);
                        std::memcpy(param.strValue, buffer.data(), copyLen);
                        param.strValue[copyLen] = '\0';
                        parsedOp.params.push_back(param);
                    }
                }
                else {
                    file.seekg(strLength, std::ios::cur);
                }
                break;
            }
            default:
                break;
            }

            /* 캐릭터 이름 설정 (키즈아토)
            if (parsedOp.name == "VV" && paramIndex == 0) {
                currentCharactorName = find_charactor_name(param.intValue);
            }*/

            /* 캐릭터 이름 설정 (Routes)
            if (parsedOp.name == "V" && paramIndex == 1) {
                currentCharactorName = find_charactor_name(param.intValue / 100);
            }
            if ((parsedOp.name == "C" || parsedOp.name == "SetChar" || parsedOp.name == "CWR" || parsedOp.name == "CW") && paramIndex == 0) {
                currentCharactorName = find_charactor_name(param.intValue);
            }*/
        }

        parsed.ops.push_back(parsedOp);
    }

    parsedFiles.push_back(parsed);
    return true;
}

const std::vector<ParsedSdtFile>& SdtParser::getParsedSdts() const {
    return parsedFiles;
}

const std::vector<ParsedString>& SdtParser::getParsedStrings() const {
    return parsedStrings;
}