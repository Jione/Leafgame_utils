#include "DataManager.h"
#include "ScanProcess.h"
#include "StringUtil.h"
#include <sstream> // std::stringstream
#include <algorithm> // std::max, std::transform
#include <iomanip> // std::setw, std::setfill for debugging output
#include <shlwapi.h> // PathCombine, PathAppend ���� ���� ��� (PathCchCombine�� �� �ֽ������� ���⼱ PathAppend ���)
#pragma comment(lib, "Shlwapi.lib") // PathAppend �Լ��� ���� Shlwapi.lib ��ũ

DataManager::DataManager()
    : m_currentMainIndex(-1),
    m_currentSubIndex(0),
    m_status(DM_STATUS::WAIT),
    m_infFilePath(L""),
    m_pakFilePath(L"")
{   // ������
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
    // �Ҹ���
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

    // ���� �÷��� �ʱ�ȭ
    m_stopDataManagerThread.store(false);

    // std::thread�� ����Ͽ� ��� �Լ��� ������� ����
    // this �����͸� ���� ĸó ������� �����Ͽ� ��� �Լ� ȣ��
    m_dataManagerThread = std::thread([this]() {
        this->ManagementDataLoop();
        });
    dprintf("Start the Data Manager thread. (DataManager)");
}

void DataManager::StopDataManagerThread() {
    if (m_dataManagerThread.joinable()) {
        m_stopDataManagerThread.store(true); // �����忡 ���� ��ȣ
    }
}

void DataManager::WaitStopDataManagerThread() {
    if (m_dataManagerThread.joinable()) {
        m_dataManagerThread.join();         // ������ ���� ���
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
        Sleep(1000); // 1�ʿ� �� ���� �˻�

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

// PAK ���� �ε�
bool DataManager::LoadPakFileData() {
    m_status = DM_STATUS::BUSY;
    if (!m_pakParser.Parse(m_pakFilePath, m_pakFiles)) {
        m_status = DM_STATUS::FAIL;
        return false;
    }
    // PAK ���� �ε� �� �˻� ���̺� ����
    if (!m_infParser.Parse(m_infFilePath)) {
        m_infParser.LoadDefault();
    }
    BuildAudioSearchTable();
    m_status = DM_STATUS::LOAD;
    return true;
}

// INF ���� �ε�
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

// INF ���� ����
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

    // ���� �����͸� ������ ���������� �̵�
    file.seekg(fileInfo.dataOffset, std::ios::beg);

    // seekg�� �����ߴ��� Ȯ�� (��: ��ȿ���� ���� ������)
    if (file.fail()) {
        dprintf("[Error] Could not not seek to offset 0x%X in file: %s", fileInfo.dataOffset, StringUtil::GetInstance().WstringToString(pakPath).c_str());
        file.close();
        return outData;
    }

    // �б� �۾� �� ������ �߻��ߴ��� Ȯ�� (��: ���� ���� ����)
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
    return nullptr; // ã�� ����
}

// ���� �߰��� ���� �˻� �Լ� (std::filesystem ��� Windows API ���)
bool DataManager::FindTargetFile(bool getInfFile, const std::wstring& inputPath, std::wstring& outputPath) {
    wchar_t fullPath[MAX_PATH];

    // inputPath�� ������ ���ڰ� ���������� Ȯ���ϰ� �ƴϸ� �߰� (PathAppend�� ó�������� ���������)
    std::wstring tempPath = inputPath;
    if (!tempPath.empty() && tempPath.back() != L'\\' && tempPath.back() != L'/') {
        tempPath += L'\\'; // Windows ��δ� �ַ� �������� ���
    }

    // ���� ���� ���θ� Ȯ���ϰ� ��ҹ��� ���� ���� ���ϸ��� ���ϴ� ���� �Լ�
    auto fileExistsCaseInsensitiveWinAPI = [&](const std::wstring& currentFilePath, const std::wstring& targetFileNameLower) {
        // ��ΰ� �ʹ� ��� ������ �� �����Ƿ�, MAX_PATH�� ���� �ʵ��� ����
        if (currentFilePath.length() >= MAX_PATH) {
            return false; // ��ΰ� �ʹ� ��ϴ�.
        }

        DWORD attributes = GetFileAttributesW(currentFilePath.c_str()); // A ���� ��� (char*)
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return false; // ������ �������� �ʰų� ���丮��
        }

        return true;
    };

    if (!getInfFile) {
        // getInfFile�� false�� �� �˻��� ����
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

        // 3) V0_FILE.BIN (��, ���� ���� V1_FILE.BIN�� �Բ� ���� ����)
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
        // getInfFile�� true�� �� �˻��� ����
        // 1) voice.inf
        wcscpy_s(fullPath, MAX_PATH, tempPath.c_str());
        PathAppendW(fullPath, L"voice.inf");
        if (fileExistsCaseInsensitiveWinAPI(fullPath, L"voice.inf")) {
            outputPath = fullPath;
            return true;
        }

        // 2) RTSOUND.inf (��, ���� ���� RTSOUND.SFS�� �Բ� ���� ����)
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

    outputPath = L""; // ������ ã�� ���� ���
    return false;
}

bool DataManager::GetFileWriteTime(const std::wstring& filePath, uint32_t& fileTime) {
    HANDLE hFile = CreateFileW( // CreateFileA�� char* ��θ� ����
        filePath.c_str(),      // ���� ���
        GENERIC_READ,          // ���� �б� ����
        FILE_SHARE_READ,       // ���� ���� ��� (�ٸ� ���μ����� ���� �� �ֵ���)
        NULL,                  // ���� �Ӽ� (�⺻��)
        OPEN_EXISTING,         // ������ �̹� �����ؾ� ��
        FILE_ATTRIBUTE_NORMAL, // �Ϲ� ���� �Ӽ�
        NULL                   // ���ø� ���� �ڵ� (��� �� ��)
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        // ���� ���� ���� (������ ���ų� ���� ���� ��)
        //dprintf("[Error] Failed to open file %s. Error code: %d", filePath, GetLastError());
        return false;
    }

    FILETIME creationTime, lastAccessTime, lastWriteTime;
    if (!GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) {
        // ���� ũ�� �������� ����
        dprintf("[Error] Failed to get file size for %s. Error code: %d", StringUtil::GetInstance().WstringToString(filePath).c_str(), GetLastError());
        CloseHandle(hFile); // �ڵ� �ݱ�
        return false;
    }

    fileTime = lastWriteTime.dwLowDateTime;
    CloseHandle(hFile); // ���� �ڵ� �ݱ�
    return true;
}

// ���ϸ��� ���� �κе��� �Ľ��ϴ� ���� �Լ�
// `%05d_%03d_%03d.ogg` �Ǵ� `%05d_%03d_%03d_%02d.ogg`
bool DataManager::ParseAudioFileName(const std::string& fileName, int& main_idx, int& sub_idx, int& last_idx1, int& last_idx2) const {
    main_idx = -1;
    sub_idx = -1;
    last_idx1 = -1; // �� ��° %03d
    last_idx2 = -1; // �� ��° %02d (���� ����)

    std::string baseName = fileName.substr(0, fileName.find_last_of('.')); // .ogg Ȯ���� ����

    std::replace(baseName.begin(), baseName.end(), '_', ' '); // _�� �������� �ٲ㼭 stringstream���� �Ľ� �����ϰ�

    std::stringstream ss(baseName);
    std::string s1, s2, s3, s4;

    // 00010 001 001 03
    if (ss >> s1 >> s2 >> s3) {
        try {
            main_idx = std::stoi(s1);
            sub_idx = std::stoi(s2);
            last_idx1 = std::stoi(s3);

            if (ss >> s4) { // �� ��° ���ڵ� �ִ� ��� (ex: _02), ���ڰ� �ƴ� ��쿡�� 0 ��ȯ
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
    m_audioSearchTable.clear(); // ���� ���̺� �ʱ�ȭ

    // �ϸ�� ���̺� ����
    std::map<int, std::map<int, int>> m_audioHarmonySearchTable;

    dprintf("Start building audio search table...");
    for (size_t i = 0; i < m_pakFiles.size(); ++i) {
        const PakFileInfo& fileInfo = m_pakFiles[i];

        int main_idx, sub_idx, last_idx1, last_idx2;
        if (ParseAudioFileName(fileInfo.fileName, main_idx, sub_idx, last_idx1, last_idx2)) {
            // �ش� main_idx�� sub_idx ������ �̹� ���̺� �ִ��� Ȯ��
            auto& subMap = m_audioSearchTable[main_idx]; // main_idx ������ �ڵ� ����

            // sub_idx�� ���� ���� ��Ʈ���� �ִ��� Ȯ��
            auto it = subMap.find(sub_idx);
            if (it == subMap.end()) {
                // �ش� sub_idx ������ ������, ���� ������ �߰�
                subMap[sub_idx] = static_cast<int>(i);
                // �߰������� � last_idx ������ ���õǾ����� ������� ���� ������ �� ����.
                // ��: subMap_combined_last_idx[sub_idx] = combined_last_idx;
            }
            else {
                // �̹� �ش� sub_idx ������ �ִٸ�, ���� ��Ʈ���� ���ϸ� �Ľ��Ͽ� ��
                int existing_pak_index = it->second;
                const PakFileInfo& existingFileInfo = m_pakFiles[existing_pak_index];

                // `%05d_%03d_%03d_%02d.ogg` ������ ��� ������ %02d�� ���� ū ���� ��Ͽ� ����
                // -> `last_idx1` (�� ��° %03d)�� `last_idx2` (�� ��° %02d)�� �����Ͽ� ������ ���� ����ϴ�.
                //    ���� _02d�� ������ last_idx2�� -1 �̹Ƿ�, 0���� �����ϰų� ū ������ �����ؾ� �մϴ�.
                //    ���⼭�� (last_idx1 * 100) + (last_idx2 >= 0 ? last_idx2 : 0) ������� ����ġ�� �ݴϴ�.
                //    �̷��� �ϸ� (1,1,1,0) < (1,1,1,1) < (1,1,2) �� �˴ϴ�.
                int combined_last_idx = last_idx1 + ((last_idx2 >= 0 ? last_idx2 : 0) * 1000);

                int existing_main_idx, existing_sub_idx, existing_last_idx1, existing_last_idx2;
                if (ParseAudioFileName(existingFileInfo.fileName, existing_main_idx, existing_sub_idx, existing_last_idx1, existing_last_idx2)) {
                    int existing_combined_last_idx = existing_last_idx1 + ((existing_last_idx2 >= 0 ? existing_last_idx2 : 0) * 1000);

                    if (combined_last_idx > existing_combined_last_idx) {
                        // ���� ������ combined_last_idx�� �� ũ�ٸ� ������Ʈ
                        subMap[sub_idx] = static_cast<int>(i);
                    }
                }
            }

            // ������ %02d�� ���� ��� �ϸ�� �ε����� �߰�
            if (last_idx2 != -1) {
                // �ش� main_idx�� sub_idx ������ �̹� ���̺� �ִ��� Ȯ��
                auto& subMap = m_audioHarmonySearchTable[main_idx]; // main_idx ������ �ڵ� ����
                int harmony_idx = (sub_idx * 1000) + last_idx2;

                // �ش� harmony_idx�� �˻����� �ʰ� ���� ������ �߰� �Ǵ� ������Ʈ
                subMap[harmony_idx] = static_cast<int>(i);
            }
        }
    }

    // �ϸ�� �ε����� ���� �ε����� ������Ʈ
    for (auto it_main = m_audioHarmonySearchTable.begin(); it_main != m_audioHarmonySearchTable.end(); it_main++) {
        int main_idx = it_main->first;
        auto& harmonyMap = it_main->second;

        // �ش� main_idx�� sub_idx ������ �̹� ���̺� �ִ��� Ȯ��
        auto& subMap = m_audioSearchTable[main_idx]; // main_idx ������ �ڵ� ����

        // �ش� harmony_idx�� �˻����� �ʰ� ���� ������ �߰� �Ǵ� ������Ʈ
        for (auto it_sub = harmonyMap.begin(); it_sub != harmonyMap.end(); it_sub++) {
            subMap[(it_sub->first)] = it_sub->second;
        }
    }

    // 00000 ������� ���� ��� 90900 ���� �߰�
    auto it_zero = m_audioSearchTable.find(0);
    if (it_zero != m_audioSearchTable.end()) {
        auto& it_map = it_zero->second;
        auto& subMap = m_audioSearchTable[90900]; // main_idx ������ �ڵ� ����
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
            return it_sub->second; // ���� pakFiles�� �ε��� ��ȣ ��ȯ
        }
    }
    return -1; // ã�� ����
}

std::map<int, int> DataManager::GetCurrentAudioIndex() const {  
    auto it_main = m_audioSearchTable.find(m_currentMainIndex.load());  
    if (it_main != m_audioSearchTable.end()) {  
        return it_main->second;  
    }  
    return std::map<int, int>(); // �� �� ��ȯ  
}

// ���Ӱ� �߰��Ǵ� �Լ���
void DataManager::SetCurrentMainIndex(int index) {
    m_currentMainIndex.store(index); // atomic ���� ������Ʈ
}

int DataManager::GetCurrentMainIndex() const {
    return m_currentMainIndex.load(); // atomic ���� �б�
}

void DataManager::SetCurrentSubIndex(int index) {
    m_currentSubIndex.store(index); // atomic ���� ������Ʈ
}

int DataManager::GetCurrentSubIndex() const {
    return m_currentSubIndex.load(); // atomic ���� �б�
}

// INF ������ ������� ������ SubIndex�� ����Ͽ� ��ȯ�ϴ� �Լ�
int DataManager::CalculateAdjustedSubIndex(int subIndex, bool isForced) const {
    int mainIndex = m_currentMainIndex.load();
    const auto& infMappings = m_infParser.GetInfMappings(); // InfParser���� ���� ������

    auto it_main = infMappings.find(mainIndex);
    if (it_main == infMappings.end()) {
        // �ش� mainIndex�� INF �ʿ� ���� ���, ���� ���� subIndex �״�� ��ȯ
        // �Ǵ� ������ ��Ÿ���� �ٸ� ���� ��ȯ�� �� ����
        // ���� �ó������� ���� ���⼭�� subIndex �״�� ��ȯ
        //dprintf("[Warning] MainIndex %d not found in INF mappings. Returning original subIndex %d.", mainIndex, subIndex);
        return subIndex;
    }

    const auto& subMap = it_main->second;

    auto it_found = subMap.upper_bound(subIndex); // subIndex���� ū ù ��° ��Ҹ� ã��

    if (it_found == subMap.begin()) {
        // subIndex���� �۰ų� ���� ��Ұ� ����
        // dprintf("[Warning] No subIndex <= %d found for MainIndex %d in INF mappings. Returning original subIndex %d.", subIndex, mainIndex, subIndex);
        return subIndex; // �� ��� ���� subIndex�� ��ȯ
    }

    --it_found; // subIndex���� �۰ų� ���� ���� ū ��Ҹ� ã��

    int baseSubIndex = it_found->first;
    int baseValue = it_found->second;
    int adjustedValue = 0;

    if (baseValue == -1) {
        if (!isForced && (baseSubIndex == subIndex)) {
            // ã�� baseValue�� -1�� ���, �ش� ������ -1�� ��ȯ
            // dprintf("[Info] Calculated adjusted SubIndex for MainIndex %d, SubIndex %d: -1 (due to base value -1 at %d)", mainIndex, subIndex, baseSubIndex);
            return -1;
        }
        else {
            adjustedValue--;
            while (it_found != subMap.begin()) {
                --it_found; // subIndex���� �۰ų� ���� ���� ū ��Ҹ� ã��

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