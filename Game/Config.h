#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    // ������� reload() ��������� ��������� �� ����� settings.json,
    // ����� �������� ������������ ��� ����������� ���������.
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // �������� ������� ������ ��������� ���������� � ������� Config
    // ��� � ������� ��� ��������� �������� ��������� �� ����� ������� � ���������.
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
