#include <iostream>
#include "cli.h"
#include "globals.h"

void init_commands() {
    cli.register_command("dump", "Dump memory <start> <end>", 
        [](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                std::cout << "Usage: dump <start> <end>" << std::endl;
                return;
            }
            
            if (!CLIUtils::is_hex(args[0]) || !CLIUtils::is_hex(args[1])) {
                std::cout << "Error: addresses must be in hex" << std::endl;
                return;
            }
            
            uint32_t start = CLIUtils::hex_to_int(args[0]);
            uint32_t end = CLIUtils::hex_to_int(args[1]);
            
            std::cout << "Dumping memory " 
                    << CLIUtils::int_to_hex(start) << "-" 
                    << CLIUtils::int_to_hex(end) << std::endl;
        });
}