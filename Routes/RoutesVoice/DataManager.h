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
    static DataManager& GetInstance(); // 싱글톤

    // 데이터 매니저 스레드 시작
    void StartDataManagerThread();
    // 데이터 로드 스레드 종료
    void StopDataManagerThread();
    // 데이터 로드 스레드 종료 대기
    void WaitStopDataManagerThread();
    // 현재 데이터 로드 상태 반환
    DM_STATUS GetStatus() const { return m_status; }
    // 현재 실행 중인 폴더 반환
    std::wstring& GetExecutablePath() { return m_executablePath; }
    // 현재 파싱 중인 Pak 파일 경로 반환
    std::wstring& GetPackPath() { return m_pakFilePath; }
    // 현재 파싱 중인 inf 파일 경로 반환
    std::wstring& GetInfPath() { return m_infFilePath; }
    // Pak 파일 경로를 임의로 설정
    void SetPackPath(std::wstring path) { m_pakFilePath = path; LoadPakFileData();}
    // inf 파일 경로를 임의로 설정
    void SetInfPath(std::wstring path) { m_infFilePath = path; LoadInfFileData(); }
    // 기본 inf 파일을 저장
    bool SaveDefaultInf();
    // INF 파일 다시 로드
    bool ReloadInf() { return LoadInfFileData(); }

    // 로드된 PAK 파일 정보 반환
    const std::vector<PakFileInfo>& GetPakFiles() const { return m_pakFiles; }

    // PAK 파일 정보를 기반으로 데이터 반환
    std::vector<uint8_t> GetPakData(const PakFileInfo& fileInfo) const;

    // 특정 uniqueId에 해당하는 PakFileInfo를 찾습니다.
    const PakFileInfo* GetPakFileInfo(uint32_t uniqueId) const;

    // 새로운 검색 함수: main_index와 sub_index를 기반으로 실제 pakFiles의 인덱스 번호 반환
    // 찾지 못할 경우 -1 반환
    int SearchIndex(int main_index, int sub_index) const;

    // 로드된 PAK 전체 데이터 반환
    std::map<int, int> GetCurrentAudioIndex() const;

    // INF 매핑을 기반으로 조정된 SubIndex를 계산하여 반환
    int CalculateAdjustedSubIndex(int subIndex, bool isForced = false) const;

    // 현재 읽어온 main_index 값을 설정
    void SetCurrentMainIndex(int index);
    // main_index 값을 가져옴
    int GetCurrentMainIndex() const;
    // 현재 읽어온 sub_index 값을 설정
    void SetCurrentSubIndex(int index);
    // sub_index 값을 가져옴
    int GetCurrentSubIndex() const;

private:
    DataManager(); // 싱글톤
    ~DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    void ManagementDataLoop();  // 실제 작동 루프
    bool LoadPakFileData();     // PAK 파일 로드
    bool LoadInfFileData();     // INF 파일 로드

    bool FindTargetFile(bool getInfFile, const std::wstring& inputPath, std::wstring& outputPath);
    bool GetFileWriteTime(const std::wstring& filePath, uint32_t& fileSize);

    std::vector<PakFileInfo> m_pakFiles;      // 파싱된 PAK 파일 목록
    std::map<uint32_t, uint32_t> m_infMappings; // 파싱된 INF 매핑

    // main_index (첫 5자리) -> sub_index (두 번째 3자리) -> pakFiles 내의 실제 인덱스
    std::map<int, std::map<int, int>> m_audioSearchTable;

    // 오디오 검색 테이블을 구축합니다.
    void BuildAudioSearchTable();
    // 파일명에서 숫자 부분들을 파싱합니다.
    bool ParseAudioFileName(const std::string& fileName, int& main_idx, int& sub_idx, int& last_idx1, int& last_idx2) const;

    std::atomic<bool> m_stopDataManagerThread; // 스레드 종료 플래그
    std::thread m_dataManagerThread;           // 데이터 매니저 스레드 객체

    std::atomic<int> m_currentMainIndex; // 읽어온 main_idx 값을 저장 (스레드 안전)
    std::atomic<int> m_currentSubIndex; // 읽어온 sub_idx 값을 저장

    std::wstring m_executablePath;
    std::wstring m_executableName;
    std::wstring m_infFilePath;
    std::wstring m_pakFilePath;
    std::wstring m_pakFilePath1;
    DM_STATUS m_status;
    PakParser m_pakParser;
    InfParser m_infParser;
};