#include "DataManager.h"
#include "ScanProcess.h"
#include "StringUtil.h"
#include <sstream> // std::stringstream
#include <algorithm> // std::max, std::transform
#include <iomanip> // std::setw, std::setfill for debugging output
#include <shlwapi.h> // PathCombine, PathAppend 등을 위한 헤더 (PathCchCombine이 더 최신이지만 여기선 PathAppend 사용)
#pragma comment(lib, "Shlwapi.lib") // PathAppend 함수를 위해 Shlwapi.lib 링크

DataManager::DataManager()
    : m_currentMainIndex(-1),
    m_currentSubIndex(0),
    m_status(DM_STATUS::WAIT),
    m_infFilePath(L""),
    m_pakFilePath(L"")
{   // 생성자
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring fullPath(buffer);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    size_t lastDot = fullPath.find_last_of(L".");
    if (std::string::npos != lastSlash) {
        m_executablePath = fullPath.substr(0, lastSlash + 1);
        m_executableName = fullPath.substr(lastSlash + 1, lastDot);
    }
    else {
        m_executablePath = L".\\";
        m_executableName = fullPath.substr(0, lastDot);
    }
}

DataManager::~DataManager() {
    // 소멸자
}

DataManager& DataManager::GetInstance() {
    static DataManager instance;
    return instance;
}


void DataManager::StartDataManagerThread() {
    if (m_dataManagerThread.joinable()) {
        dprintf("Data Manager thread is already running. (DataManager)");
        return;
    }

    // 종료 플래그 초기화
    m_stopDataManagerThread.store(false);

    // std::thread를 사용하여 멤버 함수를 스레드로 실행
    // this 포인터를 람다 캡처 목록으로 전달하여 멤버 함수 호출
    m_dataManagerThread = std::thread([this]() {
        this->ManagementDataLoop();
        });
    dprintf("Start the Data Manager thread. (DataManager)");
}

void DataManager::StopDataManagerThread() {
    if (m_dataManagerThread.joinable()) {
        m_stopDataManagerThread.store(true); // 스레드에 종료 신호
    }
}

void DataManager::WaitStopDataManagerThread() {
    if (m_dataManagerThread.joinable()) {
        m_dataManagerThread.join();         // 스레드 종료 대기
    }
    dprintf("Data Manager thread terminated. (DataManager)");
}

void DataManager::ManagementDataLoop() {
    HANDLE curHandle = NULL;
    std::wstring curBasePath, newInfPath, newPakPath;
    uint32_t curInfTime = 0;
    uint32_t newInfTime = 0;
    curBasePath = L"";

    while (!m_stopDataManagerThread.load()) {
        Sleep(1000); // 1초에 한 번씩 검사

        HANDLE newHnd = ScanProcess::GetInstance().GetTargetProcessHandle();
        if ((newHnd != NULL) && ((curHandle != newHnd) || (m_status != DM_STATUS::LOAD))) {
            Sleep(10);
            std::wstring basePath = ScanProcess::GetInstance().GetTargetProcessPath();
            if (curBasePath != basePath) {
                if ((FindTargetFile(false, basePath, newPakPath)) || (FindTargetFile(false, m_executablePath, newPakPath))) {
                    if ((FindTargetFile(true, basePath, newInfPath)) || (FindTargetFile(true, m_executablePath, newInfPath))) {
                        m_infFilePath = newInfPath;
                        if (GetFileWriteTime(m_infFilePath, newInfTime)) {
                            curInfTime = newInfTime;
                        }
                    }
                    m_pakFilePath = newPakPath;
                    if (LoadPakFileData()) {
                        curHandle = newHnd;
                        curBasePath = basePath;
                    }
                }
            }
        }

        if (m_status == DM_STATUS::LOAD) {
            if (GetFileWriteTime(m_infFilePath, newInfTime) && (curInfTime != newInfTime)) {
                if (LoadInfFileData()) {
                    dprintf("INF change detected, Reload...");
                    curInfTime = newInfTime;
                }
            }
        }
        else if (newHnd == NULL) {
            dprintf("Wait for target process...");
        }
        else {
            dprintf("Target file not found...");
        }
    }
}

// PAK 파일 로드
bool DataManager::LoadPakFileData() {
    m_status = DM_STATUS::BUSY;
    if (!m_pakParser.Parse(m_pakFilePath, m_pakFiles)) {
        m_status = DM_STATUS::FAIL;
        return false;
    }
    // PAK 파일 로드 후 검색 테이블 구축
    if (!m_infParser.Parse(m_infFilePath)) {
        m_infParser.LoadDefault();
    }
    BuildAudioSearchTable();
    m_status = DM_STATUS::LOAD;
    return true;
}

// INF 파일 로드
bool DataManager::LoadInfFileData() {
    if ((m_status != DM_STATUS::LOAD) || !(m_infParser.Parse(m_infFilePath) || m_infParser.LoadDefault())) {
        return false;
    }
    ScanProcess::GetInstance().ControlPlayOffset(RESET_OFFSET);
    int idx = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
    int newidx = CalculateAdjustedSubIndex(idx);
    if (newidx <= 0) {
        for (idx; 0 <= idx; --idx) {
            newidx = CalculateAdjustedSubIndex(idx);
            if (1 <= newidx) {
                break;
            }
        }
        if (idx <= 0) {
            newidx = -1;
        }
    }
    SetCurrentSubIndex(newidx);
    return true;
}

// INF 파일 저장
bool DataManager::SaveDefaultInf() {
    std::wstring basePath = m_executablePath + L"voice.inf";
    if (!m_infParser.SaveDefault(basePath)) {
        return false;
    }
    if ((m_infFilePath.size() != 0) && (m_status != DM_STATUS::BUSY)) {
        auto oldStatus = m_status;
        m_status = DM_STATUS::BUSY;
        m_infFilePath = basePath;
        LoadInfFileData();
        m_status = oldStatus;
    }
    dprintf("Successfully saved the INF file.");
    return true;
}

std::vector<uint8_t> DataManager::GetPakData(const PakFileInfo& fileInfo) const {
    while (m_status != DM_STATUS::LOAD) {
        dprintf("Wait for Load data...");
        Sleep(10);
    }
    std::wstring pakPath = (fileInfo.isFirst ? m_pakFilePath : m_pakFilePath1);
    std::ifstream file(pakPath, std::ios::binary | std::ios::in);
    std::vector<uint8_t> outData;
    outData.resize(fileInfo.dataLength);

    if (!file.is_open()) {
        dprintf("[Error] Could not open pak file: %s", StringUtil::GetInstance().WstringToString(pakPath).c_str());
        return outData;
    }

    // 파일 포인터를 지정된 오프셋으로 이동
    file.seekg(fileInfo.dataOffset, std::ios::beg);

    // seekg가 실패했는지 확인 (예: 유효하지 않은 오프셋)
    if (file.fail()) {
        dprintf("[Error] Could not not seek to offset 0x%X in file: %s", fileInfo.dataOffset, StringUtil::GetInstance().WstringToString(pakPath).c_str());
        file.close();
        return outData;
    }

    // 읽기 작업 중 오류가 발생했는지 확인 (예: 파일 끝에 도달)
    file.read(reinterpret_cast<char*>(outData.data()), fileInfo.dataLength);
    if (!file && file.gcount() != fileInfo.dataLength) {
        dprintf("[Warning] Could not read %dbytes from file: %s", fileInfo.dataLength, StringUtil::GetInstance().WstringToString(pakPath).c_str());
        memset((outData.data() + file.gcount()), 0, (fileInfo.dataLength - file.gcount()));
    }

    file.close();
    return outData;
}

const PakFileInfo* DataManager::GetPakFileInfo(uint32_t uniqueId) const {
    return &m_pakFiles[uniqueId];
    return nullptr; // 찾지 못함
}

// 새로 추가된 파일 검색 함수 (std::filesystem 대신 Windows API 사용)
bool DataManager::FindTargetFile(bool getInfFile, const std::wstring& inputPath, std::wstring& outputPath) {
    wchar_t fullPath[MAX_PATH];

    // inputPath의 마지막 문자가 슬래시인지 확인하고 아니면 추가 (PathAppend가 처리하지만 명시적으로)
    std::wstring tempPath = inputPath;
    if (!tempPath.empty() && tempPath.back() != L'\\' && tempPath.back() != L'/') {
        tempPath += L'\\'; // Windows 경로는 주로 역슬래시 사용
    }

    // 파일 존재 여부를 확인하고 대소문자 구분 없이 파일명을 비교하는 헬퍼 함수
    auto fileExistsCaseInsensitiveWinAPI = [&](const std::wstring& currentFilePath, const std::wstring& targetFileNameLower) {
        // 경로가 너무 길면 실패할 수 있으므로, MAX_PATH를 넘지 않도록 주의
        if (currentFilePath.length() >= MAX_PATH) {
            return false; // 경로가 너무 깁니다.
        }

        DWORD attributes = GetFileAttributesW(currentFilePath.c_str()); // A 버전 사용 (char*)
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return false; // 파일이 존재하지 않거나 디렉토리임
        }

        return true;
    };

    if (!getInfFile) {
        // getInfFile이 false일 때 검색할 파일
        // 1) voice.pak
        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, L"voice.pak");
        if (fileExistsCaseInsensitiveWinAPI(fullPath, L"voice.pak")) {
            outputPath = fullPath;
            return true;
        }

        // 2) RTSOUND.SFS
        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, L"RTSOUND.SFS");
        if (fileExistsCaseInsensitiveWinAPI(fullPath, L"rtsound.sfs")) {
            outputPath = fullPath;
            return true;
        }

        // 3) V0_FILE.BIN (단, 폴더 내에 V1_FILE.BIN과 함께 있을 때만)
        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, L"V0_FILE.BIN");
        bool v0Exists = fileExistsCaseInsensitiveWinAPI(fullPath, L"v0_file.bin");

        wchar_t v1FullPath[MAX_PATH];
        wcscpy_s(v1FullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(v1FullPath, L"V1_FILE.BIN");
        bool v1Exists = fileExistsCaseInsensitiveWinAPI(v1FullPath, L"v1_file.bin");

        if (v0Exists && v1Exists) {
            outputPath = fullPath;
            m_pakFilePath1 = v1FullPath;
            return true;
        }
    }
    else {
        // getInfFile이 true일 때 검색할 파일
        // 1) voice.inf
        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, L"voice.inf");
        if (fileExistsCaseInsensitiveWinAPI(fullPath, L"voice.inf")) {
            outputPath = fullPath;
            return true;
        }

        // 2) RTSOUND.inf (단, 폴더 내에 RTSOUND.SFS과 함께 있을 때만)
        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, L"RTSOUND.inf");
        bool rtSoundInfExists = fileExistsCaseInsensitiveWinAPI(fullPath, L"rtsound.inf");

        wchar_t rtSoundSFSFullPath[MAX_PATH];
        wcscpy_s(rtSoundSFSFullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(rtSoundSFSFullPath, L"RTSOUND.SFS");
        bool rtSoundSFSExists = fileExistsCaseInsensitiveWinAPI(rtSoundSFSFullPath, L"rtsound.sfs");

        if (rtSoundInfExists && rtSoundSFSExists) {
            outputPath = fullPath;
            return true;
        }

        // 3) (m_executableName + ".inf")
        std::wstring execInfFileName = m_executableName + L".inf";

        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, execInfFileName.c_str());

        std::wstring execInfFileNameLower = execInfFileName;
        std::transform(execInfFileNameLower.begin(), execInfFileNameLower.end(), execInfFileNameLower.begin(), ::tolower);

        if (fileExistsCaseInsensitiveWinAPI(fullPath, execInfFileNameLower)) {
            outputPath = fullPath;
            return true;
        }
    }

    outputPath = L""; // 파일을 찾지 못한 경우
    return false;
}

bool DataManager::GetFileWriteTime(const std::wstring& filePath, uint32_t& fileTime) {
    HANDLE hFile = CreateFileW( // CreateFileA는 char* 경로를 받음
        filePath.c_str(),      // 파일 경로
        GENERIC_READ,          // 파일 읽기 권한
        FILE_SHARE_READ,       // 파일 공유 모드 (다른 프로세스가 읽을 수 있도록)
        NULL,                  // 보안 속성 (기본값)
        OPEN_EXISTING,         // 파일이 이미 존재해야 함
        FILE_ATTRIBUTE_NORMAL, // 일반 파일 속성
        NULL                   // 템플릿 파일 핸들 (사용 안 함)
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        // 파일 열기 실패 (파일이 없거나 권한 문제 등)
        //dprintf("[Error] Failed to open file %s. Error code: %d", filePath, GetLastError());
        return false;
    }

    FILETIME creationTime, lastAccessTime, lastWriteTime;
    if (!GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) {
        // 파일 크기 가져오기 실패
        dprintf("[Error] Failed to get file size for %s. Error code: %d", StringUtil::GetInstance().WstringToString(filePath).c_str(), GetLastError());
        CloseHandle(hFile); // 핸들 닫기
        return false;
    }

    fileTime = lastWriteTime.dwLowDateTime;
    CloseHandle(hFile); // 파일 핸들 닫기
    return true;
}

// 파일명에서 숫자 부분들을 파싱하는 헬퍼 함수
// `%05d_%03d_%03d.ogg` 또는 `%05d_%03d_%03d_%02d.ogg`
bool DataManager::ParseAudioFileName(const std::string& fileName, int& main_idx, int& sub_idx, int& last_idx1, int& last_idx2) const {
    main_idx = -1;
    sub_idx = -1;
    last_idx1 = -1; // 세 번째 %03d
    last_idx2 = -1; // 네 번째 %02d (선택 사항)

    std::string baseName = fileName.substr(0, fileName.find_last_of('.')); // .ogg 확장자 제거

    std::replace(baseName.begin(), baseName.end(), '_', ' '); // _를 공백으로 바꿔서 stringstream으로 파싱 용이하게

    std::stringstream ss(baseName);
    std::string s1, s2, s3, s4;

    // 00010 001 001 03
    if (ss >> s1 >> s2 >> s3) {
        try {
            main_idx = std::stoi(s1);
            sub_idx = std::stoi(s2);
            last_idx1 = std::stoi(s3);

            if (ss >> s4) { // 네 번째 숫자도 있는 경우 (ex: _02), 숫자가 아닐 경우에는 0 반환
                if ((('0' <= s4.c_str()[0]) && (s4.c_str()[0] <= '9')) && (('0' <= s4.c_str()[1]) && (s4.c_str()[1] <= '9'))) {
                    last_idx2 = std::stoi(s4);
                }
                else {
                    last_idx2 = 0;
                }
            }
            return true;
        }
        catch (const std::exception& e) {
            dprintf("[Error] Number conversion failed: %s, %s (ParseAudioFileName)", fileName.c_str(), e.what());
            return false;
        }
    }
    dprintf("[Error] File name format mismatch: %s (ParseAudioFileName)", fileName.c_str());
    return false;
}

void DataManager::BuildAudioSearchTable() {
    m_audioSearchTable.clear(); // 기존 테이블 초기화

    // 하모니 테이블 생성
    std::map<int, std::map<int, int>> m_audioHarmonySearchTable;

    dprintf("Start building audio search table...");
    for (size_t i = 0; i < m_pakFiles.size(); ++i) {
        const PakFileInfo& fileInfo = m_pakFiles[i];

        int main_idx, sub_idx, last_idx1, last_idx2;
        if (ParseAudioFileName(fileInfo.fileName, main_idx, sub_idx, last_idx1, last_idx2)) {
            // 해당 main_idx와 sub_idx 조합이 이미 테이블에 있는지 확인
            auto& subMap = m_audioSearchTable[main_idx]; // main_idx 없으면 자동 생성

            // sub_idx에 대한 기존 엔트리가 있는지 확인
            auto it = subMap.find(sub_idx);
            if (it == subMap.end()) {
                // 해당 sub_idx 조합이 없으면, 현재 파일을 추가
                subMap[sub_idx] = static_cast<int>(i);
                // 추가적으로 어떤 last_idx 조합이 선택되었는지 디버깅을 위해 저장할 수 있음.
                // 예: subMap_combined_last_idx[sub_idx] = combined_last_idx;
            }
            else {
                // 이미 해당 sub_idx 조합이 있다면, 기존 엔트리의 파일명 파싱하여 비교
                int existing_pak_index = it->second;
                const PakFileInfo& existingFileInfo = m_pakFiles[existing_pak_index];

                // `%05d_%03d_%03d_%02d.ogg` 형태일 경우 마지막 %02d가 가장 큰 값만 목록에 포함
                // -> `last_idx1` (세 번째 %03d)과 `last_idx2` (네 번째 %02d)를 조합하여 유일한 값을 만듭니다.
                //    만약 _02d가 없으면 last_idx2는 -1 이므로, 0으로 간주하거나 큰 값으로 간주해야 합니다.
                //    여기서는 (last_idx1 * 100) + (last_idx2 >= 0 ? last_idx2 : 0) 방식으로 가중치를 줍니다.
                //    이렇게 하면 (1,1,1,0) < (1,1,1,1) < (1,1,2) 가 됩니다.
                int combined_last_idx = last_idx1 + ((last_idx2 >= 0 ? last_idx2 : 0) * 1000);

                int existing_main_idx, existing_sub_idx, existing_last_idx1, existing_last_idx2;
                if (ParseAudioFileName(existingFileInfo.fileName, existing_main_idx, existing_sub_idx, existing_last_idx1, existing_last_idx2)) {
                    int existing_combined_last_idx = existing_last_idx1 + ((existing_last_idx2 >= 0 ? existing_last_idx2 : 0) * 1000);

                    if (combined_last_idx > existing_combined_last_idx) {
                        // 현재 파일의 combined_last_idx가 더 크다면 업데이트
                        subMap[sub_idx] = static_cast<int>(i);
                    }
                }
            }

            // 마지막 %02d가 있을 경우 하모니 인덱스로 추가
            if (last_idx2 != -1) {
                // 해당 main_idx와 sub_idx 조합이 이미 테이블에 있는지 확인
                auto& subMap = m_audioHarmonySearchTable[main_idx]; // main_idx 없으면 자동 생성
                int harmony_idx = (sub_idx * 1000) + last_idx2;

                // 해당 harmony_idx를 검색하지 않고 현재 파일을 추가 또는 업데이트
                subMap[harmony_idx] = static_cast<int>(i);
            }
        }
    }

    // 하모니 인덱스를 메인 인덱스로 업데이트
    for (auto it_main = m_audioHarmonySearchTable.begin(); it_main != m_audioHarmonySearchTable.end(); it_main++) {
        int main_idx = it_main->first;
        auto& harmonyMap = it_main->second;

        // 해당 main_idx와 sub_idx 조합이 이미 테이블에 있는지 확인
        auto& subMap = m_audioSearchTable[main_idx]; // main_idx 없으면 자동 생성

        // 해당 harmony_idx를 검색하지 않고 현재 파일을 추가 또는 업데이트
        for (auto it_sub = harmonyMap.begin(); it_sub != harmonyMap.end(); it_sub++) {
            subMap[(it_sub->first)] = it_sub->second;
        }
    }

    // 00000 오디오가 있을 경우 90900 정보 추가
    auto it_zero = m_audioSearchTable.find(0);
    if (it_zero != m_audioSearchTable.end()) {
        auto& it_map = it_zero->second;
        auto& subMap = m_audioSearchTable[90900]; // main_idx 없으면 자동 생성
        subMap = it_map;
    }

    dprintf("Audio search table has been built. (Main_index count: %d)", m_audioSearchTable.size());
}


int DataManager::SearchIndex(int main_index, int sub_index) const {
    auto it_main = m_audioSearchTable.find(main_index);
    if (it_main != m_audioSearchTable.end()) {
        const auto& subMap = it_main->second;
        int i = sub_index;
        for (auto it_sub = subMap.begin(); it_sub != subMap.end(); ++it_sub) {
            if (--i == 0) {
                return it_sub->second;
            }
        }
        auto it_sub = subMap.find(sub_index);
        if (it_sub != subMap.end()) {
            return it_sub->second; // 실제 pakFiles의 인덱스 번호 반환
        }
    }
    return -1; // 찾지 못함
}

std::map<int, int> DataManager::GetCurrentAudioIndex() const {  
    auto it_main = m_audioSearchTable.find(m_currentMainIndex.load());  
    if (it_main != m_audioSearchTable.end()) {  
        return it_main->second;  
    }  
    return std::map<int, int>(); // 빈 맵 반환  
}

// 새롭게 추가되는 함수들
void DataManager::SetCurrentMainIndex(int index) {
    m_currentMainIndex.store(index); // atomic 변수 업데이트
}

int DataManager::GetCurrentMainIndex() const {
    return m_currentMainIndex.load(); // atomic 변수 읽기
}

void DataManager::SetCurrentSubIndex(int index) {
    m_currentSubIndex.store(index); // atomic 변수 업데이트
}

int DataManager::GetCurrentSubIndex() const {
    return m_currentSubIndex.load(); // atomic 변수 읽기
}

// INF 매핑을 기반으로 조정된 SubIndex를 계산하여 반환하는 함수
int DataManager::CalculateAdjustedSubIndex(int subIndex, bool isForced) const {
    int mainIndex = m_currentMainIndex.load();
    const auto& infMappings = m_infParser.GetInfMappings(); // InfParser에서 맵을 가져옴

    auto it_main = infMappings.find(mainIndex);
    if (it_main == infMappings.end()) {
        // 해당 mainIndex가 INF 맵에 없는 경우, 조정 없이 subIndex 그대로 반환
        // 또는 오류를 나타내는 다른 값을 반환할 수 있음
        // 예시 시나리오에 따라 여기서는 subIndex 그대로 반환
        //dprintf("[Warning] MainIndex %d not found in INF mappings. Returning original subIndex %d.", mainIndex, subIndex);
        return subIndex;
    }

    const auto& subMap = it_main->second;

    auto it_found = subMap.upper_bound(subIndex); // subIndex보다 큰 첫 번째 요소를 찾음

    if (it_found == subMap.begin()) {
        // subIndex보다 작거나 같은 요소가 없음
        // dprintf("[Warning] No subIndex <= %d found for MainIndex %d in INF mappings. Returning original subIndex %d.", subIndex, mainIndex, subIndex);
        return subIndex; // 이 경우 원본 subIndex를 반환
    }

    --it_found; // subIndex보다 작거나 같은 가장 큰 요소를 찾음

    int baseSubIndex = it_found->first;
    int baseValue = it_found->second;
    int adjustedValue = 0;

    if (baseValue == -1) {
        if (!isForced && (baseSubIndex == subIndex)) {
            // 찾은 baseValue가 -1인 경우, 해당 구간은 -1로 반환
            // dprintf("[Info] Calculated adjusted SubIndex for MainIndex %d, SubIndex %d: -1 (due to base value -1 at %d)", mainIndex, subIndex, baseSubIndex);
            return -1;
        }
        else {
            adjustedValue--;
            while (it_found != subMap.begin()) {
                --it_found; // subIndex보다 작거나 같은 가장 큰 요소를 찾음

                baseSubIndex = it_found->first;
                baseValue = it_found->second;

                if (baseValue == -1) {
                    adjustedValue--;
                }
                else {
                    adjustedValue += baseValue + (subIndex - baseSubIndex);
                    break;
                }
            }
            if (adjustedValue < 0) {
                adjustedValue += subIndex;
            }
        }
    }
    else {
        adjustedValue = baseValue + (subIndex - baseSubIndex);
    }
#ifdef _DEBUG
    dprintf("[Info] Calculated adjusted SubIndex for MainIndex %d, SubIndex %d: %d (base %d from %d, diff %d)", mainIndex, subIndex, adjustedValue, baseValue, baseSubIndex, (subIndex - baseSubIndex));
#endif
    return adjustedValue;
}