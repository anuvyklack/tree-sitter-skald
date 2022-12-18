const MAX_HEADING = 6
const MAX_LIST_LEVEL = 10

const skald_grammar = {
  name: 'skald',

  externals: $ => [
    $.bold_open,
    $.italic_open,
    $.strikethrough_open,
    $.underline_open,
    $.verbatim_open,
    $.inline_math_open,

    $.bold_close,
    $.italic_close,
    $.strikethrough_close,
    $.underline_close,
    $.verbatim_close,
    $.inline_math_close,

    $.heading_1_prefix,
    $.heading_2_prefix,
    $.heading_3_prefix,
    $.heading_4_prefix,
    $.heading_5_prefix,
    $.heading_6_prefix,

    $.definition_begin,
    $.definition_separator,
    $.definition_end,

    $.list_1_prefix,
    $.list_2_prefix,
    $.list_3_prefix,
    $.list_4_prefix,
    $.list_5_prefix,
    $.list_6_prefix,
    $.list_7_prefix,
    $.list_8_prefix,
    $.list_9_prefix,
    $.list_10_prefix,

    $.ordered_list_label,

    $.checkbox_open,
    $.checkbox_close,

    $.checkbox_undone,  // [ ]
    $.checkbox_done,    // [x]
    $.checkbox_pending, // [-]
    $.checkbox_urgent,  // [!]

    $.code_block_begin,
    $.ranged_tag_begin,
    $.ranged_tag_end,
    $.hashtag_begin,
    $.tag_parameter,

    $._blank_line,
    $._soft_break,
    $._hard_break,

    $.escape_char,

    $._word,
    $._raw_word,

    $._, // none
  ],

  // conflicts: $ => [
  //   [$.list_1, $.ordered_list_1],
  // ],

  inline: $ => [
    $.section,
  ],

  // // https://github.com/tree-sitter/tree-sitter/pull/939
  // precedences: _ => [
  //   ['document_directive', 'body_directive'],
  //   ['special', 'immediate', 'non-immediate'],
  // ],

  rules: {
    document: $ => repeat1(
      choice(
        alias($.section, $.section),
        $.list,
        $.definition,
        $.code_block,
        $.ranged_tag,
        $.hashtag,
        $.paragraph,
        $._blank_line,
        $._hard_break,
        $.escaped_sequence
      )
    ),

    paragraph: $ => prec.right(
      repeat1(
        choice(
          $._word,
          $.bold,
          $.italic,
          $.strikethrough,
          $.underline,
          $.verbatim,
          $.inline_math,
        )
      ),
    ),

    section: $ => choice(
      $.section1, $.section2, $.section3, $.section4, $.section5, $.section6
    ),

    definition: $ => seq(
      alias($.definition_begin, "token"),

      alias(
        repeat1(alias($.paragraph, "paragraph")),
        $.term
      ),

      alias($.definition_separator, "token"),

      alias(
        repeat1(alias($.paragraph, "paragraph")),
        $.description
      ),

      alias($.definition_end, "token")
    ),

    list: $ => seq(
      choice(
        repeat1($.unordered_list_1),
        repeat1($.ordered_list_1)
      ),
      $._soft_break
    ),

    code_block: $ => seq(
      alias($.code_block_begin, $.tag),
      optional(
        alias($.tag_parameter, $.language)
      ),
      alias(
        repeat($._raw_word),
        $.code
      ),
      alias($.ranged_tag_end, $.end_tag)
    ),

    ranged_tag: $ => seq(
      alias($.ranged_tag_begin, $.tag),
      repeat($.tag_parameter),
      repeat(
        choice(
          $.list,
          $.definition,
          $.code_block,
          $.hashtag,
          $.paragraph,
          $._blank_line,
          $.escaped_sequence,
        )
      ),
      alias($.ranged_tag_end, $.end_tag)
    ),

    hashtag: $=> seq(
      alias($.hashtag_begin, $.tag),
      repeat($.tag_parameter)
    ),

    escaped_sequence: $=> seq(
      $.escape_char,
      alias($._raw_word, "_word")
    ),

    // _word: _ => choice(/\p{L}+/, /\p{N}+/),
  }
}

const sections = {
  section1: $ => gen_section($, 1),
  section2: $ => gen_section($, 2),
  section3: $ => gen_section($, 3),
  section4: $ => gen_section($, 4),
  section5: $ => gen_section($, 5),
  section6: $ => gen_section($, 6),

  heading1: $ => gen_heading($, 1),
  heading2: $ => gen_heading($, 2),
  heading3: $ => gen_heading($, 3),
  heading4: $ => gen_heading($, 4),
  heading5: $ => gen_heading($, 5),
  heading6: $ => gen_heading($, 6),
}

const lists = {
  checkbox: $ => seq(
    alias($.checkbox_open, "open"),
    choice(
      alias($.checkbox_undone,  "undone"),
      alias($.checkbox_done,    "done"),
      alias($.checkbox_pending, "pending"),
      alias($.checkbox_urgent,  "urgent")
    ),
    alias($.checkbox_close, "close")
  ),

  unordered_list_1: $ => gen_list($, 1),
  unordered_list_2: $ => gen_list($, 2),
  unordered_list_3: $ => gen_list($, 3),
  unordered_list_4: $ => gen_list($, 4),
  unordered_list_5: $ => gen_list($, 5),
  unordered_list_6: $ => gen_list($, 6),
  unordered_list_7: $ => gen_list($, 7),
  unordered_list_8: $ => gen_list($, 8),
  unordered_list_9: $ => gen_list($, 9),
  unordered_list_10:$ => gen_list($,10),

  ordered_list_1: $ => gen_list($, 1, true),
  ordered_list_2: $ => gen_list($, 2, true),
  ordered_list_3: $ => gen_list($, 3, true),
  ordered_list_4: $ => gen_list($, 4, true),
  ordered_list_5: $ => gen_list($, 5, true),
  ordered_list_6: $ => gen_list($, 6, true),
  ordered_list_7: $ => gen_list($, 7, true),
  ordered_list_8: $ => gen_list($, 8, true),
  ordered_list_9: $ => gen_list($, 9, true),
  ordered_list_10:$ => gen_list($,10, true),
}

const markup = {
  bold: $ => prec.right(
    seq(
      alias($.bold_open, "open"),
      repeat1(
        choice(
          $._word,
          $.italic,
          $.underline,
          $.strikethrough,
          $.verbatim,
        ),
      ),
      alias($.bold_close, "close")
    ),
  ),

  italic: $ => prec.right(
    seq(
      alias($.italic_open, "open"),
      repeat1(
        choice(
          $._word,
          $.bold,
          $.underline,
          $.strikethrough,
          $.verbatim,
        ),
      ),
      alias($.italic_close, "close")
    ),
  ),

  underline: $ => prec.right(
    seq(
      alias($.underline_open, "open"),
      repeat1(
        choice(
          $._word,
          $.bold,
          $.italic,
          $.strikethrough,
          $.verbatim,
        ),
      ),
      alias($.underline_close, "close")
    ),
  ),

  strikethrough: $ => prec.right(
    seq(
      alias($.strikethrough_open, "open"),
      repeat1(
        choice(
          $._word,
          $.bold,
          $.italic,
          $.underline,
          $.verbatim,
        ),
      ),
      alias($.strikethrough_close, "close")
    ),
  ),

  verbatim: $ => prec.right(
    seq(
      alias($.verbatim_open, "open"),
      repeat1($._word),
      alias($.verbatim_close, "close")
    ),
  ),

  inline_math: $ => prec.right(
    seq(
      alias($.inline_math_open, "open"),
      repeat1($._word),
      alias($.inline_math_close, "close")
    ),
  ),
}

function gen_section($, level) {

  let lower_level_sections = []
  for (let i = 0; i + 1 + level <= MAX_HEADING; ++i) {
      lower_level_sections[i] = $["section" + (i + 1 + level)]
  }

  return prec.right(
    seq(
      alias($["heading" + level], $.heading),
      repeat(
        choice(
          alias(choice(...lower_level_sections), $.section),
          $.list,
          $.definition,
          $.code_block,
          $.ranged_tag,
          $.hashtag,
          $.paragraph,
          $._blank_line,
          $.escaped_sequence
        )
      ),
      optional($._soft_break)
    )
  );
}

function gen_heading($, level) {
  return seq(
    alias($["heading_" + level + "_prefix"], $.token),
    alias($.paragraph, $.title)
  );
}

function gen_list($, level, ordered = false) {
  let token = []
  token[0] = alias($["list_" + level + "_prefix"], $.token)
  if (ordered)
    token[1] = alias($.ordered_list_label, $.label )

  let next_level_list = []
  if (level < MAX_LIST_LEVEL) {
    next_level_list[0] = optional(
      choice(
        repeat1($["unordered_list_" + (level + 1)]),
        repeat1($["ordered_list_" + (level + 1)])
      )
    )
  }

  return prec.right(1,
    seq(
      ...token,
      optional($.checkbox),
      repeat1(
        choice(
          $.code_block,
          $.ranged_tag,
          $.hashtag,
          $.paragraph,
          $._blank_line,
          $.escaped_sequence
        )
      ),
      ...next_level_list
    )
  );
}

Object.assign(skald_grammar.rules, sections, lists, markup)

module.exports = grammar(skald_grammar)

// vim: ts=2 sts=2 sw=2
