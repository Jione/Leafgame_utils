#pragma once

#include "GlobalDefinitions.h"
#include "PakParser.h"
#include "InfParser.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <thread>   // std::thread
#include <atomic>   // std::atomic
#include <iostream> // cout

class DataManager {
public:
    static DataManager& GetInstance(); // �̱���

    // ������ �Ŵ��� ������ ����
    void StartDataManagerThread();
    // ������ �ε� ������ ����
    void StopDataManagerThread();
    // ������ �ε� ������ ���� ���
    void WaitStopDataManagerThread();
    // ���� ������ �ε� ���� ��ȯ
    DM_STATUS GetStatus() const { return m_status; }
    // ���� ���� ���� ���� ��ȯ
    std::wstring& GetExecutablePath() { return m_executablePath; }
    // ���� �Ľ� ���� Pak ���� ��� ��ȯ
    std::wstring& GetPackPath() { return m_pakFilePath; }
    // ���� �Ľ� ���� inf ���� ��� ��ȯ
    std::wstring& GetInfPath() { return m_infFilePath; }
    // Pak ���� ��θ� ���Ƿ� ����
    void SetPackPath(std::wstring path) { m_pakFilePath = path; LoadPakFileData();}
    // inf ���� ��θ� ���Ƿ� ����
    void SetInfPath(std::wstring path) { m_infFilePath = path; LoadInfFileData(); }
    // �⺻ inf ������ ����
    bool SaveDefaultInf();
    // INF ���� �ٽ� �ε�
    bool ReloadInf() { return LoadInfFileData(); }

    // �ε�� PAK ���� ���� ��ȯ
    const std::vector<PakFileInfo>& GetPakFiles() const { return m_pakFiles; }

    // PAK ���� ������ ������� ������ ��ȯ
    std::vector<uint8_t> GetPakData(const PakFileInfo& fileInfo) const;

    // Ư�� uniqueId�� �ش��ϴ� PakFileInfo�� ã���ϴ�.
    const PakFileInfo* GetPakFileInfo(uint32_t uniqueId) const;

    // ���ο� �˻� �Լ�: main_index�� sub_index�� ������� ���� pakFiles�� �ε��� ��ȣ ��ȯ
    // ã�� ���� ��� -1 ��ȯ
    int SearchIndex(int main_index, int sub_index) const;

    // �ε�� PAK ��ü ������ ��ȯ
    std::map<int, int> GetCurrentAudioIndex() const;

    // INF ������ ������� ������ SubIndex�� ����Ͽ� ��ȯ
    int CalculateAdjustedSubIndex(int subIndex, bool isForced = false) const;

    // ���� �о�� main_index ���� ����
    void SetCurrentMainIndex(int index);
    // main_index ���� ������
    int GetCurrentMainIndex() const;
    // ���� �о�� sub_index ���� ����
    void SetCurrentSubIndex(int index);
    // sub_index ���� ������
    int GetCurrentSubIndex() const;

private:
    DataManager(); // �̱���
    ~DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    void ManagementDataLoop();  // ���� �۵� ����
    bool LoadPakFileData();     // PAK ���� �ε�
    bool LoadInfFileData();     // INF ���� �ε�

    bool FindTargetFile(bool getInfFile, const std::wstring& inputPath, std::wstring& outputPath);
    bool GetFileWriteTime(const std::wstring& filePath, uint32_t& fileSize);

    std::vector<PakFileInfo> m_pakFiles;      // �Ľ̵� PAK ���� ���
    std::map<uint32_t, uint32_t> m_infMappings; // �Ľ̵� INF ����

    // main_index (ù 5�ڸ�) -> sub_index (�� ��° 3�ڸ�) -> pakFiles ���� ���� �ε���
    std::map<int, std::map<int, int>> m_audioSearchTable;

    // ����� �˻� ���̺��� �����մϴ�.
    void BuildAudioSearchTable();
    // ���ϸ��� ���� �κе��� �Ľ��մϴ�.
    bool ParseAudioFileName(const std::string& fileName, int& main_idx, int& sub_idx, int& last_idx1, int& last_idx2) const;

    std::atomic<bool> m_stopDataManagerThread; // ������ ���� �÷���
    std::thread m_dataManagerThread;           // ������ �Ŵ��� ������ ��ü

    std::atomic<int> m_currentMainIndex; // �о�� main_idx ���� ���� (������ ����)
    std::atomic<int> m_currentSubIndex; // �о�� sub_idx ���� ����

    std::wstring m_executablePath;
    std::wstring m_executableName;
    std::wstring m_infFilePath;
    std::wstring m_pakFilePath;
    std::wstring m_pakFilePath1;
    DM_STATUS m_status;
    PakParser m_pakParser;
    InfParser m_infParser;
};