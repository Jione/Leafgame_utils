#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <conio.h>
#include <windows.h>

#include "DataManager.h"
#include "AudioManager.h"
#include "ScanProcess.h" // ���� �߰�

#undef max

int main() {
    std::cout << "RoutesVoice CUI �׽�Ʈ ���α׷� ����." << std::endl;
    std::cout << "------------------------------------" << std::endl;

    // ���μ��� �޸� ��ĵ ������ ���� (ScanProcess Ŭ���� ���)
    DataManager::GetInstance().StartDataManagerThread();
    ScanProcess::GetInstance().StartScanThread();
    if (!AudioManager::GetInstance().Initialize()) {
        std::cerr << "����� �ý��� �ʱ�ȭ ����. ���α׷��� �����մϴ�." << std::endl;
        return 1;
    }
    int count = 0;
    std::cout << "����� �ý��� �ʱ�ȭ ��..";
    while (true) {
        if ((DataManager::GetInstance().GetStatus() == LOAD)) {
            break;
        }
        if (++count == 6) {
            count = 0;
            std::cout << "\r����� �ý��� �ʱ�ȭ ��..      " << "\r����� �ý��� �ʱ�ȭ ��..";
        }
        else {
            std::cout << ".";
        }
        Sleep(120);
    }
    std::cout << "\r����� �ý��� �ʱ�ȭ ���� (DirectSound)." << std::endl;
    // ----------------------------------------------------

    std::string input_main, input_sub;
    int curMainId = -1; // �޸𸮿��� �о�� �� ������ (����)
    int curBlockNo = -1;  // �޸𸮿��� �о�� �� ������ (���)
    int oncePlayId = -1;
    char sdtNo[6] = { 0, };
    Sleep(1000);
    DataManager::GetInstance().ReloadInf();
    while (true) {
        int newMainId = DataManager::GetInstance().GetCurrentMainIndex();
        int offset = ScanProcess::GetInstance().ControlPlayOffset(GET_OFFSET);
        int blockNo = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
        if (newMainId != -1 && newMainId != curMainId) {
            sprintf(sdtNo, "%05d", newMainId);
            std::cout << "\n[�˸�] �޸𸮿��� ���ο� Main Index ����: " << sdtNo << std::endl;
            curMainId = newMainId;
        }
        if (blockNo > 0 && blockNo != curBlockNo) {
            sprintf(sdtNo, "%05d", newMainId);
            std::cout << "[ " << sdtNo << " | " << blockNo << " ] ��� ���� ����" << std::endl;
            curBlockNo = blockNo;
        }
        std::cout << "������ ����(q:����/w:����/o:��ȣ�Է�/r:�ʱ�ȭ) �Ǵ� ���(m:����/b:����/n:����/l:����Ʈ): ";
        int input_ch = _getch();
        if ((0x20 <= input_ch) && (input_ch <= 0x7F)) {
            std::cout << (char)input_ch << std::endl;
        }
        else if (input_ch == 224) {
            input_ch = _getch();
            switch (input_ch) {
            case 'H': std::cout << "�ε��� ����" << std::endl; input_ch = 'w'; break;
            case 'P': std::cout << "�ε��� ����" << std::endl; input_ch = 'q'; break;
            case 'K': std::cout << "���� ���" << std::endl; input_ch = 'b'; break;
            case 'M': std::cout << "���� ���" << std::endl; input_ch = 'n'; break;
            }
        }
        else if (input_ch == 13) {
            std::cout << "���� �ε��� ���" << std::endl;
            input_ch = 'p';
        }
        else if (input_ch == 27) {
            std::cout << "������ ���� �ʱ�ȭ" << std::endl;
            input_ch = 'r';
        }
        else {
            std::cout << (int)input_ch << std::endl;
        }
        std::cout << std::endl;

        int newSubId = DataManager::GetInstance().GetCurrentSubIndex();

        if (input_ch == 'q' || input_ch == 'Q') {
            ScanProcess::GetInstance().ControlPlayOffset(DEC_OFFSET);
            AudioManager::GetInstance().StopAllAudio();
            DataManager::GetInstance().SetCurrentSubIndex(--newSubId);
        }
        else if (input_ch == 'w' || input_ch == 'W') {
            ScanProcess::GetInstance().ControlPlayOffset(ADD_OFFSET);
            AudioManager::GetInstance().StopAllAudio();
            DataManager::GetInstance().SetCurrentSubIndex(++newSubId);
        }
        else if (input_ch == 'p' || input_ch == 'P') {
            AudioManager::GetInstance().PlayCurrentAudio();
        }
        else if (input_ch == 'b' || input_ch == 'B') {
            ScanProcess::GetInstance().ControlPlayOffset(DEC_OFFSET);
            DataManager::GetInstance().SetCurrentSubIndex(--newSubId);
            AudioManager::GetInstance().PlayCurrentAudio();
        }
        else if (input_ch == 'n' || input_ch == 'N') {
            ScanProcess::GetInstance().ControlPlayOffset(ADD_OFFSET);
            DataManager::GetInstance().SetCurrentSubIndex(++newSubId);
            AudioManager::GetInstance().PlayCurrentAudio();
        }
        else if (input_ch == 'r' || input_ch == 'R') {
            std::cout << "������ �� �ʱ�ȭ �� inf ���� ��ε�" << std::endl;
            ScanProcess::GetInstance().ControlPlayOffset(RESET_OFFSET);
            DataManager::GetInstance().ReloadInf();
            continue;
        }
        else if (input_ch == 'l' || input_ch == 'L') {
            std::cout << "����� ���� ����Ʈ ���" << std::endl;
            std::map<int, int> currentAudio = DataManager::GetInstance().GetCurrentAudioIndex();
            int i = 0;
            for (auto it_sub = currentAudio.begin(); it_sub != currentAudio.end(); ++it_sub) {
                int indexNum = it_sub->second;
                if (indexNum == -1) {
                    std::cout << "Index: " << ++i << ", ���ϸ�: [empty_file]" << std::endl;
                }
                else {
                    auto pakFileIndex = DataManager::GetInstance().GetPakFileInfo(indexNum);
                    std::cout << "Index: " << ++i << ", ���ϸ�: " << pakFileIndex->fileName << std::endl;
                }
            }
            continue;
        }
        else if (input_ch == 'o' || input_ch == 'O') {
            offset = ScanProcess::GetInstance().ControlPlayOffset(GET_OFFSET);
            blockNo = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
            std::cout << "���ο� ������ �� �Է� (����: " << offset << "): ";
            std::getline(std::cin, input_sub, '\n');
            try {
                offset = std::stoi(input_sub);
            }
            catch (const std::exception& e) {
                std::cerr << "�߸��� ������ �� �Է��Դϴ�. ���ڸ� �Է��ϼ���. ����: " << e.what() << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            ScanProcess::GetInstance().ControlPlayOffset(SET_OFFSET, offset);
            DataManager::GetInstance().SetCurrentSubIndex(blockNo + offset);
            continue;
        }
        else if (input_ch == 'i' || input_ch == 'I') {
            std::cout << "����� ID �� �Է� (����: " << newSubId << "): ";
            std::getline(std::cin, input_sub, '\n');
            try {
                oncePlayId = std::stoi(input_sub);
            }
            catch (const std::exception& e) {
                std::cerr << "�߸��� ID �� �Է��Դϴ�. ���ڸ� �Է��ϼ���. ����: " << e.what() << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            DataManager::GetInstance().SetCurrentSubIndex(oncePlayId);
            AudioManager::GetInstance().PlayCurrentAudio();
            DataManager::GetInstance().SetCurrentSubIndex(newSubId);
        }
        curMainId = DataManager::GetInstance().GetCurrentMainIndex();
        sprintf(sdtNo, "%05d", curMainId);
        offset = ScanProcess::GetInstance().ControlPlayOffset(GET_OFFSET);
        curBlockNo = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
        std::cout << "[" << sdtNo << "] block: " << curBlockNo << " | offset: " << offset;
        if (oncePlayId != -1) {
            newSubId = DataManager::GetInstance().GetCurrentSubIndex();
        }
        else {
            newSubId = oncePlayId;
            oncePlayId = -1;
        }
        auto indexNum = DataManager::GetInstance().SearchIndex(DataManager::GetInstance().GetCurrentMainIndex(), newSubId);
        if (indexNum != -1) {
            auto pakFileIndex = DataManager::GetInstance().GetPakFileInfo(indexNum);
            std::cout << " | Index: " << newSubId << " | Audio: " << pakFileIndex->fileName;
        }
        else {
            std::cout << " | Index: �ʱ�ȭ ���� ����";
        }
        std::cout << std::endl << std::endl;
    }

    // ----------------------------------------------------
    // ���α׷� ���� �� ������ ���� �� ����
    // ----------------------------------------------------
    ScanProcess::GetInstance().StopScanThread();
    DataManager::GetInstance().StopDataManagerThread();
    AudioManager::GetInstance().Shutdown();
    // ----------------------------------------------------


    std::cout << "RoutesVoice CUI �׽�Ʈ ���α׷� ����." << std::endl;

    return 0;
}
