#pragma once
#include <string>
#include <atomic>   // std::atomic
#include <thread>   // std::thread
#include <windows.h> // For DWORD, LPVOID, HWND (though HWND not directly used in this module now)
#include <vector>    // for std::vector

// ��� ���μ��� ������ �����ϴ� ����ü
struct TargetProcessInfo {
    std::string processName;
    std::string windowCaption; // â ĸ�� (�ʿ�� �߰�)
};

// ���μ��� �޸𸮿��� �о���� ���� ���¸� �����ϴ� ����ü
struct ProcessMemoryStatus {
    // ���� �޸𸮿��� �о�� ���� (��Ȯ�� ���������� ����)
    char sdtBase[4];                        // "kos" �Ǵ� "sdt" ���ڿ�
    DWORD textBufferCurrentOffset;          // textBuffer ������ ���� ������
    DWORD textBufferBaseAddress;            // ���ۿ� �ؽ�Ʈ�� �ε�� ��ġ (LPVOID ĳ���� �ʿ�)
    char textOutputBusyFlag;                // ���� ���α׷��� �ؽ�Ʈ ��� ���� (0=�ε� ����, 1=���� ��, 2=�۾� ��)
    char currentScriptNumber[6];            // ���� �ε� ���� ��ũ��Ʈ�� (%05d.sdt), %05d�� ���
    DWORD printTextBaseAddress;             // ���� �����쿡 ��� ���� ������ ������ (LPVOID ĳ���� �ʿ�)
    WORD printTextBaseCounts;               // ���� ���� ������ ��� ��� ���� (0xFFFF�� ��� ������ �б�)

    // ���� ��� ���α׷� ���� ����
    char textBuffer[0x400];                 // ȭ�鿡 ��µǴ� �ؽ�Ʈ�� ����
    int scriptIndex;                        // ��ũ��Ʈ�� ��� �ε��� �� (��ó�� �� sub_idx�� ����)
    int voiceFlag;                          // ���̽� ��� �÷���
    int voiceIndexBuffer;                   // ���� ���̽� ��� ����
    bool passVoicePlay;                     // ���̽� ��� �Ͻ����� �÷���
    int playOffset;                         // ��� ������ ����

    // �����ڿ��� �⺻�� �ʱ�ȭ
    ProcessMemoryStatus() :
        textBufferCurrentOffset(0),
        textBufferBaseAddress(0),
        textOutputBusyFlag(0),
        printTextBaseAddress(0),
        printTextBaseCounts(0xFFFE), // �⺻���� 0xFFFE�� ����
        scriptIndex(0),
        voiceFlag(0),
        voiceIndexBuffer(0),
        passVoicePlay(false),
        playOffset(0)
    {
        memset(sdtBase, 0, sizeof(sdtBase));
        memset(currentScriptNumber, 0, sizeof(currentScriptNumber));
        memset(textBuffer, 0, sizeof(textBuffer));
    }
};

enum SCAN_STATUS {
    ADD_OFFSET = 0, // ������ 1 ����
    DEC_OFFSET,     // ������ 1 ����
    RESET_OFFSET,   // ������ �ʱ�ȭ
    GET_OFFSET,     // ���� ������ ���
    SET_OFFSET,     // ������ �� ����
    GET_BLOCK,      // ���� ��� ���
    SET_BLOCK,      // ��� �� ����
    GET_SDT,        // ���� SDT �� ����
};


// ScanProcess ����� ĸ��ȭ�ϴ� Ŭ����
class ScanProcess {
public:
    static ScanProcess& GetInstance(); // �̱���

    // ��ĵ ������ ���� (���� ����)
    void StartScanThread();
    // ��ĵ ������ ����
    void StopScanThread();
    // ��ĵ ������ ���� ���
    void WaitStopScanThread();
    // ���̽� ��� ��� ������ ����
    int ControlPlayOffset(SCAN_STATUS mode, int value = 0);
    // ��� ���μ��� �ڵ� ��ȯ
    HANDLE& GetTargetProcessHandle() { return m_hProcess; }
    // ��� ���μ��� ��� ��ȯ
    std::wstring& GetTargetProcessPath() { return m_targetProcessPath; }

private:
    ScanProcess(); // �̱���
    ~ScanProcess();
    ScanProcess(const ScanProcess&) = delete;
    ScanProcess& operator=(const ScanProcess&) = delete;

    // FindProcess �Լ� (TargetProcessInfo ���)
    HANDLE FindProcess(const TargetProcessInfo& targetInfo);
    // ��ȿ�� ���μ��� �ڵ��� �˻��ϰ� �����ϴ� �Լ�
    bool FindValidProcessHandle(HANDLE& outProcessId, HANDLE& outHProcess);

    void ProcessMemoryScanLoop(); // ���� ��ĵ ����

    void EvaluateProcessStatus(); // m_processStatus ������ ������� voice ��� ���¸� Ȯ��/���
    bool CalculateCurrentBlockIndex(); // m_processStatus ������ ������� scriptIndex, voiceFlag ���� ���
    int BlockCountFromBuffer();        // m_processStatus.textBuffer ���ڿ��� ��ü ���ڿ� ��� ���� ���

    // textBuff���� textLen��ŭ �˻��Ͽ� voiceFlag, blockCount�� ����
    void BlockCountFromPointer(const char* textBuff, int textLen, int& voiceFlag, int& blockCount);

    // idx�� printCount�� ������� ����ũ ��� ���࿩�� Ȯ��
    bool UniqueChecker();

    // �ε� �� SCN ������ ù ��° �������� Ȯ��
    bool CheckFirstLoad(const char* sdtBuffer, SIZE_T readSize) const;

    std::atomic<bool> m_stopScanThread; // ������ ���� �÷���
    std::thread m_scanThread;           // ��ĵ ������ ��ü

    std::vector<TargetProcessInfo> m_targetProcesses;   // ��� ���μ��� ����
    std::wstring m_targetProcessPath;                    // Ÿ�� ���μ����� ���� ���
    ProcessMemoryStatus m_processStatus;                // ���μ��� �޸� ���¸� �����ϴ� ����ü
    HANDLE m_hProcess;                                  // Ÿ�� ���μ��� �ڵ�
    char m_sdtBuffer[0x10000];                          // sdt �ؽ�Ʈ ����
    int m_blockCount;                                   // ������ block ī����
    int m_bbCount;                                      // ��� ������ block ī����
    bool m_cinema;                                      // ���� ���� Ȯ��
    bool m_isLoad;                                        // �ε� ���� Ȯ��
};