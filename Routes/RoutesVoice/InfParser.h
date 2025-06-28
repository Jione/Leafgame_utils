#pragma once

#include "GlobalDefinitions.h"
#include <string>
#include <vector>
#include <map>       // mainIndex�� subIndex ������ ����
#include <fstream>   // ifstream
#include <iostream>  // cout

class InfParser {
public:
    // INF ������ �Ľ��Ͽ� mainIndex -> (subIndex -> value) ������ �ε��մϴ�.
    // ������ outInfMappings�� �� �Լ� ���ο��� �����ϵ��� �����մϴ�.
    bool SaveDefault(const std::wstring& infFilePath);
    bool LoadDefault();
    bool Parse(const std::wstring& infFilePath);

    // �Ľ̵� ��ü INF ���� ��ȯ�մϴ�.
    const std::map<int, std::map<int, int>>& GetInfMappings() const { return m_infMappings; }

private:
    // �ؽ�Ʈ ���ҽ� �ε� �Լ�
    std::string LoadTextResource(int resourceId, const char* resourceType);
    std::map<int, std::map<int, int>> m_infMappings; // main_index -> (sub_index -> value) ����

    // Helper �Լ�: ���� �޽��� ��� (�ʿ�� �߰�)
};