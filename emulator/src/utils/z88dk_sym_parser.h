#ifndef Z88DK_SYM_PARSER_H
#define Z88DK_SYM_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <memory>
#include <optional>
#include <iomanip>
#include <iostream>
#include <cctype>

// Представляет расположение в исходном коде
struct SourceLocation {
    std::string filename;
    int line = 0;
    
    SourceLocation() = default;
    SourceLocation(const std::string& f, int l) : filename(f), line(l) {}
    
    std::string to_string() const {
        if (filename.empty()) return "";
        return filename + ":" + std::to_string(line);
    }
    
    bool operator==(const SourceLocation& other) const {
        return filename == other.filename && line == other.line;
    }
    
    bool operator!=(const SourceLocation& other) const {
        return !(*this == other);
    }
};

// Информация о символе
struct SymbolInfo {
    std::string name;
    uint16_t address = 0;
    std::string type;           // const, public, local и т.д.
    std::string section;        // сегмент (.text, .data и т.д.)
    SourceLocation location;    // расположение в исходном коде
    
    // Для отладки/поиска
    uint16_t size = 0;          // размер символа (если известен)
    bool is_function = false;   // является ли функцией
    std::string comment;        // комментарий
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << std::hex << "0x" << std::setfill('0') << std::setw(4) << address;
        oss << " " << name;
        if (!type.empty()) oss << " [" << type << "]";
        if (location.line > 0) oss << " @ " << location.to_string();
        return oss.str();
    }
    
    bool operator==(const SymbolInfo& other) const {
        return address == other.address && name == other.name;
    }
    
    bool operator!=(const SymbolInfo& other) const {
        return !(*this == other);
    }
};

// Класс для парсинга .sym файлов Z88DK
class Z88dkSymParser {
private:
    std::vector<SymbolInfo> symbols_;
    std::unordered_map<uint16_t, size_t> address_index_; // address -> index in symbols_
    std::unordered_map<std::string, size_t> name_index_; // name -> index in symbols_
    
    // Кэш для быстрого поиска
    mutable std::vector<uint16_t> sorted_addresses_;
    mutable bool needs_sort_ = true;
    
    // Вспомогательная функция для преобразования в нижний регистр
    static std::string to_lower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
    
    // Разбор строки символа
    SymbolInfo parse_symbol_line(const std::string& line) {
        SymbolInfo info;
        
        try {
            // Пример строки: 
            // IOCTL_ITERM_CRLF = $C101 ; const, public, , , , /home/.../config_z80_public.inc:710
            // _main = $8000 ; public, , , , , main.c:5
            
            // Удаляем начальные/конечные пробелы
            std::string trimmed = line;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start == std::string::npos) {
                return info; // Пустая строка
            }
            trimmed = trimmed.substr(start);
            
            size_t end = trimmed.find_last_not_of(" \t");
            if (end != std::string::npos) {
                trimmed = trimmed.substr(0, end + 1);
            }
            
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
                return info; // Пустая или комментарий
            }
            
            // Разделяем имя и остальное
            size_t equals_pos = trimmed.find('=');
            if (equals_pos == std::string::npos) {
                // Может быть формата: 8000 _main
                size_t space_pos = trimmed.find(' ');
                if (space_pos != std::string::npos) {
                    std::string addr_str = trimmed.substr(0, space_pos);
                    info.name = trimmed.substr(space_pos + 1);
                    
                    // Очищаем имя от пробелов
                    size_t name_start = info.name.find_first_not_of(" \t");
                    if (name_start != std::string::npos) {
                        info.name = info.name.substr(name_start);
                        size_t name_end = info.name.find_last_not_of(" \t");
                        if (name_end != std::string::npos) {
                            info.name = info.name.substr(0, name_end + 1);
                        }
                    }
                    
                    info.address = parse_address(addr_str);
                }
                return info;
            }
            
            // Извлекаем имя
            info.name = trimmed.substr(0, equals_pos);
            size_t name_start = info.name.find_first_not_of(" \t");
            if (name_start != std::string::npos) {
                info.name = info.name.substr(name_start);
                size_t name_end = info.name.find_last_not_of(" \t");
                if (name_end != std::string::npos) {
                    info.name = info.name.substr(0, name_end + 1);
                }
            }
            
            // Извлекаем адрес и комментарий
            std::string rest = trimmed.substr(equals_pos + 1);
            
            // Ищем адрес (может быть в формате $C101 или 0xC101)
            size_t addr_start = rest.find_first_not_of(" \t");
            if (addr_start == std::string::npos) {
                return info;
            }
            
            size_t addr_end = rest.find(';', addr_start);
            if (addr_end == std::string::npos) {
                addr_end = rest.find(' ', addr_start);
                if (addr_end == std::string::npos) {
                    addr_end = rest.length();
                }
            }
            
            std::string addr_str = rest.substr(addr_start, addr_end - addr_start);
            info.address = parse_address(addr_str);
            
            // Ищем комментарий с информацией
            size_t comment_start = rest.find(';', addr_end);
            if (comment_start != std::string::npos) {
                std::string comment = rest.substr(comment_start + 1);
                
                // Разбиваем комментарий по запятым
                std::vector<std::string> parts;
                std::istringstream comment_stream(comment);
                std::string part;
                
                while (std::getline(comment_stream, part, ',')) {
                    // Удаляем пробелы
                    size_t part_start = part.find_first_not_of(" \t");
                    if (part_start != std::string::npos) {
                        part = part.substr(part_start);
                        size_t part_end = part.find_last_not_of(" \t");
                        if (part_end != std::string::npos) {
                            part = part.substr(0, part_end + 1);
                        }
                        if (!part.empty()) {
                            parts.push_back(part);
                        }
                    }
                }
                
                // Парсим части комментария
                if (!parts.empty()) info.type = parts[0];
                
                // Последняя часть может быть filename:line
                if (!parts.empty()) {
                    std::string last = parts.back();
                    size_t colon_pos = last.find(':');
                    if (colon_pos != std::string::npos) {
                        info.location.filename = last.substr(0, colon_pos);
                        try {
                            info.location.line = std::stoi(last.substr(colon_pos + 1));
                        } catch (...) {
                            info.location.line = 0;
                        }
                    }
                }
                
                info.comment = comment;
            }
            
            // Определяем, является ли символ функцией
            info.is_function = (info.type.find("function") != std::string::npos) ||
                               (info.type.find("code") != std::string::npos) ||
                               (info.name[0] == '_' && 
                                !info.name.empty() && 
                                info.name.find("_data") == std::string::npos &&
                                info.name.find("_bss") == std::string::npos);
            
        } catch (const std::exception& e) {
            // В случае ошибки парсинга, оставляем пустой символ
            std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
        }
        
        return info;
    }
    
    // Парсинг адреса из разных форматов
    uint16_t parse_address(const std::string& addr_str) {
        std::string str = addr_str;
        
        // Удаляем пробелы
        size_t start = str.find_first_not_of(" \t");
        if (start == std::string::npos) return 0;
        str = str.substr(start);
        
        size_t end = str.find_last_not_of(" \t");
        if (end != std::string::npos) {
            str = str.substr(0, end + 1);
        }
        
        if (str.empty()) return 0;
        
        // Определяем основание системы счисления
        int base = 10;
        size_t start_pos = 0;
        
        if (str.size() > 1 && str[0] == '$') {
            // Формат $C101
            base = 16;
            start_pos = 1;
        } else if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            // Формат 0xC101
            base = 16;
            start_pos = 2;
        } else {
            // Проверяем, содержит ли строка только шестнадцатеричные цифры
            bool is_hex = true;
            for (size_t i = 0; i < str.size(); ++i) {
                char c = str[i];
                if (!((c >= '0' && c <= '9') || 
                      (c >= 'a' && c <= 'f') || 
                      (c >= 'A' && c <= 'F'))) {
                    is_hex = false;
                    break;
                }
            }
            
            if (is_hex && str.size() >= 2) {
                // Шестнадцатеричное без префикса
                base = 16;
                start_pos = 0;
            }
        }
        
        try {
            unsigned long value = std::stoul(str.substr(start_pos), nullptr, base);
            if (value > 0xFFFF) {
                std::cerr << "Warning: Address out of 16-bit range: " << str << std::endl;
                value = 0xFFFF;
            }
            return static_cast<uint16_t>(value);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing address: " << str << " - " << e.what() << std::endl;
            return 0;
        }
    }
    
    // Обновление кэша отсортированных адресов
    void update_sorted_cache() const {
        if (!needs_sort_) return;
        
        sorted_addresses_.clear();
        sorted_addresses_.reserve(symbols_.size());
        
        for (const auto& sym : symbols_) {
            if (sym.address > 0 && !sym.name.empty()) {
                sorted_addresses_.push_back(sym.address);
            }
        }
        
        std::sort(sorted_addresses_.begin(), sorted_addresses_.end());
        sorted_addresses_.erase(std::unique(sorted_addresses_.begin(), sorted_addresses_.end()), 
                               sorted_addresses_.end());
        
        const_cast<bool&>(needs_sort_) = false;
    }
    
    // Бинарный поиск ближайшего адреса
    std::optional<size_t> find_nearest_symbol_index(uint16_t address) const {
        if (sorted_addresses_.empty()) return std::nullopt;
        
        update_sorted_cache();
        
        auto it = std::lower_bound(sorted_addresses_.begin(), sorted_addresses_.end(), address);
        
        if (it == sorted_addresses_.end()) {
            // Все адреса меньше, берем последний
            auto last_it = address_index_.find(sorted_addresses_.back());
            if (last_it != address_index_.end()) {
                return last_it->second;
            }
            return std::nullopt;
        }
        
        if (it == sorted_addresses_.begin()) {
            // Первый адрес уже больше или равен
            auto first_it = address_index_.find(*it);
            if (first_it != address_index_.end()) {
                return first_it->second;
            }
            return std::nullopt;
        }
        
        // Находим ближайший адрес
        uint16_t next_addr = *it;
        uint16_t prev_addr = *(it - 1);
        
        // Выбираем ближайший по расстоянию
        int distance_to_prev = static_cast<int>(address) - static_cast<int>(prev_addr);
        int distance_to_next = static_cast<int>(next_addr) - static_cast<int>(address);
        
        if (distance_to_prev <= distance_to_next) {
            auto prev_it = address_index_.find(prev_addr);
            if (prev_it != address_index_.end()) {
                return prev_it->second;
            }
        } else {
            auto next_it = address_index_.find(next_addr);
            if (next_it != address_index_.end()) {
                return next_it->second;
            }
        }
        
        return std::nullopt;
    }

public:
    Z88dkSymParser() = default;
    
    // Загрузка файла .sym
    bool load_file(const std::string& path) {
        symbols_.clear();
        address_index_.clear();
        name_index_.clear();
        sorted_addresses_.clear();
        needs_sort_ = true;
        
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << path << std::endl;
            return false;
        }
        
        std::string line;
        int line_num = 0;
        size_t loaded_count = 0;
        
        while (std::getline(file, line)) {
            line_num++;
            
            SymbolInfo sym = parse_symbol_line(line);
            
            // Добавляем только валидные символы
            if (!sym.name.empty() && sym.address > 0) {
                // Проверяем на дубликаты
                bool duplicate = false;
                
                auto addr_it = address_index_.find(sym.address);
                if (addr_it != address_index_.end()) {
                    std::cerr << "Warning: Duplicate address at line " << line_num 
                              << ": 0x" << std::hex << sym.address 
                              << " (" << sym.name << " vs " 
                              << symbols_[addr_it->second].name << ")" << std::dec << std::endl;
                    duplicate = true;
                }
                
                auto name_it = name_index_.find(sym.name);
                if (name_it != name_index_.end()) {
                    std::cerr << "Warning: Duplicate name at line " << line_num 
                              << ": " << sym.name 
                              << " (0x" << std::hex << sym.address << " vs 0x" 
                              << symbols_[name_it->second].address << ")" << std::dec << std::endl;
                    duplicate = true;
                }
                
                if (!duplicate) {
                    symbols_.push_back(sym);
                    size_t index = symbols_.size() - 1;
                    address_index_[sym.address] = index;
                    name_index_[sym.name] = index;
                    loaded_count++;
                }
            }
        }
        
        std::cout << "Loaded " << loaded_count << " symbols from " << path << std::endl;
        return loaded_count > 0;
    }
    
    // Очистка всех данных
    void clear() {
        symbols_.clear();
        address_index_.clear();
        name_index_.clear();
        sorted_addresses_.clear();
        needs_sort_ = true;
    }
    
    // Получение имени символа по точному адресу
    std::string get_name(uint16_t address) const {
        auto it = address_index_.find(address);
        if (it != address_index_.end()) {
            return symbols_[it->second].name;
        }
        return "";
    }
    
    // Получение имени символа по адресу (с поиском ближайшего)
    std::string get_name(uint16_t address, uint16_t tolerance, bool& exact_match) const {
        exact_match = false;
        
        // Сначала пробуем точное совпадение
        auto exact_it = address_index_.find(address);
        if (exact_it != address_index_.end()) {
            exact_match = true;
            return symbols_[exact_it->second].name;
        }
        
        // Если tolerance = 0, то только точное совпадение
        if (tolerance == 0) {
            return "";
        }
        
        // Ищем ближайший символ в пределах tolerance
        update_sorted_cache();
        
        for (uint16_t addr : sorted_addresses_) {
            int distance = std::abs(static_cast<int>(addr) - static_cast<int>(address));
            if (distance <= static_cast<int>(tolerance)) {
                const SymbolInfo& sym = symbols_[address_index_.at(addr)];
                std::string result = sym.name;
                if (address != addr) {
                    int offset = static_cast<int>(address) - static_cast<int>(addr);
                    if (offset != 0) {
                        result += (offset > 0 ? "+" : "") + std::to_string(offset);
                    }
                }
                return result;
            }
        }
        
        return "";
    }
    
    // Получение имени символа по адресу (простая версия)
    std::string get_name(uint16_t address, uint16_t tolerance = 0) const {
        bool exact;
        return get_name(address, tolerance, exact);
    }
    
    // Получение расположения в исходном коде
    SourceLocation get_location(uint16_t address) const {
        auto it = address_index_.find(address);
        if (it != address_index_.end()) {
            return symbols_[it->second].location;
        }
        return SourceLocation();
    }
    
    // Получение расположения в исходном коде (с поиском ближайшего)
    SourceLocation get_location(uint16_t address, uint16_t tolerance) const {
        // Точное совпадение
        auto exact_it = address_index_.find(address);
        if (exact_it != address_index_.end()) {
            return symbols_[exact_it->second].location;
        }
        
        // Поиск ближайшего
        auto nearest = find_nearest_symbol_index(address);
        if (nearest && tolerance > 0) {
            const SymbolInfo& sym = symbols_[*nearest];
            int distance = std::abs(static_cast<int>(address) - static_cast<int>(sym.address));
            if (distance <= static_cast<int>(tolerance)) {
                return sym.location;
            }
        }
        
        return SourceLocation();
    }
    
    // Получение исходного файла
    std::string get_source(uint16_t address) const {
        return get_location(address).filename;
    }
    
    // Получение всей информации о символе
    std::optional<SymbolInfo> get_symbol(uint16_t address) const {
        auto it = address_index_.find(address);
        if (it != address_index_.end()) {
            return symbols_[it->second];
        }
        return std::nullopt;
    }
    
    // Поиск символа по имени (без учета регистра)
    std::optional<SymbolInfo> find_symbol_by_name(const std::string& name) const {
        // Прямой поиск
        auto it = name_index_.find(name);
        if (it != name_index_.end()) {
            return symbols_[it->second];
        }
        
        // Поиск без учета регистра
        std::string lower_name = to_lower(name);
        
        for (const auto& sym : symbols_) {
            if (to_lower(sym.name) == lower_name) {
                return sym;
            }
        }
        
        return std::nullopt;
    }
    
    // Поиск символов по подстроке в имени
    std::vector<SymbolInfo> find_symbols_by_substring(const std::string& substring, bool case_sensitive = false) const {
        std::vector<SymbolInfo> result;
        std::string search_str = case_sensitive ? substring : to_lower(substring);
        
        for (const auto& sym : symbols_) {
            std::string sym_name = case_sensitive ? sym.name : to_lower(sym.name);
            if (sym_name.find(search_str) != std::string::npos) {
                result.push_back(sym);
            }
        }
        
        return result;
    }
    
    // Получение всех символов в диапазоне адресов
    std::vector<SymbolInfo> get_symbols_in_range(uint16_t start_addr, uint16_t end_addr) const {
        std::vector<SymbolInfo> result;
        
        for (const auto& sym : symbols_) {
            if (sym.address >= start_addr && sym.address <= end_addr) {
                result.push_back(sym);
            }
        }
        
        // Сортируем по адресу
        std::sort(result.begin(), result.end(), 
                 [](const SymbolInfo& a, const SymbolInfo& b) {
                     return a.address < b.address;
                 });
        
        return result;
    }
    
    // Получение ближайших символов к адресу
    std::vector<SymbolInfo> get_nearest_symbols(uint16_t address, size_t count = 5) const {
        std::vector<std::pair<int, size_t>> distances; // distance, index
        
        for (size_t i = 0; i < symbols_.size(); ++i) {
            if (symbols_[i].address > 0) {
                int distance = std::abs(static_cast<int>(symbols_[i].address) - 
                                        static_cast<int>(address));
                distances.emplace_back(distance, i);
            }
        }
        
        // Сортируем по расстоянию
        std::sort(distances.begin(), distances.end(),
                 [](const auto& a, const auto& b) { return a.first < b.first; });
        
        // Берем первые count
        std::vector<SymbolInfo> result;
        for (size_t i = 0; i < std::min(count, distances.size()); ++i) {
            result.push_back(symbols_[distances[i].second]);
        }
        
        return result;
    }
    
    // Получение символа, содержащего адрес (например, если адрес внутри функции)
    std::optional<SymbolInfo> get_symbol_containing_address(uint16_t address, uint16_t max_size = 0x100) const {
        // Сначала ищем точное совпадение
        auto exact = get_symbol(address);
        if (exact) return exact;
        
        // Ищем символы, которые могут содержать этот адрес
        // (предполагаем, что символы отсортированы по адресу)
        std::vector<SymbolInfo> sorted = symbols_;
        std::sort(sorted.begin(), sorted.end(),
                 [](const SymbolInfo& a, const SymbolInfo& b) {
                     return a.address < b.address;
                 });
        
        // Ищем последний символ с адресом меньше или равным искомому
        for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
            if (it->address <= address) {
                // Проверяем, может ли адрес быть внутри этого символа
                uint16_t end_address = it->address + max_size;
                if (address < end_address) {
                    // Возвращаем символ с информацией о смещении
                    SymbolInfo result = *it;
                    if (it->size > 0) {
                        // Если известен размер символа
                        if (address < it->address + it->size) {
                            return result;
                        }
                    } else {
                        // Если размер неизвестен, предполагаем максимальный
                        return result;
                    }
                }
                break;
            }
        }
        
        return std::nullopt;
    }
    
    // Статистика
    size_t get_symbol_count() const { return symbols_.size(); }
    
    // Получение всех символов (отсортированных по адресу)
    std::vector<SymbolInfo> get_all_symbols_sorted() const {
        std::vector<SymbolInfo> result = symbols_;
        std::sort(result.begin(), result.end(),
                 [](const SymbolInfo& a, const SymbolInfo& b) {
                     return a.address < b.address;
                 });
        return result;
    }
    
    // Получение всех символов
    const std::vector<SymbolInfo>& get_all_symbols() const { return symbols_; }
    
    // Проверка, загружены ли символы
    bool is_loaded() const { return !symbols_.empty(); }
    
    // Экспорт в простой формат (адрес имя)
    bool export_simple(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) {
            std::cerr << "Cannot open file for writing: " << path << std::endl;
            return false;
        }
        
        // Сортируем по адресу
        auto sorted = get_all_symbols_sorted();
        
        for (const auto& sym : sorted) {
            out << std::hex << std::setfill('0') << std::setw(4) 
                << sym.address << " " << sym.name << std::endl;
        }
        
        std::cout << "Exported " << sorted.size() << " symbols to " << path << std::endl;
        return true;
    }
    
    // Экспорт в формат Z88DK .sym
    bool export_z88dk_format(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) {
            std::cerr << "Cannot open file for writing: " << path << std::endl;
            return false;
        }
        
        auto sorted = get_all_symbols_sorted();
        
        for (const auto& sym : sorted) {
            out << sym.name << " = $" << std::hex << std::setfill('0') 
                << std::setw(4) << sym.address;
            
            // Добавляем комментарий, если есть информация
            if (!sym.type.empty() || sym.location.line > 0) {
                out << " ; " << sym.type;
                if (!sym.location.filename.empty()) {
                    out << ", , , , , " << sym.location.filename;
                    if (sym.location.line > 0) {
                        out << ":" << std::dec << sym.location.line;
                    }
                }
            }
            out << std::endl;
        }
        
        std::cout << "Exported " << sorted.size() << " symbols to Z88DK format: " << path << std::endl;
        return true;
    }
};

#endif // Z88DK_SYM_PARSER_H