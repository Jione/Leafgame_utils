#include "LocalizationText.h"
#include "LocalizationGraphic.h"
#include <iostream>
#include <conio.h> // _getch() 사용

int main() {
    // 콘솔에서 한글 출력을 위해 로케일 설정
    setlocale(LC_ALL, "Korean");

    std::wcout << L"==============================================" << std::endl;
    std::wcout << L"      Integrated Localization Tool v1.0       " << std::endl;
    std::wcout << L"==============================================" << std::endl;
    std::wcout << L" 1. Extract Text Resources (event.pak -> KoS)" << std::endl;
    std::wcout << L" 2. Extract Graphic Resources (*.pak -> KoR)" << std::endl;
    std::wcout << L" 3. Building Text Archive (KoS -> KoS.pak)" << std::endl;
    std::wcout << L" 4. Building Graphic Archive (KoR -> KoR.pak)" << std::endl;
    std::wcout << L" 0. Exit" << std::endl;
    std::wcout << L"==============================================" << std::endl;
    std::wcout << L"Select Mode > ";

    char choice = _getch();
    std::wcout << choice << std::endl << std::endl;

    if (choice == '1') {
        Localization::ExtractTextResources();
    }
    else if (choice == '2') {
        // 구현 예정
        Localization::ExtractGraphicResources();
    }
    else if (choice == '3') {
        Localization::BuildTextArchive();
    }
    else if (choice == '4') {
        // 구현 예정
        Localization::BuildGraphicArchive();
    }
    else if (choice == '0') {
        std::wcout << L"Exiting..." << std::endl;
    }
    else {
        std::wcout << L"Invalid selection." << std::endl;
    }

    std::wcout << L"\nPress any key to close...";
    _getch();
    return 0;
}