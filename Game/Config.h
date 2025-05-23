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

    // Функция reload() загружает настройки из файла settings.json,
    // чтобы обновить конфигурацию без перезапуска программы.
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // Оператор круглые скобки позволяет обращаться к объекту Config
    // как к функции для получения значения настройки по имени раздела и параметра.
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
