#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <conio.h>
#include <windows.h>

#include "DataManager.h"
#include "AudioManager.h"
#include "ScanProcess.h" // 새로 추가

#undef max

int main() {
    std::cout << "RoutesVoice CUI 테스트 프로그램 시작." << std::endl;
    std::cout << "------------------------------------" << std::endl;

    // 프로세스 메모리 스캔 스레드 시작 (ScanProcess 클래스 사용)
    DataManager::GetInstance().StartDataManagerThread();
    ScanProcess::GetInstance().StartScanThread();
    if (!AudioManager::GetInstance().Initialize()) {
        std::cerr << "오디오 시스템 초기화 실패. 프로그램을 종료합니다." << std::endl;
        return 1;
    }
    int count = 0;
    std::cout << "오디오 시스템 초기화 중..";
    while (true) {
        if ((DataManager::GetInstance().GetStatus() == LOAD)) {
            break;
        }
        if (++count == 6) {
            count = 0;
            std::cout << "\r오디오 시스템 초기화 중..      " << "\r오디오 시스템 초기화 중..";
        }
        else {
            std::cout << ".";
        }
        Sleep(120);
    }
    std::cout << "\r오디오 시스템 초기화 성공 (DirectSound)." << std::endl;
    // ----------------------------------------------------

    std::string input_main, input_sub;
    int curMainId = -1; // 메모리에서 읽어온 값 추적용 (메인)
    int curBlockNo = -1;  // 메모리에서 읽어온 값 추적용 (블록)
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
            std::cout << "\n[알림] 메모리에서 새로운 Main Index 감지: " << sdtNo << std::endl;
            curMainId = newMainId;
        }
        if (blockNo > 0 && blockNo != curBlockNo) {
            sprintf(sdtNo, "%05d", newMainId);
            std::cout << "[ " << sdtNo << " | " << blockNo << " ] 블록 변경 감지" << std::endl;
            curBlockNo = blockNo;
        }
        std::cout << "오프셋 변경(q:이전/w:다음/o:번호입력/r:초기화) 또는 재생(m:현재/b:이전/n:다음/l:리스트): ";
        int input_ch = _getch();
        if ((0x20 <= input_ch) && (input_ch <= 0x7F)) {
            std::cout << (char)input_ch << std::endl;
        }
        else if (input_ch == 224) {
            input_ch = _getch();
            switch (input_ch) {
            case 'H': std::cout << "인덱스 증가" << std::endl; input_ch = 'w'; break;
            case 'P': std::cout << "인덱스 감소" << std::endl; input_ch = 'q'; break;
            case 'K': std::cout << "이전 재생" << std::endl; input_ch = 'b'; break;
            case 'M': std::cout << "다음 재생" << std::endl; input_ch = 'n'; break;
            }
        }
        else if (input_ch == 13) {
            std::cout << "현재 인덱스 재생" << std::endl;
            input_ch = 'p';
        }
        else if (input_ch == 27) {
            std::cout << "오프셋 설정 초기화" << std::endl;
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
            std::cout << "오프셋 값 초기화 및 inf 파일 재로드" << std::endl;
            ScanProcess::GetInstance().ControlPlayOffset(RESET_OFFSET);
            DataManager::GetInstance().ReloadInf();
            continue;
        }
        else if (input_ch == 'l' || input_ch == 'L') {
            std::cout << "오디오 파일 리스트 출력" << std::endl;
            std::map<int, int> currentAudio = DataManager::GetInstance().GetCurrentAudioIndex();
            int i = 0;
            for (auto it_sub = currentAudio.begin(); it_sub != currentAudio.end(); ++it_sub) {
                int indexNum = it_sub->second;
                if (indexNum == -1) {
                    std::cout << "Index: " << ++i << ", 파일명: [empty_file]" << std::endl;
                }
                else {
                    auto pakFileIndex = DataManager::GetInstance().GetPakFileInfo(indexNum);
                    std::cout << "Index: " << ++i << ", 파일명: " << pakFileIndex->fileName << std::endl;
                }
            }
            continue;
        }
        else if (input_ch == 'o' || input_ch == 'O') {
            offset = ScanProcess::GetInstance().ControlPlayOffset(GET_OFFSET);
            blockNo = ScanProcess::GetInstance().ControlPlayOffset(GET_BLOCK);
            std::cout << "새로운 오프셋 값 입력 (현재: " << offset << "): ";
            std::getline(std::cin, input_sub, '\n');
            try {
                offset = std::stoi(input_sub);
            }
            catch (const std::exception& e) {
                std::cerr << "잘못된 오프셋 값 입력입니다. 숫자를 입력하세요. 오류: " << e.what() << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            ScanProcess::GetInstance().ControlPlayOffset(SET_OFFSET, offset);
            DataManager::GetInstance().SetCurrentSubIndex(blockNo + offset);
            continue;
        }
        else if (input_ch == 'i' || input_ch == 'I') {
            std::cout << "재생할 ID 값 입력 (현재: " << newSubId << "): ";
            std::getline(std::cin, input_sub, '\n');
            try {
                oncePlayId = std::stoi(input_sub);
            }
            catch (const std::exception& e) {
                std::cerr << "잘못된 ID 값 입력입니다. 숫자를 입력하세요. 오류: " << e.what() << std::endl;
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
            std::cout << " | Index: 초기화 되지 않음";
        }
        std::cout << std::endl << std::endl;
    }

    // ----------------------------------------------------
    // 프로그램 종료 시 스레드 종료 및 정리
    // ----------------------------------------------------
    ScanProcess::GetInstance().StopScanThread();
    DataManager::GetInstance().StopDataManagerThread();
    AudioManager::GetInstance().Shutdown();
    // ----------------------------------------------------


    std::cout << "RoutesVoice CUI 테스트 프로그램 종료." << std::endl;

    return 0;
}
