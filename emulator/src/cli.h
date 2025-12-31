#ifndef _CLI_H_
#define _CLI_H_

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <sstream>
#include <stdint.h>

class CLI {
public:
    using CommandHandler = std::function<void(const std::vector<std::string>& args)>;
    
    // Конструктор
    CLI();
    
    // Регистрация команды
    void register_command(const std::string& name, 
                         const std::string& help_text, 
                         CommandHandler handler);
    
    // Выполнение строки команды
    bool execute(const std::string& line);
    
    // Автодополнение (опционально)
    std::vector<std::string> autocomplete(const std::string& prefix);
    
    // Показать справку
    void show_help();
    
    // Показать справку по конкретной команде
    void show_command_help(const std::string& command);
    
    // История команд (опционально)
    void add_to_history(const std::string& command);
    const std::vector<std::string>& get_history() const;
    

private:
    struct Command {
        std::string name;
        std::string help;
        CommandHandler handler;
    };
    
    std::map<std::string, Command> commands_;
    std::vector<std::string> history_;
    
    // Разбор строки на аргументы
    std::vector<std::string> parse_args(const std::string& line);
    
    // Встроенные команды
    void cmd_help(const std::vector<std::string>& args);
    void cmd_clear(const std::vector<std::string>& args);
    void cmd_history(const std::vector<std::string>& args);
    void cmd_echo(const std::vector<std::string>& args);
};

class CLIUtils {
    public:
    static bool is_hex(const std::string& s);
    static uint32_t hex_to_int(const std::string& s);
    static std::string int_to_hex(uint32_t value, int width = 4);
};

#endif // CLI_H