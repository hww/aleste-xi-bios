#ifndef I_BUFFER_H
#define I_BUFFER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

class IBuffer {
public:
    // Конструктор с максимальным размером в байтах и количеством видимых строк
    explicit IBuffer(size_t max_line = 1024, int visible_lines = 20);
    
    // Деструктор
    ~IBuffer() = default;
    
    // ============ ОСНОВНОЙ API ============
    
    // Печать без форматирования
    void print(const std::string& text);
    
    // Печать с форматированием (printf-style)
    void printf(const char* format, ...);
    
    // Печать с цветом (формат: RRGGBB или {c:RRGGBB})
    void printf_color(const std::string& color, const char* format, ...);
    
    // Печать с переводом строки
    void println(const std::string& text);
    void println(const char* format, ...);
    
    // Печать с цветом и переводом строки
    void println_color(const std::string& color, const std::string& text);
    void println_color(const std::string& color, const char* format, ...);
    
    // ============ УПРАВЛЕНИЕ БУФЕРОМ ============
    
    // Очистить буфер полностью
    void clear();
    
    // Ограничить размер буфера (обрезать старые данные)
    void trim_to_lines_num(size_t new_max_lines);
    
    // Получить максимальный размер
    size_t get_max_lines() const { return max_lines_; }
    
    // Получить/установить количество видимых строк
    int get_visible_lines_count() const { return visible_lines_; }
    void set_visible_lines(int lines) { 
        visible_lines_ = std::max(1, lines); 
        ensure_scroll_validity();
    }
    
    // ============ РАБОТА СО СТРОКАМИ ============
    
    // Получить количество строк
    size_t get_lines_num() const { return lines_.size(); }
    
    // Получить диапазон строк (от start_line, num_lines строк)
    std::string get_lines(int start_line, int num_lines) const;
    
    // Получить последние N строк
    std::string get_last_lines(int num_lines) const;
    
    // Получить конкретную строку
    std::string get_line(int line_num) const;
    
    // Получить весь буфер как строку
    std::string get_all() const;
    
    // Получить видимые строки как текст (с учетом скролла и visible_lines_)
    std::string get_visible_lines_text() const;
    
    // ============ ИТЕРАТОРЫ ============
    
    // Константные итераторы для range-based for
    std::vector<std::string>::const_iterator begin() const { return lines_.begin(); }
    std::vector<std::string>::const_iterator end() const { return lines_.end(); }
    
    // ============ КОЛЛБЭКИ ============
    
    // Установить коллбэк на добавление строки
    using LineAddedCallback = std::function<void(const std::string&)>;
    void set_line_added_callback(LineAddedCallback callback);
    
    // Установить коллбэк на переполнение буфера
    using BufferOverflowCallback = std::function<void(size_t trimmed_size)>;
    void set_overflow_callback(BufferOverflowCallback callback);
    
    // ============ УТИЛИТЫ ============
    
    // Поиск текста в буфере (возвращает номер строки или -1)
    int find(const std::string& text, int start_line = 0) const;
    
    // Проверить, содержит ли буфер текст
    bool contains(const std::string& text) const;
    
    // Получить статистику
    struct Stats {
        size_t total_lines;
        size_t max_lines;
    };
    Stats get_stats() const;
    
    // ============ СКРОЛЛИНГ ============
    
    // Получить текущую позицию скролла
    int get_scroll_pos() const { return scroll_pos_; }
    
    // Установить позицию скролла
    void set_scroll_pos(int pos);
    
    // Скролл в конец (последние строки)
    void scroll_to_end();
    
    // Скролл в начало
    void scroll_to_top();
    
    // Скролл на N строк вверх/вниз
    void scroll_up(int lines = 1);
    void scroll_down(int lines = 1);
    
    // Скролл на одну страницу
    void page_up();
    void page_down();
    
    // Проверить, находится ли скролл в конце
    bool is_at_end() const;
    
    // Проверить, видна ли последняя строка при текущих настройках
    bool is_last_line_visible() const;
    
    // Получить индекс первой отображаемой строки (с учетом видимых строк)
    int get_display_start_line() const;
    
private:
    void add_line(const std::string& line);
    void check_and_trim();
    void ensure_scroll_validity();
    std::string format_color(const std::string& color) const;

    std::vector<std::string> lines_;
    size_t max_lines_;
    int visible_lines_;    // Количество строк, которые показываются
    LineAddedCallback line_added_cb_;
    BufferOverflowCallback overflow_cb_;

    // Скроллинг
    int scroll_pos_;      // Текущая позиция скролла (индекс первой видимой строки)
};

// Утилитарные функции
namespace BufferUtils {
    // Создать буфер с автоопределением размера
    std::unique_ptr<IBuffer> create_screen_buffer();
    std::unique_ptr<IBuffer> create_debug_buffer();
    std::unique_ptr<IBuffer> create_log_buffer();
    
    // Быстрое создание буфера с настройками
    std::unique_ptr<IBuffer> create_buffer(size_t max_size, 
                                          int visible_lines = 20,
                                          IBuffer::LineAddedCallback line_cb = nullptr,
                                          IBuffer::BufferOverflowCallback overflow_cb = nullptr);
}

#endif // I_BUFFER_H