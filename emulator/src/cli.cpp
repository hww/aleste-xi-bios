#include "cli.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include "globals.h"
#include "utils/ibuffer.h"

CLI::CLI() {
    // Регистрируем только базовые команды CLI
    register_command("help", "Show help", 
                    [this](const auto& args) { cmd_help(args); });
    register_command("?", "Show help (short)", 
                    [this](const auto& args) { cmd_help(args); });
    register_command("clear", "Clear screen", 
                    [this](const auto& args) { cmd_clear(args); });
    register_command("cls", "Clear screen (DOS style)", 
                    [this](const auto& args) { cmd_clear(args); });
    register_command("history", "Show command history", 
                    [this](const auto& args) { cmd_history(args); });
    register_command("echo", "Echo arguments", 
                    [this](const auto& args) { cmd_echo(args); });
}

void CLI::register_command(const std::string& name, 
                          const std::string& help_text, 
                          CommandHandler handler) {
    Command cmd;
    cmd.name = name;
    cmd.help = help_text;
    cmd.handler = handler;
    commands_[name] = cmd;
}

std::vector<std::string> CLI::parse_args(const std::string& line) {
    std::vector<std::string> args;
    std::stringstream ss(line);
    std::string arg;
    
    while (ss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

bool CLI::execute(const std::string& line) {
    if (line.empty()) return false;
    
    add_to_history(line);
    
    std::vector<std::string> args = parse_args(line);
    if (args.empty()) return false;
    
    std::string cmd_name = args[0];
    
    // Ищем команду (регистронезависимо для удобства)
    std::string lower_cmd = cmd_name;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    auto it = commands_.find(lower_cmd);
    if (it == commands_.end()) {
        // Попробуем найти по префиксу (как в старых мониторах)
        for (const auto& [name, cmd] : commands_) {
            if (name.find(lower_cmd) == 0) {
                it = commands_.find(name);
                break;
            }
        }
        
        if (it == commands_.end()) {
            ibuffer.printf("Unknown command: %s\n", cmd_name);
            ibuffer.printf("Type 'help' for available commands\n");
            return false;
        }
    }
    
    // Удаляем имя команды из аргументов
    args.erase(args.begin());
    
    try {
        it->second.handler(args);
        return true;
    } catch (const std::exception& e) {
        ibuffer.printf("Error executing command: %s\n", e.what());
        return false;
    }
}

std::vector<std::string> CLI::autocomplete(const std::string& prefix) {
    std::vector<std::string> matches;
    
    if (prefix.empty()) {
        for (const auto& [name, cmd] : commands_) {
            matches.push_back(name);
        }
    } else {
        for (const auto& [name, cmd] : commands_) {
            if (name.find(prefix) == 0) {
                matches.push_back(name);
            }
        }
    }
    
    return matches;
}

void CLI::show_help() {
    ibuffer.printf("Available commands:\n");;
    ibuffer.printf("==================\n");

    for (const auto& [name, cmd] : commands_) {
        ibuffer.printf("  %s - %s\n",  name, cmd.help);
    }
}

void CLI::show_command_help(const std::string& command) {
    auto it = commands_.find(command);
    if (it != commands_.end()) {
        ibuffer.printf("Command: %s\n", it->second.name);
        ibuffer.printf("Description: %s\n", it->second.help);
    } else {
        ibuffer.printf("Unknown command: %s\n", command);
    }
}

void CLI::add_to_history(const std::string& command) {
    history_.push_back(command);
    
    // Ограничиваем историю
    if (history_.size() > 100) {
        history_.erase(history_.begin());
    }
}

const std::vector<std::string>& CLI::get_history() const {
    return history_;
}

// === Встроенные команды ===

void CLI::cmd_help(const std::vector<std::string>& args) {
    if (args.empty()) {
        show_help();
    } else {
        show_command_help(args[0]);
    }
}

void CLI::cmd_clear(const std::vector<std::string>& args) {
    std::cout << "\033[2J\033[1;1H";  // ANSI escape codes
}

void CLI::cmd_history(const std::vector<std::string>& args) {
    ibuffer.printf("Command history:\n");
    ibuffer.printf("================\n");
    
    for (size_t i = 0; i < history_.size(); ++i) {
        ibuffer.printf("  %d %s\n", i + 1, history_[i]);
    }
}

void CLI::cmd_echo(const std::vector<std::string>& args) {
    for (const auto& arg : args) {
        std::cout << arg << " ";
    }
    std::cout << std::endl;
}

// === CLIUtils ===

bool CLIUtils::is_hex(const std::string& s) {
    if (s.empty()) return false;
    
    // Проверяем префикс 0x
    size_t start = 0;
    if (s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        start = 2;
    }
    
    // Проверяем остальные символы
    for (size_t i = start; i < s.length(); ++i) {
        if (!isxdigit(s[i])) return false;
    }
    
    return true;
}

uint32_t CLIUtils::hex_to_int(const std::string& s) {
    return std::stoul(s, nullptr, 16);
}

std::string CLIUtils::int_to_hex(uint32_t value, int width) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return ss.str();
}