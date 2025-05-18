#pragma once
#include <vector>

namespace CompareConfig {
    // ù ��° ����: ���� ���� �� ��ǥ ����
    static const std::vector<wchar_t> openingChars = {
        L'\u3000', // ���� ����
        L'\u300C', // ��
        L'\u300E'  // ��
    };

    // ������ ����: ��ǥ �ݱ�
    static const std::vector<wchar_t> closingChars = {
        L'\u300D', // ��
        L'\u300F'  // ��
    };

    // ��ǥ ���� �񱳿�
    static const std::vector<wchar_t> quoteChars = {
        L'\u300C', // ��
        L'\u300E', // ��
        L'\u300D', // ��
        L'\u300F'  // ��
    };
}