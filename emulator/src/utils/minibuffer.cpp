#include "minibuffer.h"
#include <algorithm>
#include <cctype>
#include <iostream>

Minibuffer::Minibuffer() 
    : default_prompt_(">> ")
    , prompt_(default_prompt_)
    , cursor_pos_(0)
    , overwrite_mode_(false)
    , visible_(false)
    , max_size_(256)
    , history_index_(-1)
    , total_chars_typed_(0)
    , commands_executed_(0) {}

Minibuffer::Minibuffer(const std::string& prompt)
    : default_prompt_(prompt.empty() ? ">> " : prompt)
    , prompt_(default_prompt_)
    , cursor_pos_(0)
    , overwrite_mode_(false)
    , visible_(false)
    , max_size_(256)
    , history_index_(-1)
    , total_chars_typed_(0)
    , commands_executed_(0) {}

void Minibuffer::show(const std::string& prompt, const std::string& initial_text) {
    if (!prompt.empty()) {
        prompt_ = prompt;
    } else {
        prompt_ = default_prompt_;
    }
    
    visible_ = true;
    cursor_pos_ = 0;
    buffer_.clear();
    
    if (!initial_text.empty()) {
        buffer_ = initial_text;
        cursor_pos_ = static_cast<int>(buffer_.length());
    }
    
    history_index_ = -1;
    SDL_StartTextInput();
    notify_visibility_changed();
}

void Minibuffer::show_with_prompt(const std::string& initial_text) {
    show(default_prompt_, initial_text);
}

void Minibuffer::hide() {
    visible_ = false;
    SDL_StopTextInput();
    notify_visibility_changed();
}

void Minibuffer::clear() {
    buffer_.clear();
    cursor_pos_ = 0;
    history_index_ = -1;
    notify_text_changed();
}

bool Minibuffer::handle_event(const SDL_Event& event) {
    if (!visible_) return false;
    
    bool command_complete = false;
    
    switch (event.type) {
        case SDL_KEYDOWN:
            handle_key_down(event.key);
            if (event.key.keysym.sym == SDLK_RETURN || 
                event.key.keysym.sym == SDLK_KP_ENTER) {
                command_complete = true;
            }
            break;
            
        case SDL_TEXTINPUT:
            handle_text_input(event.text);
            break;
    }
    
    if (command_complete && !buffer_.empty()) {
        // Вызываем коллбэк команды
        if (command_callback_) {
            command_callback_(buffer_);
        }
        
        // Добавляем в историю
        add_to_history(buffer_);
        commands_executed_++;
        

    }
    
    return command_complete;
}

void Minibuffer::set_prompt(const std::string& prompt) {
    prompt_ = prompt;
    default_prompt_ = prompt;
}

void Minibuffer::insert_text(const std::string& text) {
    if (text.empty()) return;
    
    size_t buffer_len = buffer_.length();
    
    if (overwrite_mode_ && cursor_pos_ < static_cast<int>(buffer_len)) {
        // Замена символов
        size_t replace_len = std::min(text.length(), buffer_len - cursor_pos_);
        buffer_.replace(cursor_pos_, replace_len, text.substr(0, replace_len));
        
        if (text.length() > replace_len) {
            // Вставка оставшейся части
            buffer_.insert(cursor_pos_ + static_cast<int>(replace_len), text.substr(replace_len));
        }
    } else {
        // Вставка
        buffer_.insert(cursor_pos_, text);
    }
    
    cursor_pos_ += static_cast<int>(text.length());
    total_chars_typed_ += text.length();
    
    if (buffer_.length() > max_size_) {
        buffer_.resize(max_size_);
        ensure_cursor_in_bounds();
    }
    
    notify_text_changed();
}

void Minibuffer::insert_char(char ch) {
    insert_text(std::string(1, ch));
}

void Minibuffer::backspace() {
    if (cursor_pos_ > 0 && !buffer_.empty()) {
        buffer_.erase(cursor_pos_ - 1, 1);
        cursor_pos_--;
        notify_text_changed();
    }
}

void Minibuffer::delete_char() {
    size_t buffer_len = buffer_.length();
    if (cursor_pos_ < static_cast<int>(buffer_len)) {
        buffer_.erase(cursor_pos_, 1);
        notify_text_changed();
    }
}

void Minibuffer::delete_to_end() {
    size_t buffer_len = buffer_.length();
    if (cursor_pos_ < static_cast<int>(buffer_len)) {
        buffer_.erase(cursor_pos_);
        notify_text_changed();
    }
}

void Minibuffer::delete_to_start() {
    if (cursor_pos_ > 0) {
        buffer_.erase(0, cursor_pos_);
        cursor_pos_ = 0;
        notify_text_changed();
    }
}

void Minibuffer::move_cursor_left() {
    if (cursor_pos_ > 0) {
        cursor_pos_--;
    }
}

void Minibuffer::move_cursor_right() {
    size_t buffer_len = buffer_.length();
    if (cursor_pos_ < static_cast<int>(buffer_len)) {
        cursor_pos_++;
    }
}

void Minibuffer::move_cursor_home() {
    cursor_pos_ = 0;
}

void Minibuffer::move_cursor_end() {
    cursor_pos_ = static_cast<int>(buffer_.length());
}

void Minibuffer::add_to_history(const std::string& command) {
    if (command.empty()) return;
    
    // Не добавляем дубликаты подряд
    if (history_.empty() || history_.back() != command) {
        history_.push_back(command);
        if (history_.size() > 100) { // Ограничиваем историю
            history_.erase(history_.begin());
        }
    }
    history_index_ = -1;
}

const std::string& Minibuffer::get_history_prev() {
    if (history_.empty()) {
        static std::string empty;
        return empty;
    }
    
    if (history_index_ < 0) {
        // Начинаем с последней команды
        history_index_ = static_cast<int>(history_.size()) - 1;
    } else if (history_index_ > 0) {
        history_index_--;
    }
    
    if (history_index_ >= 0 && history_index_ < static_cast<int>(history_.size())) {
        buffer_ = history_[history_index_];
        cursor_pos_ = static_cast<int>(buffer_.length());
        notify_text_changed();
        return history_[history_index_];
    }
    
    static std::string empty;
    return empty;
}

const std::string& Minibuffer::get_history_next() {
    if (history_.empty() || history_index_ < 0) {
        static std::string empty;
        return empty;
    }
    
    if (history_index_ < static_cast<int>(history_.size()) - 1) {
        history_index_++;
        buffer_ = history_[history_index_];
    } else {
        history_index_ = -1;
        buffer_.clear();
    }
    
    cursor_pos_ = static_cast<int>(buffer_.length());
    notify_text_changed();
    
    if (history_index_ >= 0) {
        return history_[history_index_];
    }
    
    static std::string empty;
    return empty;
}

void Minibuffer::clear_history() {
    history_.clear();
    history_index_ = -1;
}

std::string Minibuffer::get_display_text() const {
    std::string result = prompt_;
    size_t buffer_len = buffer_.length();
    
    if (cursor_pos_ < static_cast<int>(buffer_len)) {
        // Текст до курсора + курсор + текст после курсора
        result += buffer_.substr(0, cursor_pos_) + "|" + 
                  buffer_.substr(cursor_pos_);
    } else {
        // Курсор в конце
        result += buffer_ + "|";
    }
    
    return result;
}

std::string Minibuffer::get_mode_string() const {
    return overwrite_mode_ ? "[OVR]" : "[INS]";
}

Minibuffer::Stats Minibuffer::get_stats() const {
    Stats stats;
    stats.total_chars_typed = total_chars_typed_;
    stats.commands_executed = commands_executed_;
    stats.history_size = history_.size();
    stats.overwrite_mode = overwrite_mode_;
    return stats;
}

// ============ PRIVATE METHODS ============

void Minibuffer::handle_key_down(const SDL_KeyboardEvent& key) {
    switch (key.keysym.sym) {
        case SDLK_ESCAPE:
            hide();
            clear();
            break;
            
        case SDLK_BACKSPACE:
            backspace();
            break;
            
        case SDLK_DELETE:
            delete_char();
            break;
            
        case SDLK_LEFT:
            move_cursor_left();
            break;
            
        case SDLK_RIGHT:
            move_cursor_right();
            break;
            
        case SDLK_HOME:
            move_cursor_home();
            break;
            
        case SDLK_END:
            move_cursor_end();
            break;
            
        case SDLK_INSERT:
            toggle_overwrite_mode();
            break;
            
        case SDLK_UP:
            get_history_prev();
            break;
            
        case SDLK_DOWN:
            get_history_next();
            break;
            
        default:
            if (is_alt_pressed()) {
                handle_alt_key(key.keysym.sym);
            }
            break;
    }
}

void Minibuffer::handle_text_input(const SDL_TextInputEvent& text) {
    // Проверяем, не превысит ли вставка максимальный размер
    if (buffer_.length() + strlen(text.text) > max_size_) {
        // Рассчитываем, сколько символов можно вставить
        size_t available = max_size_ - buffer_.length();
        if (available > 0) {
            // Вставляем только то, что помещается
            std::string truncated(text.text, available);
            insert_text(truncated);
        }
    } else {
        insert_text(text.text);
    }
}

void Minibuffer::handle_alt_key(int key) {
    switch (key) {
        case SDLK_a: // Alt+A - начало строки
            move_cursor_home();
            break;
            
        case SDLK_e: // Alt+E - конец строки
            move_cursor_end();
            break;
            
        case SDLK_d: // Alt+D - удалить символ под курсором
            delete_char();
            break;
            
        case SDLK_k: // Alt+K - удалить до конца строки
            delete_to_end();
            break;
            
        case SDLK_u: // Alt+U - удалить от начала до курсора
            delete_to_start();
            break;
            
        case SDLK_x: // Alt+X - закрыть мини-буфер
            hide();
            clear();
            break;
    }
}

void Minibuffer::notify_text_changed() {
    if (text_changed_callback_) {
        text_changed_callback_(buffer_);
    }
}

void Minibuffer::notify_visibility_changed() {
    if (visibility_callback_) {
        visibility_callback_(visible_);
    }
}

void Minibuffer::ensure_cursor_in_bounds() {
    size_t buffer_len = buffer_.length();
    if (cursor_pos_ > static_cast<int>(buffer_len)) {
        cursor_pos_ = static_cast<int>(buffer_len);
    }
    if (cursor_pos_ < 0) {
        cursor_pos_ = 0;
    }
}

bool Minibuffer::is_alt_pressed() {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    return (state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT]);
}

bool Minibuffer::is_ctrl_pressed() {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    return (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]);
}

bool Minibuffer::is_shift_pressed() {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    return (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]);
}