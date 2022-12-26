#include <functional>
#include <vector>
#include <cstring>
#include <cwctype>
#include <iostream>
#include <iomanip>
#include <string>
#include <stack>
#include <unordered_map>
#include "tree_sitter/parser.h"

// #define DEBUG 1

/**
 * Print the current character after every advance() call.
 */
// #define DEBUG_CURRENT_CHAR 1

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

    DEFINITION_TERM_BEGIN,
    DEFINITION_TERM_END,
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
    CHECKBOX_UNCERTAIN,

    MARKDOWN_CODE_BLOCK,
    TAG_TOKEN,
    EXTENDED_TAG_TOKEN,
    TAG_NAME,
    END_TAG,
    HASHTAG,
    TAG_PARAMETER,

    INLINE_TAG_TOKEN,
    INLINE_TAG_LABEL_OPEN,
    INLINE_TAG_LABEL_CLOSE,
    INLINE_TAG_PARAMETERS_OPEN,
    INLINE_TAG_PARAMETERS_CLOSE,

    LINK_LABEL_OPEN,
    LINK_LABEL_CLOSE,

    LINK_LABEL2_OPEN,
    LINK_LABEL2_CLOSE,

    LINK_LOCATION_OPEN,
    LINK_LOCATION_CLOSE,

    BLANK_LINE,
    SOFT_BREAK,
    HARD_BREAK,

    ESCAPE,
    COMMENT,

    WORD,
    RAW_WORD,
    NEW_LINE,

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

    "definition_term_begin",
    "definition_term_end",
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
    "checkbox_uncertain",

    "markdown_code_block",
    "tag_token",
    "extended_tag_token",
    "tag_name",
    "end_tag",
    "hashtag",
    "tag_parameter",

    "inline_tag_token",
    "inline_tag_label_open",
    "inline_tag_label_close",
    "inline_tag_parameters_open",
    "inline_tag_parameters_close",

    "link_label_open",
    "link_label_close",

    "link_label2_open",
    "link_label2_close",

    "link_location_open",
    "link_location_close",

    "blank_line",
    "soft_break",
    "hard_break",

    "escape",
    "comment",

    "word",
    "raw_word",
    "new_line",

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

struct Scanner
{
    TSLexer* lexer;

    const bool* valid_tokens;

    int32_t
    previous = 0, //< Previous char
    current = 0;  //< Current char

    /// Number of parsed chars since last space char.
    size_t parsed_chars = 0;

    deque<char> markup_stack;

    bool tag_parameter_is_valid = true;

    /**
     * The scanner analyzes the current character --- the last character the
     * "advanced" function passed, not the next (i.e lexer->lookahead) char.
     *
     *     abc
     *     ││└─ lexer->lookahead
     *     │└─ current
     *     └─ previous
     *
     */
    bool scan () {
        if (is_all_tokens_valid() || is_eof()) return false;

#ifdef DEBUG
        clog << "{" << endl;
        debug_valid_tokens();
#endif

        if (get_column() == 0)
            if (parse_newline()) return true;

        if (parse_ordered_list()) return true;
        if (parse_tag_name()) return true;

        if (parsed_chars == 0) {
            skip_spaces();
            if (is_eof()) return false;
        }

        if (parsed_chars == 0 && is_newline(lexer->lookahead)) {
            skip_newline();
            if (parse_newline()) return true;
        }

        //- before advace -------------------------------

        if (parsed_chars == 0) advance();

        //- after advance -------------------------------

        if (parse_tag_parameter()) return true;
        if (parse_inline_tag()) return true;

        if (parse_comment()) return true;
        if (parse_escape_char()) return true;

        if (parse_checkbox()) return true;
        if (prase_link()) return true;

        if (parse_definition()) return true;
        if (parse_open_markup()) return true;
        if (parse_close_markup()) return true;

        if (parse_raw_word()) return true;
        if (parse_word()) return true;

#ifdef DEBUG
        clog << "  false" << endl << "}" << endl;
#endif

        return false;
    };

    /// Advances the lexer forward. The char that was advanced
    /// will be returned in the final result.
    void advance() {
//         if (!lexer->lookahead) {
// #ifdef DEBUG_CURRENT_CHAR
//             clog << "  Next char is \\0" << endl;
// #endif
//             return;
//         }
        previous = current;
        current = lexer->lookahead;
        lexer->advance(lexer, false);
        ++parsed_chars;

#ifdef DEBUG_CURRENT_CHAR
        clog << "  -> " << setw(3) << current << ' ';
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
//         if (!lexer->lookahead) {
// #ifdef DEBUG_CURRENT_CHAR
//             clog << "  Next char is \\0" << endl;
// #endif
//             return;
//         }
        previous = current;
        current = lexer->lookahead;
        lexer->advance(lexer, true);
        parsed_chars = 0;
    }

    inline void skip_spaces() {
        // CHECKBOX_UNDONE token is a space, and we don't want to skip it.
        if (valid_tokens[CHECKBOX_UNDONE])
            return;

        while (is_space(lexer->lookahead))
            skip();
    }

    /// Skip newline char
    inline void skip_newline() {
        if (lexer->lookahead == 13) skip(); // \r
        if (lexer->lookahead == 10) skip(); // \n
    }

    /// Rules that decide based on the first token on the next line.
    bool parse_newline() {
        // If we're here, then we're in column 0 on a new line.

        skip_spaces();
        if (is_eof()) return false;

        // TAG_PARAMETER token, valid only on the same line as a range tag definition.
        // That's why if we are on the new line, then TAG_PARAMETER stops to be valid.
        tag_parameter_is_valid = false;

        // Check if current line is empty line.
        if (is_newline(lexer->lookahead)) {
            advance();
            return found(BLANK_LINE);
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

                if (!is_space(lexer->lookahead))
                    return false;

                return found(static_cast<TokenType>(
                             HEADING_1 + (n < MAX_HEADING ? n : MAX_HEADING - 1)));
            }
            // We are on the first non-blank character of the line, and it is '*',
            // need to check bold markup.
            else if (parse_open_markup()) return true;
            break;
        }
        case ':': { // DEFINITION
            // if (valid_tokens[DEFINITION_TERM_BEGIN] || valid_tokens[DEFINITION_END]) {
            //     if (is_space(lexer->lookahead))
            //         return found(DEFINITION_TERM_BEGIN);
            //     else if (is_newline(lexer->lookahead))
            //         return found(DEFINITION_END);
            // }
            if (valid_tokens[DEFINITION_TERM_BEGIN] && is_space(lexer->lookahead))
                return found(DEFINITION_TERM_BEGIN);
            else if (valid_tokens[DEFINITION_END] && is_newline(lexer->lookahead))
                return found(DEFINITION_END);
            break;
        }
        case '-': { // LIST, SOFT_BREAK
            constexpr auto expected = '-';
            while (lexer->lookahead == expected) {
                advance();
                ++n;
            }

            if (1 + n == 3 && (is_newline_or_eof(lexer->lookahead)))
                return found(SOFT_BREAK);

            if (iswdigit(lexer->lookahead)) {
                lexer->mark_end(lexer);
                while (iswdigit(lexer->lookahead))
                    advance();
            }

            if (is_space(lexer->lookahead))
                return found(static_cast<TokenType>(
                             LIST_1 + (n < MAX_LIST ? n : MAX_LIST - 1)));

            break;
        }
        case '=': { // HARD_BREAK
            while (lexer->lookahead == '=') {
                advance();
                ++n;
            }
            if (1 + n == 3 && (is_newline_or_eof(lexer->lookahead)))
                return found(HARD_BREAK);
            break;
        }
        case '@': { // Tags
            if (valid_tokens[EXTENDED_TAG_TOKEN]
                && lexer->lookahead && lexer->lookahead == '+')
            {
                advance();
                return found(EXTENDED_TAG_TOKEN);
            }
            else if (valid_tokens[TAG_TOKEN] && !iswspace(lexer->lookahead)) {
                return found(TAG_TOKEN);
            }
            break;
        }
        case '#': { // HASHTAG or COMMENT
            if (not_space_or_newline(lexer->lookahead))
                return found(HASHTAG);
            // if (is_space_or_newline(lexer->lookahead))
            //     return found(COMMENT);
            // else {
            //     while (not_space_or_newline(lexer->lookahead))
            //         advance();
            //     return found(HASHTAG);
            // }
            break;
        }
        case '`': { // MARKDOWN_CODE_BLOCK
            while (lexer->lookahead == '`') {
                advance();
                ++n;
            }
            if (1 + n == 3)
                return found(MARKDOWN_CODE_BLOCK);
            break;
        }
        }

        return false;
    }

    inline bool parse_tag_name() {
        // if (is_space(previous)) return false;
        if (current) return false;

        if (valid_tokens[END_TAG] && token("end")
            && is_space_or_newline_or_eof(lexer->lookahead))
        {
            return found(END_TAG);
        }
        else if (valid_tokens[TAG_NAME]) {
            while (not_space_or_newline(lexer->lookahead)
                   && !is_inline_tag_control_character(lexer->lookahead))
                advance();
            return found(TAG_NAME);
        }
        return false;
    }

    inline bool parse_inline_tag() {
        if (valid_tokens[INLINE_TAG_TOKEN] && current == ':')
            return found(INLINE_TAG_TOKEN);
        else if (valid_tokens[INLINE_TAG_LABEL_OPEN] && current == '[' && !previous)
            return found(INLINE_TAG_LABEL_OPEN);
        else if (valid_tokens[INLINE_TAG_LABEL_CLOSE] && current == ']' && !previous)
            return found(INLINE_TAG_LABEL_CLOSE);
        else if (valid_tokens[INLINE_TAG_PARAMETERS_OPEN] && current == '{'  && !previous)
            return found(INLINE_TAG_PARAMETERS_OPEN);
        else if (valid_tokens[INLINE_TAG_PARAMETERS_CLOSE] && current == '}')
            return found(INLINE_TAG_PARAMETERS_CLOSE);
        return false;
    }

    /**
     * Parse tag parameter. It is `param1` and `param2` in examples below:
     *   #tag param1 param2
     *        ^----- ^-----
     * or
     *   @tag param1 param2
     *        ^----- ^-----
     */
    inline bool parse_tag_parameter() {
        if (valid_tokens[TAG_PARAMETER] && tag_parameter_is_valid) {
            while (not_space_or_newline(lexer->lookahead))
                advance();

            if (current)
                return found(TAG_PARAMETER);
        }
        return false;
    }

    bool parse_definition() {
        if (parsed_chars != 1) return false;

        if (valid_tokens[DEFINITION_TERM_END]
            && current == ':'
            && is_space_or_newline_or_eof(lexer->lookahead))
        {
            return found(DEFINITION_TERM_END);
        }
        else if (valid_tokens[DEFINITION_END]
            && current == ':'
            && is_newline(lexer->lookahead))
        {
            return found(DEFINITION_END);
        }

        return false;
    }

    /// Parse the label part of the ordered list token.
    inline bool parse_ordered_list() {
        if (parsed_chars == 0
            && valid_tokens[ORDERED_LIST_LABEL]
            && iswdigit(lexer->lookahead))
        {
            /*
             *    ┌─ level
             *    │  ┌─ label
             *    │  │
             *    ---12 text
             *      🠕
             *      we are here
             */
            while (iswdigit(lexer->lookahead))
                advance();
            return found(ORDERED_LIST_LABEL);
        }
        return false;
    }

    bool parse_comment() {
        if (current == '#' && is_space_or_newline_or_eof(lexer->lookahead))
            return found(COMMENT);
        return false;
    }

    bool parse_escape_char() {
        if (current == '\\')
            return found(ESCAPE);
        return false;
    }

    bool prase_link() {
        if (parsed_chars != 1) return false;
        switch (current) {
        case '[': {
            if (valid_tokens[LINK_LABEL2_OPEN] && !previous)
                return found(LINK_LABEL2_OPEN);
            else if (valid_tokens[LINK_LABEL_OPEN])
                return found(LINK_LABEL_OPEN);
            break;
        }
        case ']': {
            if (valid_tokens[LINK_LABEL2_CLOSE])
                return found(LINK_LABEL2_CLOSE);
            else if (valid_tokens[LINK_LABEL_CLOSE] && tag_parameter_is_valid)
                return found(LINK_LABEL_CLOSE);
            break;
        }
        case '(': {
            if (valid_tokens[LINK_LOCATION_OPEN] && !previous)
                return found(LINK_LOCATION_OPEN);
            break;
        }
        case ')': {
            if (valid_tokens[LINK_LOCATION_CLOSE])
                return found(LINK_LOCATION_CLOSE);
            break;
        }
        }
        return false;
    }

    bool parse_open_markup() {
        if (parsed_chars != 1) return false;

        /// Markup token
        auto mt = markup_tokens.find(current);

        if (mt != markup_tokens.end()
            && is_markup_allowed(mt->first)
            && (is_space_or_newline_or_eof(previous) || is_markup_token(previous)))
        {
            if (is_space_or_newline_or_eof(lexer->lookahead))
                return false;

            // Empty markup. I.e: **, or //, or ``, ...
            if (lexer->lookahead == mt->first) {
                while (lexer->lookahead == mt->first)
                    advance();
                return false;
            }

            markup_stack.push_back(mt->first);
            debug_markup_stack();

            return found(mt->second);
        }

        return false;
    };

    bool parse_close_markup() {
        if (markup_stack.empty() || parsed_chars != 1
            || current != markup_stack.back() || iswspace(previous))
            return false;

        if (is_space_or_newline_or_eof(lexer->lookahead)
            || is_punkt(lexer->lookahead)
            || (markup_stack.size() > 1 && lexer->lookahead == markup_stack.end()[-2]))
        {
            found(static_cast<TokenType>(markup_tokens.at(current) + MARKUP));
            markup_stack.pop_back();
            debug_markup_stack();
            return true;
        }
        return false;
    }

    bool parse_checkbox() {
        if (parsed_chars != 1) return false;

        if (valid_tokens[CHECKBOX_OPEN]
            && current == '[' && is_checkbox_content(lexer->lookahead))
        {
            lexer->mark_end(lexer);

            advance();
            if (lexer->lookahead != ']') return false;

            advance();
            if (!is_space(lexer->lookahead)) return false;

            return found(CHECKBOX_OPEN);
        }
        else if (valid_tokens[CHECKBOX_DONE] && lexer->lookahead == ']') {
            switch (current) {
            case ' ':
                return found(CHECKBOX_UNDONE);
            case 'x':
                return found(CHECKBOX_DONE);
            case '-':
                return found(CHECKBOX_PENDING);
            case '!':
                return found(CHECKBOX_URGENT);
            case '?':
                return found(CHECKBOX_UNCERTAIN);
            }
        }
        else if (valid_tokens[CHECKBOX_CLOSE]
            && current == ']' && is_space_or_newline_or_eof(lexer->lookahead))
        {
            return found(CHECKBOX_CLOSE);
        }

        return false;
    }

    bool parse_word() {
        if (!valid_tokens[WORD] || is_eof())
            return false;

        while (not_space_or_newline(lexer->lookahead)) {
            if (!markup_stack.empty()
                && !iswspace(current)
                && lexer->lookahead == markup_stack.back())
            {
                lexer->mark_end(lexer);
                advance();
                if (is_space_or_newline_or_eof(lexer->lookahead)
                    || is_punkt(lexer->lookahead)
                    || (markup_stack.size() > 1 && lexer->lookahead == markup_stack.end()[-2]))
                    break;
            }
            // else if (valid_tokens[LINK_LABEL_CLOSE] && lexer->lookahead == ']')
            else if (lexer->lookahead == ']') // Link label closing bracket.
                break;
            advance();
            lexer->mark_end(lexer);
        }
        return found(WORD);
    }

    /// RAW_WORD is a sequence of any characters until space or new line char.
    inline bool parse_raw_word() {
        if (!valid_tokens[RAW_WORD] || is_eof())
            return false;

        while (not_space_or_newline(lexer->lookahead)
               && !(valid_tokens[LINK_LOCATION_CLOSE] && lexer->lookahead == ')')
               && !(valid_tokens[INLINE_TAG_PARAMETERS_CLOSE] && lexer->lookahead == '}'))
        {
            advance();
        }

        return found(RAW_WORD);
    }

    inline bool found(TokenType token) {
        lexer->result_symbol = token;
#ifdef DEBUG
        clog << "  found: " << tokens_names[lexer->result_symbol] << endl
             // << "  parsed chars: " << m_parsed_chars << endl
             << "}" << endl;
#endif
        return true;
    }

    // inline uint32_t get_column() { return is_eof() ? 0 : lexer->get_column(lexer); }
    inline uint32_t get_column() { return lexer->get_column(lexer); }

    inline bool is_space(const int32_t c) { return c && iswblank(c); }

    inline bool is_newline(const int32_t c) {
        switch (c) {
        case 10: // \n
        case 13: // \r
            return true;
        default:
            return false;
        }
    }

    inline bool is_newline_or_eof(const int32_t c) { return !c || is_newline(c); }

    inline bool is_space_or_newline_or_eof(const int32_t c) { return !c || iswspace(c); }

    inline bool not_space_or_newline(const int32_t c) { return c && !iswspace(c); }

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

    inline bool is_inline_tag_control_character(int32_t c) {
        switch (c) {
        case '(':
        case '[':
        case '{':
            return true;
        default:
            return false;
        }
    }

    inline bool is_checkbox_content(int32_t c) {
        switch (c) {
        case ' ': // undone
        case 'x': // done
        case '-': // pending
        case '!': // urgent
        case '?': // uncertain
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
        case '[': case ']':
        case '(': case ')':
        case '{': case '}':
            return true;
        default:
            return false;
        }
    }

    bool token(const string str)
    {
        for (int32_t c : str)
        {
            if (c == lexer->lookahead)
                advance();
            else
                return false;
        }
        return true;
    }

    /**
     * The parser appears to call `scan` with all symbols declared as valid directly
     * after it encountered an error. This function defines such a case.
     */
    inline bool is_all_tokens_valid() {
        for (int i = 0; i <= NONE; ++i)
            if (!valid_tokens[i]) return false;
        return true;
    }

    inline void debug_valid_tokens() {
#ifdef DEBUG
        clog << "  Valid: ";

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
    /// Create new Scanner object
    void* tree_sitter_skald_external_scanner_create() { return new Scanner(); }

    /// Destroy Scanner object
    void tree_sitter_skald_external_scanner_destroy(void* payload) {
        delete static_cast<Scanner*>(payload);
    }

    /// Main logic entry point
    bool tree_sitter_skald_external_scanner_scan(void* payload, TSLexer* lexer,
                                                 const bool* valid_tokens)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);
        scanner->lexer = lexer;
        scanner->valid_tokens = const_cast<bool*>(valid_tokens);
        return scanner->scan();
    }

    /// Copy the current Scanner state to another location for later reuse.
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

    /// Load another Scanner state into Scanner object,
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
