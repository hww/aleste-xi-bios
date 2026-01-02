#include "ibuffer.h"
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdio>  
#include <iostream>

IBuffer::IBuffer(size_t max_size, int visible_lines) 
    : max_size_(max_size), current_size_(0), scroll_pos_(0), visible_lines_(visible_lines) 
{
    if (visible_lines_ < 1) visible_lines_ = 20;
}

void IBuffer::print(const std::string& text) {
    if (text.empty()) return;
    
    std::string_view remaining(text);
    
    while (!remaining.empty()) {
        // Ищем первый перевод строки
        size_t newline_pos = remaining.find('\n');
        
        if (newline_pos == std::string_view::npos) {
            // Нет \n - весь оставшийся текст
            if (!lines_.empty()) {
                // Добавляем к последней строке
                std::string& last_line = lines_.back();
                size_t new_size = current_size_ - last_line.length() + remaining.length();
                
                if (new_size <= max_size_) {
                    last_line.append(remaining);
                    current_size_ = new_size;
                    if (line_added_cb_) line_added_cb_(last_line);
                    return;
                }
            }
            
            // Нет последней строки или не помещается - создаем новую
            add_line(std::string(remaining));
            return;
        }
        
        // Есть \n - обрабатываем строку до него
        std::string_view line_part = remaining.substr(0, newline_pos);
        
        if (!lines_.empty()) {
            // Добавляем к последней строке
            std::string& last_line = lines_.back();
            size_t new_size = current_size_ - last_line.length() + line_part.length();
            
            if (new_size <= max_size_) {
                last_line.append(line_part);
                current_size_ = new_size;
                if (line_added_cb_) line_added_cb_(last_line);
            } else {
                // Не помещается - создаем новую строку
                add_line(std::string(line_part));
            }
        } else {
            // Нет строк - создаем новую
            add_line(std::string(line_part));
        }
        
        // После \n всегда начинаем новую строку (даже если пустая)
        add_line("");
        
        // Переходим к остатку текста после \n
        remaining = remaining.substr(newline_pos + 1);
    }
}

void IBuffer::printf(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    print(buffer);
}

void IBuffer::printf_color(const std::string& color, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::string colored_text = format_color(color) + buffer + "{c}";
    print(colored_text);
}

void IBuffer::println(const std::string& text) {
    print(text);
    
    // Всегда добавляем пустую строку как разделитель после текста
    // (даже если текст уже содержит \n в конце)
    add_line("");
}

void IBuffer::println(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    println(std::string(buffer));
}

void IBuffer::println_color(const std::string& color, const std::string& text) {
    std::string colored_text = format_color(color) + text + "{c}";
    println(colored_text);
}

void IBuffer::println_color(const std::string& color, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    println_color(color, std::string(buffer));
}

void IBuffer::clear() {
    lines_.clear();
    current_size_ = 0;
    scroll_pos_ = 0;
}

void IBuffer::trim_to_size(size_t new_max_size) {
    max_size_ = new_max_size;
    check_and_trim();
    ensure_scroll_validity();
}

std::string IBuffer::get_lines(int start_line, int num_lines) const {
    if (lines_.empty() || start_line < 0 || num_lines <= 0) {
        return "";
    }
    
    size_t start = static_cast<size_t>(std::max(0, start_line));
    size_t end = std::min(start + static_cast<size_t>(num_lines), lines_.size());
    
    if (start >= end) return "";
    
    std::string result;
    for (size_t i = start; i < end; ++i) {
        result += lines_[i];
        if (i != end - 1) {
            result += '\n';
        }
    }
    
    return result;
}

std::string IBuffer::get_last_lines(int num_lines) const {
    if (lines_.empty() || num_lines <= 0) {
        return "";
    }
    
    int start_line = static_cast<int>(lines_.size()) - num_lines;
    if (start_line < 0) start_line = 0;
    
    return get_lines(start_line, num_lines);
}

std::string IBuffer::get_line(int line_num) const {
    if (line_num < 0 || static_cast<size_t>(line_num) >= lines_.size()) {
        return "";
    }
    return lines_[line_num];
}

std::string IBuffer::get_all() const {
    return get_lines(0, static_cast<int>(lines_.size()));
}

std::string IBuffer::get_visible_lines_text() const {
    return get_lines(get_display_start_line(), visible_lines_);
}

void IBuffer::set_line_added_callback(LineAddedCallback callback) {
    line_added_cb_ = callback;
}

void IBuffer::set_overflow_callback(BufferOverflowCallback callback) {
    overflow_cb_ = callback;
}

int IBuffer::find(const std::string& text, int start_line) const {
    if (start_line < 0) start_line = 0;
    
    for (size_t i = static_cast<size_t>(start_line); i < lines_.size(); ++i) {
        if (lines_[i].find(text) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool IBuffer::contains(const std::string& text) const {
    return find(text) != -1;
}

IBuffer::Stats IBuffer::get_stats() const {
    Stats stats;
    stats.total_lines = lines_.size();
    stats.total_bytes = current_size_;
    stats.max_lines = max_size_ / 50; // Примерная оценка
    
    if (!lines_.empty()) {
        stats.avg_line_length = current_size_ / lines_.size();
    } else {
        stats.avg_line_length = 0;
    }
    
    return stats;
}

// ============ СКРОЛЛИНГ ============

void IBuffer::set_scroll_pos(int pos) {
    if (pos < 0) {
        scroll_pos_ = 0;
    } else if (pos >= static_cast<int>(lines_.size())) {
        scroll_pos_ = std::max(0, static_cast<int>(lines_.size()) - 1);
    } else {
        scroll_pos_ = pos;
    }
    ensure_scroll_validity();
}

void IBuffer::scroll_to_end() {
    if (lines_.empty()) {
        scroll_pos_ = 0;
        //std::cout << "scroll_to_end: empty buffer, scroll_pos_=0" << std::endl;
    } else {
        // Показываем последние visible_lines_ строк
        int total = static_cast<int>(lines_.size());
        int new_scroll_pos = std::max(0, total - visible_lines_);
        scroll_pos_ = new_scroll_pos;
        //std::cout << "scroll_to_end: total=" << total 
        //          << " visible_lines_=" << visible_lines_
        //          << " new scroll_pos_=" << scroll_pos_ << std::endl;
    }
}

void IBuffer::scroll_to_top() {
    scroll_pos_ = 0;
}

void IBuffer::scroll_up(int lines) {
    scroll_pos_ = std::max(0, scroll_pos_ - lines);
    ensure_scroll_validity();
}

void IBuffer::scroll_down(int lines) {
    scroll_pos_ = std::min(
        std::max(0, static_cast<int>(lines_.size()) - visible_lines_),
        scroll_pos_ + lines
    );
}

void IBuffer::page_up() {
    scroll_up(visible_lines_);
}

void IBuffer::page_down() {
    scroll_down(visible_lines_);
}

bool IBuffer::is_at_end() const {
    if (lines_.empty()) return true;
    
    int total = static_cast<int>(lines_.size());
    
    // Если меньше строк чем visible_lines_, мы автоматически показываем все
    // и это считается "концом"
    if (total <= visible_lines_) {
        return true;
    }
    
    // Иначе проверяем, показываем ли мы последние visible_lines_ строк
    return scroll_pos_ >= (total - visible_lines_);
}

bool IBuffer::is_last_line_visible() const {
    if (lines_.empty()) return true;
    int last_line_idx = static_cast<int>(lines_.size()) - 1;
    int display_start = get_display_start_line();
    return last_line_idx >= display_start && 
           last_line_idx < display_start + visible_lines_;
}

int IBuffer::get_display_start_line() const {
    if (lines_.empty()) return 0;
    
    int total = static_cast<int>(lines_.size());
    
    // Если меньше строк чем visible_lines_, показываем с начала
    if (total <= visible_lines_) {
        return 0;  // Показываем все строки с начала
    }
    
    // Если в режиме автопрокрутки
    if (is_at_end()) {
        // Показываем последние visible_lines_ строк
        return total - visible_lines_;
    }
    
    // Иначе используем текущую позицию скролла
    int max_start = total - visible_lines_;
    return std::min(scroll_pos_, max_start);
}

// ============ PRIVATE METHODS ============

void IBuffer::add_line(const std::string& line) {
    size_t line_size = line.length() + 1;
    
    bool was_at_end = is_at_end();
    if (line.size() > 0)
        std::cout << "IBuffer::add_line(\"" << line << "\")" << std::endl;
    
    // Проверяем, поместится ли новая строка
    while (!lines_.empty() && current_size_ + line_size > max_size_) {
        check_and_trim();
    }
    
    lines_.push_back(line);
    current_size_ += line_size;
    
    if (line_added_cb_) {
        line_added_cb_(line);
    }
    
    // ВАЖНО: Автоскролл к новой строке, если мы были в конце
    if (was_at_end) {
        scroll_to_end();  // ЭТО ДОЛЖНО БЫТЬ ВЫЗВАНО!
    }
    
    ensure_scroll_validity();
}

void IBuffer::check_and_trim() {
    while (!lines_.empty() && current_size_ > max_size_) {
        size_t removed_size = lines_.front().length() + 1;
        lines_.erase(lines_.begin());
        current_size_ -= removed_size;
        
        // Корректируем позицию скролла при удалении строк
        if (scroll_pos_ > 0) {
            scroll_pos_ = std::max(0, scroll_pos_ - 1);
        }
        
        if (overflow_cb_) {
            overflow_cb_(removed_size);
        }
    }
}

void IBuffer::ensure_scroll_validity() {
    if (lines_.empty()) {
        scroll_pos_ = 0;
        return;
    }
    
    // Ограничиваем scroll_pos_ в допустимых пределах
    int max_scroll = std::max(0, static_cast<int>(lines_.size()) - visible_lines_);
    scroll_pos_ = std::max(0, std::min(scroll_pos_, max_scroll));
}

std::string IBuffer::format_color(const std::string& color) const {
    if (color.empty()) return "";
    
    // Если цвет уже в формате {c:RRGGBB}
    if (color.length() >= 10 && color[0] == '{' && color[1] == 'c' && color[2] == ':') {
        return color;
    }
    
    // Если цвет в формате RRGGBB (6 символов)
    if (color.length() == 6) {
        return "{c:" + color + "}";
    }
    
    // Или просто возвращаем как есть
    return color;
}
