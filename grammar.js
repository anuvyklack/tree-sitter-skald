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

    $.heading_1_token,
    $.heading_2_token,
    $.heading_3_token,
    $.heading_4_token,
    $.heading_5_token,
    $.heading_6_token,

    $.definition_begin,
    $.definition_separator,
    $.definition_end,

    $.list_1_token,
    $.list_2_token,
    $.list_3_token,
    $.list_4_token,
    $.list_5_token,
    $.list_6_token,
    $.list_7_token,
    $.list_8_token,
    $.list_9_token,
    $.list_10_token,

    $.ordered_list_label,

    $.checkbox_open,
    $.checkbox_close,

    $.checkbox_undone,    // [ ]
    $.checkbox_done,      // [x]
    $.checkbox_pending,   // [-]
    $.checkbox_urgent,    // [!]
    $.checkbox_uncertain, // [?]

    $.code_block_begin,
    $.ranged_tag_begin,
    $.ranged_tag_end,
    $.hashtag_begin,
    $.tag_parameter,

    $.blank_line,
    $.soft_break,
    $.hard_break,

    $.escape_char,
    $.comment_token,

    $._word,
    $.raw_word,

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
        $.comment,
        $.blank_line,
        $.hard_break,
      )
    ),

    paragraph: $ => prec.right(
      repeat1(
        choice(
          $._word,
          $.escaped_sequence,
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
      field("open", alias($.definition_begin, $.token)),

      alias($.paragraph, $.term),

      field("separator", alias($.definition_separator, $.token)),

      alias(
        repeat1(
          choice(
            $.paragraph,
            $.comment,
            $.blank_line
          )
        ),
        $.description
      ),

      field("close", alias($.definition_end, $.token))
    ),

    list: $ => seq(
      choice(
        field("item", repeat1($.item_1)),
        field("item", repeat1($.ordered_item_1))
      ),
      alias($.soft_break, $.list_break)
    ),

    code_block: $ => seq(
      alias($.code_block_begin, $.tag),
      optional(
        field("language", $.tag_parameter)
      ),
      alias(
        repeat( alias($.raw_word, "raw_word") ),
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
          $.blank_line,
        )
      ),
      alias($.ranged_tag_end, $.end_tag)
    ),

    // single line comment
    comment: $ => prec.right(
      repeat1(
        seq(
          alias($.comment_token, "comment"),
          repeat( alias($.tag_parameter, "raw_word") )
        )
      )
    ),

    hashtag: $=> seq(
      alias($.hashtag_begin, $.tag),
      repeat($.tag_parameter)
    ),

    escaped_sequence: $=> seq(
      alias($.escape_char, $.token),
      $.raw_word
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
    field("open", alias($.checkbox_open, $.token)),
    field("status",
      choice(
        alias($.checkbox_undone,    $.undone),
        alias($.checkbox_done,      $.done),
        alias($.checkbox_pending,   $.pending),
        alias($.checkbox_urgent,    $.urgent),
        alias($.checkbox_uncertain, $.uncertain)
      )
    ),
    field("close", alias($.checkbox_close, $.token))
  ),

  item_1: $ => gen_list_item($, 1),
  item_2: $ => gen_list_item($, 2),
  item_3: $ => gen_list_item($, 3),
  item_4: $ => gen_list_item($, 4),
  item_5: $ => gen_list_item($, 5),
  item_6: $ => gen_list_item($, 6),
  item_7: $ => gen_list_item($, 7),
  item_8: $ => gen_list_item($, 8),
  item_9: $ => gen_list_item($, 9),
  item_10:$ => gen_list_item($,10),

  ordered_item_1: $ => gen_list_item($, 1, true),
  ordered_item_2: $ => gen_list_item($, 2, true),
  ordered_item_3: $ => gen_list_item($, 3, true),
  ordered_item_4: $ => gen_list_item($, 4, true),
  ordered_item_5: $ => gen_list_item($, 5, true),
  ordered_item_6: $ => gen_list_item($, 6, true),
  ordered_item_7: $ => gen_list_item($, 7, true),
  ordered_item_8: $ => gen_list_item($, 8, true),
  ordered_item_9: $ => gen_list_item($, 9, true),
  ordered_item_10:$ => gen_list_item($,10, true),
}

const markup = {
  bold: $ => prec.right(
    seq(
      field("open", alias($.bold_open, $.token)),
      repeat1(
        choice(
          $._word,
          $.italic,
          $.underline,
          $.strikethrough,
          $.verbatim,
        ),
      ),
      field("close" , alias($.bold_close, $.token))
    ),
  ),

  italic: $ => prec.right(
    seq(
      field("open", alias($.italic_open, $.token)),
      repeat1(
        choice(
          $._word,
          $.bold,
          $.underline,
          $.strikethrough,
          $.verbatim,
        ),
      ),
      field("close", alias($.italic_close, $.token)),
    ),
  ),

  underline: $ => prec.right(
    seq(
      field("open", alias($.underline_open, $.token)),
      repeat1(
        choice(
          $._word,
          $.bold,
          $.italic,
          $.strikethrough,
          $.verbatim,
        ),
      ),
      field("close", alias($.underline_close, $.token)),
    ),
  ),

  strikethrough: $ => prec.right(
    seq(
      field("open", alias($.strikethrough_open, $.token)),
      repeat1(
        choice(
          $._word,
          $.bold,
          $.italic,
          $.underline,
          $.verbatim,
        ),
      ),
      field("close", alias($.strikethrough_close, $.token)),
    ),
  ),

  verbatim: $ => prec.right(
    seq(
      field("open", alias($.verbatim_open, $.token)),
      repeat1($._word),
      field("close", alias($.verbatim_close, $.token)),
    ),
  ),

  inline_math: $ => prec.right(
    seq(
      field("open", alias($.inline_math_open, $.token)),
      repeat1($._word),
      field("close", alias($.inline_math_close, $.token)),
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
          $.comment,
          $.blank_line,
        )
      ),
      optional($.soft_break)
    )
  );
}

function gen_heading($, level) {
  return seq(
    alias($["heading_" + level + "_token"], $.token),
    field("title", $.paragraph)
  );
}

function gen_list_item($, level, ordered = false) {
  let token = []
  token[0] = field("level",
    alias($["list_" + level + "_token"], $.token)
  )
  if (ordered)
    token[1] = alias($.ordered_list_label, $.label )

  let next_level_list = []
  if (level < MAX_LIST_LEVEL) {
    next_level_list[0] = optional(
      alias(
        choice(
          repeat1($["item_" + (level + 1)]),
          repeat1($["ordered_item_" + (level + 1)])
        ),
        $.list
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
          $.comment,
          $.blank_line,
        )
      ),
      ...next_level_list
    )
  );
}

Object.assign(skald_grammar.rules, sections, lists, markup)

module.exports = grammar(skald_grammar)

// vim: ts=2 sts=2 sw=2
