#include <algorithm>
#include <cstdint>
#include <vector>
#include <bitset>
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

enum TokenType : char {
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

    SPACE,
    BLANK_LINE,

    TAG_END,

    WORD,

    NONE,
};

constexpr int8_t MARKUP = 6; //< Total number of markup tokens.

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

    "space",
    "blank_line",

    "tag_end",

    "word",

    "none",
};
#endif // DEBUG

struct Scanner
{
    TSLexer* lexer;

    int32_t
    m_previous = 0, //< Previous char
    m_current = 0;  //< Current char

    uint16_t m_parsed_chars = 0; //< Number of parsed chars

    // The last matched token type (used to detect things like todo items which
    // require an unordered list prefix beforehand).
    TokenType m_last_token = NONE;

    const unordered_map<char, TokenType> markup_tokens = {
        {'*', BOLD},
        {'/', ITALIC},
        {'-', STRIKETHROUGH},
        {'_', UNDERLINE},
        {'`', VERBATIM},
        {'$', INLINE_MATH},
    };

    deque<char> markup_stack;

    bool scan (const bool* valid_symbols) {
        debug_valid_symbols(valid_symbols);

        skip_spaces();

        if (is_newline(lexer->lookahead))
            if (parse_newline()) return true;

        if (parse_open_markup()) return true;
        if (parse_close_markup()) return true;

        if (is_eof()) {
#ifdef DEBUG
            clog << "  End of the file!" << endl << "}" << endl << endl;
#endif
            return false;
        }

        // if (get_column() == 0)
        //      res = parse_newline();
        // if (res) return true;

        if (parse_text()) return true;

#ifdef DEBUG
        clog << "  false" << endl << "}" << endl;
#endif

        return false;
    };

    // Advances the lexer forward. The char that was advanced will be returned
    // in the final result.
    void advance() {
        m_previous = m_current;
        m_current = lexer->lookahead;
        lexer->advance(lexer, false);
        ++m_parsed_chars;

#ifdef DEBUG_CURRENT_CHAR
        clog << "  -> ";
        switch (m_current) {
        case 10:
            clog << "\\n";
            break;
        case 0:
            clog << "\\0";
            break;
        default:
            clog << (char)m_current;
        }
        clog << endl;
#endif
    }

    // Skips the next character without including it in the final result.
    void skip() {
        m_previous = m_current;
        m_current = lexer->lookahead;
        lexer->advance(lexer, true);
    }

    inline void skip_spaces() {
        while (is_space(lexer->lookahead)) skip();
#ifdef DEBUG
        clog << "  skip spaces" << endl;
#endif
    }

    /**
     * @brief Rules that decide based on the first token on the next line.
     * @return Whether to finish parsing.
     */
    bool parse_newline() {
        skip(); // skip newline char
        skip_spaces();

        // Check if current line is empty line.
        if (is_newline(lexer->lookahead)) {
            advance();
            lexer->result_symbol = m_last_token = BLANK_LINE;
            debug_result();
            return true;
        }

        size_t parsed_chars = 0;

        // switch (lexer->lookahead) {
        // case '*':
        //     auto expected = '*';
        //     do {
        //         if (lexer->lookahead != expected) break;
        //         advance()
        //     } while ();
        // }


        // // We are dealing with a ranged verbatim tag: @something
        // if (lexer->lookahead == '@') {
        //     advance();
        //
        //     // Check whether the tag is `@end`.
        //     if (token("end") && (!lexer->lookahead)|| iswspace(lexer->lookahead)) {
        //         lexer->result_symbol = m_last_token = TAG_END;
        //         // m_active_markup.reset();
        //         return true;
        //     }
        //
        // }

        return false;
    }

    bool parse_open_markup() {
        /// Markup token
        auto mt = markup_tokens.find(lexer->lookahead);
        if (mt != markup_tokens.end()
            && is_markup_allowed(mt->first)
            && (!m_current || iswblank(m_current) || is_markup_token(m_current)))
        {
            advance();

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

            lexer->result_symbol = m_last_token = mt->second;
            debug_result();
            return true;
        }

        return false;
    };

    bool parse_close_markup() {
        if (markup_stack.empty() || lexer->lookahead != markup_stack.back()
            || iswspace(m_current))
            return false;

        char tkn = lexer->lookahead;
        advance();
        if (is_space_or_newline(lexer->lookahead)
            || (markup_stack.size() > 1 && lexer->lookahead == markup_stack.end()[-2]))
        {
            lexer->result_symbol = m_last_token =
                static_cast<TokenType>(markup_tokens.at(tkn) + MARKUP);
            markup_stack.pop_back();
            debug_markup_stack();
            debug_result();
            return true;
        }
        return false;
    }

    bool parse_text() {
        while (lexer->lookahead && !iswspace(lexer->lookahead)) {
            if (!markup_stack.empty()
                && lexer->lookahead == markup_stack.back()
                && !iswspace(m_current))
            {
                lexer->mark_end(lexer);
                advance();
                if (is_space_or_newline(lexer->lookahead)
                    || is_markup_token(lexer->lookahead)) break;
            }
            advance();
            lexer->mark_end(lexer);
        }
        lexer->result_symbol = m_last_token = WORD;
        debug_result();
        return true;
    }

    // inline uint32_t get_column() { return is_eof() ? 0 : lexer->get_column(lexer); }
    inline uint32_t get_column() { return lexer->get_column(lexer); }

    inline bool is_space(const int32_t c) { return c && iswblank(c); }

    inline bool is_newline(const int32_t c) { return c && iswspace(c) && !iswblank(c); }

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
    bool is_all_tokens(const bool* valid_symbols) {
        for (int i = 0; i <= NONE; ++i)
            if (!valid_symbols[i]) return false;
        return true;
    }

    inline void debug_valid_symbols(const bool *valid_symbols) {
#ifdef DEBUG
        clog << "{" << endl
             << "  Valid symbols: ";

        if (is_all_tokens(valid_symbols)) {
            cout << "all" << endl;
            return;
        }

        for (int i = 0; i <= NONE; ++i) {
            if (valid_symbols[i])
                clog << tokens_names[i] << ' ';
        }

        clog << endl;
#endif
    }

    inline void debug_result() {
#ifdef DEBUG
        clog << "  found: " << tokens_names[m_last_token] << endl
             << "  parsed chars: " << m_parsed_chars << endl
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
    void* tree_sitter_note_external_scanner_create() { return new Scanner(); }

    void tree_sitter_note_external_scanner_destroy(void* payload) {
        delete static_cast<Scanner*>(payload);
    }

    bool tree_sitter_note_external_scanner_scan(void* payload, TSLexer* lexer,
                                                const bool* valid_symbols)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);
        scanner->lexer = lexer;
        return scanner->scan(valid_symbols);
    }

    unsigned tree_sitter_note_external_scanner_serialize(void* payload, char* buffer)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);

        auto to_copy = sizeof(scanner->m_current);
        char stack_length = scanner->markup_stack.size();

        if (to_copy + stack_length >= TREE_SITTER_SERIALIZATION_BUFFER_SIZE) return 0;

        int n = 0;

        memcpy(buffer + n, &scanner->m_current, to_copy);
        n += to_copy;

        if (stack_length)
            for (char m : scanner->markup_stack) {
                buffer[n] = m;
                ++n;
            }

        scanner->markup_stack.clear();
        return n;
    }

    void tree_sitter_note_external_scanner_deserialize(void* payload, const char* buffer,
                                                       unsigned length)
    {
        Scanner* scanner = static_cast<Scanner*>(payload);
        scanner->m_parsed_chars = 0;

        if (!length) {
            scanner->m_current = 0;
            return;
        };

        int n = 0;

        auto to_copy = sizeof(scanner->m_current);
        memcpy(&scanner->m_current, buffer + n, to_copy);
        n += to_copy;

        for (int8_t i = 0; i < length - n; ++i)
            scanner->markup_stack.push_back(buffer[n + i]);
    }
}
