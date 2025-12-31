#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <cstdint>  // Добавляем для uint32_t

class CLI {
public:
    using CommandHandler = std::function<void(const std::vector<std::string>&)>;
    
    struct Command {
        std::string name;
        std::string help;
        CommandHandler handler;
    };
    
    CLI();
    
    void register_command(const std::string& name, 
                         const std::string& help_text, 
                         CommandHandler handler);
    
    bool execute(const std::string& line);
    
    std::vector<std::string> autocomplete(const std::string& prefix);
    
    void show_help();
    void show_command_help(const std::string& command);
    
    void add_to_history(const std::string& command);
    const std::vector<std::string>& get_history() const;
    
private:
    std::vector<std::string> parse_args(const std::string& line);
    
    std::map<std::string, Command> commands_;
    std::vector<std::string> history_;
    
    // Встроенные команды
    void cmd_help(const std::vector<std::string>& args);
    void cmd_clear(const std::vector<std::string>& args);
    void cmd_history(const std::vector<std::string>& args);
    void cmd_echo(const std::vector<std::string>& args);
};

namespace CLIUtils {
    bool is_hex(const std::string& s);
    uint32_t hex_to_int(const std::string& s);
    std::string int_to_hex(uint32_t value, int width = 4);
}

#endif // CLI_H