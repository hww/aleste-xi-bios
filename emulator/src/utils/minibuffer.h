#ifndef MINIBUFFER_HPP
#define MINIBUFFER_HPP

#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include <vector>

class Minibuffer {
public:
    // Конструкторы
    Minibuffer();
    explicit Minibuffer(const std::string& prompt);
    
    // ============ ОСНОВНОЙ API ============
    
    // Показать мини-буфер
    void show(const std::string& prompt = "", const std::string& initial_text = "");
    void show_with_prompt(const std::string& initial_text = "");
    
    // Скрыть мини-буфер
    void hide();
    
    // Очистить содержимое
    void clear();
    
    // Обработка SDL событий
    bool handle_event(const SDL_Event& event);
    
    // ============ СОСТОЯНИЕ ============
    
    // Проверить видимость
    bool is_visible() const { return visible_; }
    
    // Получить текст
    const std::string& get_text() const { return buffer_; }
    
    // Получить промпт
    const std::string& get_prompt() const { return prompt_; }
    
    // Получить позицию курсора
    int get_cursor_pos() const { return cursor_pos_; }
    
    // Получить длину текста
    size_t get_length() const { return buffer_.length(); }
    
    // Получить режим (insert/overwrite)
    bool is_overwrite_mode() const { return overwrite_mode_; }
    
    // ============ НАСТРОЙКИ ============
    
    // Установить промпт
    void set_prompt(const std::string& prompt);
    
    // Установить максимальный размер буфера
    void set_max_size(size_t max_size) { max_size_ = max_size; }
    
    // Переключить режим вставки/замены
    void toggle_overwrite_mode() { overwrite_mode_ = !overwrite_mode_; }
    
    // ============ КОЛЛБЭКИ ============
    
    // Типы коллбэков
    using CommandCallback = std::function<void(const std::string&)>;
    using VisibilityCallback = std::function<void(bool)>;
    using TextChangedCallback = std::function<void(const std::string&)>;
    
    // Установить коллбэки
    void set_command_callback(CommandCallback callback) { command_callback_ = callback; }
    void set_visibility_callback(VisibilityCallback callback) { visibility_callback_ = callback; }
    void set_text_changed_callback(TextChangedCallback callback) { text_changed_callback_ = callback; }
    
    // ============ КОМАНДЫ РЕДАКТИРОВАНИЯ ============
    
    // Вставка текста
    void insert_text(const std::string& text);
    void insert_char(char ch);
    
    // Удаление
    void backspace();
    void delete_char();
    void delete_to_end();
    void delete_to_start();
    
    // Навигация
    void move_cursor_left();
    void move_cursor_right();
    void move_cursor_home();
    void move_cursor_end();
    
    // История команд
    void add_to_history(const std::string& command);
    const std::string& get_history_prev();
    const std::string& get_history_next();
    void clear_history();
    size_t get_history_size() const { return history_.size(); }
    
    // ============ УТИЛИТЫ ============
    
    // Получить текст с курсором для отображения
    std::string get_display_text() const;
    
    // Получить информацию о режиме
    std::string get_mode_string() const;
    
    // Проверить, пустой ли буфер
    bool is_empty() const { return buffer_.empty(); }
    
    // Получить статистику
    struct Stats {
        size_t total_chars_typed;
        size_t commands_executed;
        size_t history_size;
        bool overwrite_mode;
    };
    Stats get_stats() const;
    
private:
    // Основные данные
    std::string buffer_;
    std::string default_prompt_;
    std::string prompt_;
    
    // Состояние
    int cursor_pos_;
    bool overwrite_mode_;
    bool visible_;
    size_t max_size_;
    
    // История команд
    std::vector<std::string> history_;
    int history_index_;
    
    // Коллбэки
    CommandCallback command_callback_;
    VisibilityCallback visibility_callback_;
    TextChangedCallback text_changed_callback_;
    
    // Статистика
    size_t total_chars_typed_;
    size_t commands_executed_;
    
    // Вспомогательные методы
    void handle_key_down(const SDL_KeyboardEvent& key);
    void handle_text_input(const SDL_TextInputEvent& text);
    void handle_alt_key(int key);
    void notify_text_changed();
    void notify_visibility_changed();
    void ensure_cursor_in_bounds();
    
    // Проверка модификаторов клавиш
    static bool is_alt_pressed();
    static bool is_ctrl_pressed();
    static bool is_shift_pressed();
};

#endif // MINIBUFFER_HPP