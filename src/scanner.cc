/**
 * organ *.ogn
 * clavecin
 * organica
 * org mode
 * norg2
 * orc
 * muninn *.mnn *.min
 * lamp *.lmp
 * remark
 * context
 * memo
 *
 * orgdown -> odin
 * orgnote -> ono
 * U+2625 - Ankh
 *
 * corgi
 * orgnote
 *
 * cyborg
 * morgen
 * treeorg
 * mistral
 * piramida
 *
 * forge - кузница
 *
 * Obelisk
 * Crystal
 * Skald --- The poetry of organization!
 */
#include <functional>
#include <vector>
#include <cstring>
#include <cwctype>
#include <iostream>
#include <string>
#include <stack>
#include <unordered_map>
#include "tree_sitter/parser.h"

#define DEBUG 1

/**
 * Print the upcoming token after parsing finished.
 * Note: May change parser behaviour.
 */
#define DEBUG_CURRENT_CHAR 1

using namespace std;

enum TokenType : unsigned char {
    BOLD,
    ITALIC,
    STRIKETHROUGH,
    UNDERLINE,
    VERBATIM,
    INLINE_MATH,

    BOLD_CLOSE,
    ITALIC_CLOSE,
    STRIKETHROUGH_CLOSE,
    UNDERLINE_CLOSE,
    VERBATIM_CLOSE,
    INLINE_MATH_CLOSE,

    HEADING_1,
    HEADING_2,
    HEADING_3,
    HEADING_4,
    HEADING_5,
    HEADING_6,

    DEFINITION_BEGIN,
    DEFINITION_SEP,
    DEFINITION_END,

    LIST_1,
    LIST_2,
    LIST_3,
    LIST_4,
    LIST_5,
    LIST_6,
    LIST_7,
    LIST_8,
    LIST_9,
    LIST_10,

    ORDERED_LIST_LABEL,

    CHECKBOX_OPEN,
    CHECKBOX_CLOSE,

    CHECKBOX_UNDONE,
    CHECKBOX_DONE,
    CHECKBOX_PENDING,
    CHECKBOX_URGENT,

    CODE_BEGIN,
    TAG_BEGIN,
    TAG_END,
    HASHTAG,
    TAG_PARAMETER,

    BLANK_LINE,
    SOFT_BREAK,
    HARD_BREAK,

    WORD,
    RAW_WORD,

    NONE,
};

#ifdef DEBUG
vector<string> tokens_names = {
    "bold",
    "italic",
    "strikethrough",
    "underline",
    "verbatim",
    "inline_math",

    "bold_close",
    "italic_close",
    "strikethrough_close",
    "underline_close",
    "verbatim_close",
    "inline_math_close",

    "heading_1",
    "heading_2",
    "heading_3",
    "heading_4",
    "heading_5",
    "heading_6",

    "definition_begin",
    "definition_sep",
    "definition_end",

    "list_1_prefix",
    "list_2_prefix",
    "list_3_prefix",
    "list_4_prefix",
    "list_5_prefix",
    "list_6_prefix",
    "list_7_prefix",
    "list_8_prefix",
    "list_9_prefix",
    "list_10_prefix",

    "ordered_list_label",

    "checkbox_open",
    "checkbox_close",

    "checkbox_undone",
    "checkbox_done",
    "checkbox_pending",
    "checkbox_urgent",

    "code_begin",
    "tag_begin",
    "tag_end",
    "hashtag",
    "tag_parameter",

    "blank_line",
    "soft_break",
    "hard_break",

    "word",
    "raw_word",

    "none",
};
#endif // DEBUG

const unordered_map<char, TokenType> markup_tokens = {
    {'*', BOLD},
    {'/', ITALIC},
    {'~', STRIKETHROUGH},
    {'_', UNDERLINE},
    {'`', VERBATIM},
    {'$', INLINE_MATH},
};

constexpr uint8_t MARKUP = 6;      //< Total number of markup tokens.
constexpr uint8_t MAX_HEADING = 6; //< Maximum heading level.
constexpr uint8_t MAX_LIST = 10;   //< Maximum list level.

/**
 * The scanner analyzes the current character --- the last character the
 * "advanced" function passed.
 *
 * a [b] c
 * │  │  └─ lexer->lookahead
 * │  └─ current
 * └─ previous
 */
struct Scanner
{
    TSLexer* lexer;

    const bool* valid_tokens;

    int32_t
    previous = 0, //< Previous char
    current = 0;  //< Current char

    uint16_t parsed_chars = 0; //< Number of parsed chars

    deque<char> markup_stack;

    bool tag_parameter_is_valid = true;

    bool scan () {
#ifdef DEBUG
        clog << "{" << endl;
#endif
        debug_valid_tokens();

        if (get_column() == 0) {
            if (parse_newline()) return true;
        }

        if (parsed_chars == 0) {
            if (parse_ordered_list()) return true;
            skip_spaces();
        }

        if (parsed_chars == 0 && is_newline(lexer->lookahead)) {
            skip_newline(); // skip newline char
            if (parse_newline()) return true;
        }

        if (parsed_chars == 0)
            advance();

        if (parse_tag_parameter()) return true;
        if (parse_code_block()) return true;

        if (parse_check_box()) return true;
        if (parse_definition()) return true;
        if (parse_open_markup()) return true;
        if (parse_close_markup()) return true;

        if (parse_word()) return true;

#ifdef DEBUG
        clog << "  false" << endl << "}" << endl;
#endif

        if (is_eof()) {
#ifdef DEBUG
            clog << "End of the file!" << endl << endl;
#endif
            return false;
        }

        return false;
    };

    // Advances the lexer forward. The char that was advanced will be returned
    // in the final result.
    void advance() {
        previous = current;
        current = lexer->lookahead;
        lexer->advance(lexer, false);
        ++parsed_chars;

#ifdef DEBUG_CURRENT_CHAR
        clog << "  -> " << current << ' ';
        switch (current) {
        case 13:
            clog << "\\r";
            break;
        case 10:
            clog << "\\n";
            break;
        case 0:
            clog << "\\0";
            break;
        default:
            clog << static_cast<char>(current);
        }
        clog << endl;
#endif
    }

    /// Skips the next character without including it in the final result.
    void skip() {
        previous = current;
        current = lexer->lookahead;
        lexer->advance(lexer, true);
        parsed_chars = 0;
    }

    inline void skip_spaces() {
        if (valid_tokens[CHECKBOX_UNDONE]) return;
        while (is_space(lexer->lookahead)) skip();
    }

    /// Skip newline char
    inline void skip_newline() {
        if (lexer->lookahead == 13) skip(); // \r
        if (lexer->lookahead == 10) skip(); // \n
    }

    /**
     * @brief Rules that decide based on the first token on the next line.
     * @param valid_tokens Valid symbols.
     * @return Whether to finish parsing.
     */
    bool parse_newline() {
        skip_spaces();

        // TAG_PARAMETER token, valid only on the same line as a range tag definition.
        // That's why if we are on the new line, then TAG_PARAMETER stops to be valid.
        tag_parameter_is_valid = false;

        // Check if current line is empty line.
        if (is_newline(lexer->lookahead)) {
            advance();
            lexer->result_symbol = BLANK_LINE;
            debug_result();
            return true;
        }

        advance();

        uint8_t n = 0; //< Number of parsed chars
        switch (current) {
        case '*': { // HEADING
            constexpr auto expected = '*';
            if (lexer->lookahead == expected || is_space(lexer->lookahead)) {
                while (lexer->lookahead == expected) {
                    advance();
                    ++n;
                }
                skip_spaces();
                if (is_newline(lexer->lookahead))
                    return false;
                lexer->result_symbol =
                    static_cast<TokenType>(HEADING_1 + (n < MAX_HEADING ? n : MAX_HEADING - 1));
                debug_result();
                return true;
            }
            else if (parse_open_markup()) return true;
            break;
        }
        case ':': { // DEFINITION
            if (valid_tokens[DEFINITION_BEGIN] && is_space_or_newline(lexer->lookahead)) {
                lexer->result_symbol = DEFINITION_BEGIN;
                debug_result();
                return true;
            }
            break;
        }
        case '-': { // LIST, SOFT_BREAK
            constexpr auto expected = '-';
            while (lexer->lookahead == expected) {
                advance();
                ++n;
            }

            if (1 + n == 3 && (!lexer->lookahead || is_newline(lexer->lookahead))) {
                lexer->result_symbol = SOFT_BREAK;
                debug_result();
                return true;
            }

            if (iswdigit(lexer->lookahead)) {
                lexer->mark_end(lexer);
                while (iswdigit(lexer->lookahead))
                    advance();
            }

            if (is_space(lexer->lookahead)) {
                lexer->result_symbol =
                    static_cast<TokenType>(LIST_1 + (n < MAX_LIST ? n : MAX_LIST - 1));
                debug_result();
                return true;
            }

            break;
        }
        case '=': { // HARD_BREAK
            constexpr auto expected = '=';
            while (lexer->lookahead == expected) {
                advance();
                ++n;
            }
            if (1 + n == 3 && (!lexer->lookahead || is_newline(lexer->lookahead))) {
                lexer->result_symbol = HARD_BREAK;
                debug_result();
                return true;
            }
            break;
        }
        case '@': {
            if (valid_tokens[TAG_END]) {
                if (token("end")
                    && (!lexer->lookahead || is_newline(lexer->lookahead)))
                {
                    lexer->result_symbol = TAG_END;
                    debug_result();
                    return true;
                }
                else
                    return parse_raw_word();
            }
            else if (token("code") && iswspace(lexer->lookahead))
                lexer->result_symbol = CODE_BEGIN;
            else {
                while (!iswspace(lexer->lookahead))
                    advance();
                lexer->result_symbol = TAG_BEGIN;
            }
            debug_result();
            return true;
        }
        case '#': {
            while (!iswspace(lexer->lookahead))
                advance();
            lexer->result_symbol = HASHTAG;
            debug_result();
            return true;
        }
        }

        if (parse_code_block()) return true;

        return false;
    }

    bool parse_tag_parameter() {
        if (valid_tokens[TAG_PARAMETER] && tag_parameter_is_valid) {
            // while (lexer->lookahead && !iswspace(lexer->lookahead))
            while (!iswspace(lexer->lookahead))
                advance();
            lexer->result_symbol = TAG_PARAMETER;
            debug_result();
            return true;
        }
        return false;
    }

    bool parse_code_block() {
        if (valid_tokens[RAW_WORD] && valid_tokens[TAG_END])
            return parse_raw_word();
        return false;
    }

    bool parse_definition() {
        if (parsed_chars != 1) return false;

        if (valid_tokens[DEFINITION_SEP]
            && current == ':' && is_space_or_newline(lexer->lookahead))
        {
            lexer->result_symbol = DEFINITION_SEP;
            debug_result();
            return true;
        }
        else if (valid_tokens[DEFINITION_END]
            && current == ':' && is_newline(lexer->lookahead))
        {
            lexer->result_symbol = DEFINITION_END;
            debug_result();
            return true;
        }
        return false;
    }

    bool parse_ordered_list() {
        if (valid_tokens[ORDERED_LIST_LABEL]
            && !is_space_or_newline(lexer->lookahead))
        {
            while (iswdigit(lexer->lookahead))
                advance();
            lexer->result_symbol = ORDERED_LIST_LABEL;
            debug_result();
            return true;
        }
        return false;
    }

    bool parse_open_markup() {
        if (parsed_chars != 1) return false;

        /// Markup token
        auto mt = markup_tokens.find(current);

        if (mt != markup_tokens.end()
            && is_markup_allowed(mt->first)
            && (is_space_or_newline(previous) || is_markup_token(previous)))
        {
            if (is_space_or_newline(lexer->lookahead))
                return false;

            // Empty markup. I.e: **, or //, or ``, ...
            if (lexer->lookahead == mt->first) {
                while (lexer->lookahead == mt->first)
                    advance();
                return false;
            }

            markup_stack.push_back(mt->first);
            debug_markup_stack();

            lexer->result_symbol = mt->second;
            debug_result();
            return true;
        }

        return false;
    };

    bool parse_close_markup() {
        if (markup_stack.empty() || parsed_chars != 1
            || current != markup_stack.back() || iswspace(previous))
            return false;

        if (is_space_or_newline(lexer->lookahead)
            || is_punkt(lexer->lookahead)
            || (markup_stack.size() > 1 && lexer->lookahead == markup_stack.end()[-2]))
        {
            lexer->result_symbol =
                static_cast<TokenType>(markup_tokens.at(current) + MARKUP);
            markup_stack.pop_back();
            debug_markup_stack();
            debug_result();
            return true;
        }
        return false;
    }

    bool parse_check_box() {
        if (parsed_chars != 1) return false;

        if (valid_tokens[CHECKBOX_OPEN]
            && current == '[' && is_checkbox_content(lexer->lookahead))
        {
            lexer->mark_end(lexer);

            advance();
            if (lexer->lookahead != ']') return false;

            advance();
            if (!is_space(lexer->lookahead)) return false;

            lexer->result_symbol = CHECKBOX_OPEN;
            debug_result();
            return true;
        }
        else if (valid_tokens[CHECKBOX_DONE] && lexer->lookahead == ']') {
            switch (current) {
            case ' ':
                lexer->result_symbol = CHECKBOX_UNDONE;
                break;
            case 'x':
                lexer->result_symbol = CHECKBOX_DONE;
                break;
            case '-':
                lexer->result_symbol = CHECKBOX_PENDING;
                break;
            case '!':
                lexer->result_symbol = CHECKBOX_URGENT;
                break;
            }
            debug_result();
            return true;
        }
        else if (valid_tokens[CHECKBOX_CLOSE]
            && current == ']' && is_space_or_newline(lexer->lookahead))
        {
            lexer->result_symbol = CHECKBOX_CLOSE;
            debug_result();
            return true;
        }

        return false;
    }

    bool parse_word() {
        if (!current) return false;

        while (lexer->lookahead && !iswspace(lexer->lookahead)) {
            if (!markup_stack.empty()
                && lexer->lookahead == markup_stack.back()
                && !iswspace(current))
            {
                lexer->mark_end(lexer);
                advance();
                if (is_space_or_newline(lexer->lookahead)
                    || is_punkt(lexer->lookahead)
                    || (markup_stack.size() > 1 && lexer->lookahead == markup_stack.end()[-2]))
                    break;
            }
            advance();
            lexer->mark_end(lexer);
        }
        lexer->result_symbol = WORD;
        debug_result();
        return true;
    }

    bool parse_raw_word() {
        while (lexer->lookahead && !iswspace(lexer->lookahead))
            advance();
        lexer->result_symbol = RAW_WORD;
        debug_result();
        return true;
    }

    // inline uint32_t get_column() { return is_eof() ? 0 : lexer->get_column(lexer); }
    inline uint32_t get_column() { return lexer->get_column(lexer); }

    inline bool is_space(const int32_t c) { return c && iswblank(c); }

    // inline bool is_newline(const int32_t c) { return c && iswspace(c) && !iswblank(c); }
    inline bool is_newline(const int32_t c) {
        switch (c) {
        case 10: // \n
        case 13: // \r
            return true;
        default:
            return false;
        }
    }

    inline bool is_space_or_newline(const int32_t c) { return !c || iswspace(c); }

    inline bool is_eof() { return lexer->eof(lexer); }

    inline bool is_markup_allowed(const int32_t c) {
        if (markup_stack.empty())
            return true;

        for (auto m : markup_stack)
            if (c == m) return false;

        switch (markup_tokens.at(markup_stack.back())) {
        case VERBATIM:
        case INLINE_MATH:
            return false;
        default:
            return true;
        }
    };

    inline bool is_markup_token(int32_t c) { return markup_tokens.find(c) != markup_tokens.end(); }

    inline bool is_checkbox_content(int32_t c) {
        switch (c) {
        case ' ': // undone
        case 'x': // done
        case '-': // pending
        case '!': // urgent
            return true;
        default:
            return false;
        }
    }

    bool is_punkt(int32_t c) {
        switch (c) {
        case '.': case ',': case ':': case ';':
        case '!': case '?':
        case '"': case '\'':
            return true;
        default:
            return false;
        }
    }

    bool token(const string str) {
        for (int32_t c : str) {
            if (c == lexer->lookahead)
                advance();
            else
                return false;
        }
        return true;
    }

    /**
     * The parser appears to call `scan` with all symbols declared as valid directly
     * after it encountered an error, so this function is used to detect them.
     */
    bool is_all_tokens_valid() {
        for (int i = 0; i <= NONE; ++i)
            if (!valid_tokens[i]) return false;
        return true;
    }

    inline void debug_valid_tokens() {
#ifdef DEBUG
        clog << "  Valid symbols: ";

        if (is_all_tokens_valid()) {
            cout << "all" << endl;
            return;
        }

        for (int i = 0; i <= NONE; ++i) {
            if (valid_tokens[i])
                clog << tokens_names[i] << ' ';
        }

        clog << endl;
#endif
    }

    inline void debug_result() {
#ifdef DEBUG
        clog << "  found: " << tokens_names[lexer->result_symbol] << endl
             // << "  parsed chars: " << m_parsed_chars << endl
             << "}" << endl;
#endif
    }

    inline void debug_markup_stack() {
#ifdef DEBUG
        cout << "  markup stack: ";
        for (auto m : markup_stack)
            cout << m << ", ";
        cout << endl;
#endif
    }
};

extern "C"
{
    void* tree_sitter_skald_external_scanner_create() { return new Scanner(); }

    void tree_sitter_skald_external_scanner_destroy(void* payload) {
        delete static_cast<Scanner*>(payload);
    }

    bool tree_sitter_skald_external_scanner_scan(void* payload, TSLexer* lexer,
                                                 const bool* valid_tokens)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);
        scanner->lexer = lexer;
        scanner->valid_tokens = const_cast<bool*>(valid_tokens);
        return scanner->scan();
    }

    unsigned tree_sitter_skald_external_scanner_serialize(void* payload, char* buffer)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);

        char stack_length = scanner->markup_stack.size();

        if (static_cast<int>(stack_length) >= TREE_SITTER_SERIALIZATION_BUFFER_SIZE) return 0;

        int n = 0;

        if (stack_length)
            for (char m : scanner->markup_stack) {
                buffer[n] = m;
                ++n;
            }

        scanner->markup_stack.clear();
        return n;
    }

    void tree_sitter_skald_external_scanner_deserialize(void* payload, const char* buffer,
                                                        unsigned length)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);
        scanner->current = 0;
        scanner->parsed_chars = 0;
        scanner->tag_parameter_is_valid = true;

        if (!length) return;

        int n = 0;
        for (int8_t i = 0; i < length - n; ++i)
            scanner->markup_stack.push_back(buffer[n + i]);
    }
}
