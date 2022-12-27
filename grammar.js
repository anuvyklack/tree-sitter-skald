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

    $.definition_term_begin,
    $.definition_term_end,
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
    $.checkbox_discarded, // [_]

    $.markdown_code_block_token,
    $.tag_token,
    $.extended_tag_token,
    $.tag_name,
    $.end_tag_name,
    $.hashtag_token,
    $.tag_parameter,

    $.inline_tag_token,
    $.inline_tag_label_open,
    $.inline_tag_label_close,
    $.inline_tag_parameters_open,
    $.inline_tag_parameters_close,

    $.link_label_open,
    $.link_label_close,

    $.link_label2_open,
    $.link_label2_close,

    $.link_location_open,
    $.link_location_close,

    $.blank_line,
    $.soft_break,
    $.hard_break,

    $.escape_char,
    $.comment_token,

    $._word,
    $.raw_word,

    $.new_line,

    $._, // none
  ],

  // conflicts: $ => [
  //   [$.verbatim_tag_content],
  // ],

  inline: $ => [
    $.section,
  ],

  // https://github.com/tree-sitter/tree-sitter/pull/939
  precedences: _ => [
    ['link', 'long_link_reference', 'short_link_reference'],
  ],

  rules: {
    document: $ => repeat1(
      choice(
        alias($.section, $.section),
        $.list,
        $.definition,
        // $.markdown_code_block,
        alias($.verbatim_tag, $.tag),
        $.tag,
        $.hashtag,
        $.paragraph,
        $.blank_line,
        $.hard_break
    )),

    paragraph: $ => prec.right(
      repeat1(choice(
          $.comment,
          $._word,
          $.escaped_sequence,
          $.bold, $.italic, $.strikethrough, $.underline, $.verbatim, $.inline_math,
          $.link, $.link_reference, $.short_link_reference,
          $.inline_tag
      )),
    ),

    section: $ => choice(
      $.section1, $.section2, $.section3, $.section4, $.section5, $.section6),

    definition: $ => seq(
      field("term_open", alias($.definition_term_begin, $.token)),
      alias($.paragraph, $.term),
      field("term_close", alias($.definition_term_end, $.token)),

      repeat(
        seq(
          optional( alias($.paragraph, $.description)),
          field("term_open", alias($.definition_term_begin, $.token)),
          alias($.paragraph, $.term),
          field("term_close", alias($.definition_term_end, $.token)))),

      alias(
        repeat(choice(
            $.paragraph,
            $.blank_line,
            $.list,
            // $.markdown_code_block,
            alias($.verbatim_tag, $.tag),
            $.tag,
            $.hashtag
        )),
        $.description),

      field("end", alias($.definition_end, $.token))
    ),

    list: $ => seq(
      choice(
        repeat1(
          alias($.item_1, $.item)),
        repeat1(
          alias($.ordered_item_1, $.item))),
      alias($.soft_break, $.list_break)
    ),

    // markdown_code_block: $ => seq(
    //   field("open",
    //     alias($.markdown_code_block_token, $.token)
    //   ),
    //   optional(
    //     alias($.tag_parameter, $.language)
    //   ),
    //   optional(
    //     alias($.raw_content, $.content)
    //   ),
    //   field("close",
    //     alias($.markdown_code_block_token, $.token)
    //   ),
    // ),

    inline_tag: $ => seq(
      field("token", alias($.inline_tag_token, $.token)),
      field("name", $.tag_name),

      optional(
        seq(
          field("open_label",
                alias($.inline_tag_label_open, $.token)),
          optional(alias($.link_label, $.label)),
          field("close_label",
                alias($.inline_tag_label_close, $.token)))),

      optional(
        seq(
          field("open_content",
                alias($.link_location_open, $.token)),
          alias( repeat( alias($.raw_word, "raw_word")),
                 $.content),
          field("close_content",
                alias($.link_location_close, $.token)))),

      optional(
        seq(
          field("open_parameters",
                alias($.inline_tag_parameters_open, $.token)),
          alias( repeat(alias($.raw_word, "raw_word")),
                 $.parameters),
          field("close_parameters",
                alias($.inline_tag_parameters_close, $.token)))),
    ),

    // The content of this tag will be parsed by this parser
    tag: $ => seq(
      field("token", alias($.extended_tag_token, $.extended_token)),
      field("name", $.tag_name),
      field("parameter", repeat($.tag_parameter)),
      optional(
        alias( repeat(choice(
                   $.list,
                   $.definition,
                   // $.markdown_code_block,
                   alias($.verbatim_tag, $.tag),
                   $.tag,
                   $.hashtag,
                   $.paragraph,
                   $.blank_line)),
               $.content)),
      $.end_tag
    ),

    // The content of this tag this sitter parser will skip.
    verbatim_tag: $ => seq(
      field("token", alias($.tag_token, $.token)),
      field("name", $.tag_name),
      field("parameter", repeat($.tag_parameter)),
      optional( // content
         alias(repeat1(choice(
                   alias($.raw_word, "raw_word"),
                   alias($.blank_line, "blank_line"),
                   alias($.verbatim_tag, $.tag),
                   $.tag )),
               $.content)),
      $.end_tag
    ),

    end_tag: $ => seq(
      alias($.tag_token, "token"),
      alias($.end_tag_name, "tag_name")
    ),

    hashtag: $ => seq(
      alias($.hashtag_token, $.token),
      $.tag_name,
      repeat($.tag_parameter)
    ),

    // single line comment
    comment: $ => prec.right(
      repeat1( seq(
          alias($.comment_token, "comment"),
          repeat(alias($.tag_parameter, "raw_word"))))
    ),

    escaped_sequence: $ => seq(
      alias($.escape_char, $.token),
      $.raw_word
    ),

    link: $ => prec('link',
      seq(
        field("open_label", alias($.link_label_open, $.token)),
        optional(alias($.link_label, $.label)),
        field("close_label", alias($.link_label_close, $.token)),

        field("open_location", alias($.link_location_open, $.token),),
        alias(repeat(
                 alias($.raw_word, "raw_word")),
              $.location),
        field("close_location", alias($.link_location_close, $.token))
    )),

    link_reference: $ => prec('long_link_reference',
      seq(
        field("open_label", alias($.link_label_open, $.token)),
        alias($.link_label, $.label),
        field("close_label", alias($.link_label_close, $.token)),

        field("open_reference", alias($.link_label2_open, $.token)),
        optional(
           alias($.link_label, $.reference)),
        field("close_reference", alias($.link_label2_close, $.token))
    )),

    short_link_reference: $ => prec('short_link_reference',
      seq(
        field("open_reference", alias($.link_label_open, $.token)),
        alias($.link_label, $.reference),
        field("close_reference", alias($.link_label_close, $.token))
    )),

    link_label: $ => repeat1(
      choice(
        $._word,
        $.escaped_sequence,
        $.bold, $.italic, $.strikethrough, $.underline, $.verbatim, $.inline_math,
    )),

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
        alias($.checkbox_uncertain, $.uncertain),
        alias($.checkbox_discarded, $.discarded)
      )),
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
  item_10: $ => gen_list_item($, 10),

  ordered_item_1: $ => gen_list_item($, 1, true),
  ordered_item_2: $ => gen_list_item($, 2, true),
  ordered_item_3: $ => gen_list_item($, 3, true),
  ordered_item_4: $ => gen_list_item($, 4, true),
  ordered_item_5: $ => gen_list_item($, 5, true),
  ordered_item_6: $ => gen_list_item($, 6, true),
  ordered_item_7: $ => gen_list_item($, 7, true),
  ordered_item_8: $ => gen_list_item($, 8, true),
  ordered_item_9: $ => gen_list_item($, 9, true),
  ordered_item_10: $ => gen_list_item($, 10, true),
}

const markup = {
  bold: $ => gen_markup($, "bold",
    [$.italic, $.underline, $.strikethrough, $.verbatim, $.inline_math]),

  italic: $ => gen_markup($, "italic",
    [$.bold, $.underline, $.strikethrough, $.verbatim, $.inline_math]),

  underline: $ => gen_markup($, "underline",
    [$.bold, $.italic, $.strikethrough, $.verbatim, $.inline_math]),

  strikethrough: $ => gen_markup($, "strikethrough",
    [$.bold, $.italic, $.underline, $.verbatim, $.inline_math]),

  verbatim: $ => prec.right(
    seq(
      field("open", alias($.verbatim_open, $.token)),
      alias(
        repeat1($._word), $.content),
      field("close", alias($.verbatim_close, $.token)),
    ),
  ),

  inline_math: $ => prec.right(
    seq(
      field("open", alias($.inline_math_open, $.token)),
      alias(
        repeat1($._word), $.content),
      field("close", alias($.inline_math_close, $.token)),
    ),
  ),
}

function gen_markup($, kind, other_kinds) {
  return prec.right(
    seq(
      field("open", alias($[kind + "_open"],
                          $.token)),
      alias(
        repeat1(choice(
            $._word,
            $.escaped_sequence,
            ...other_kinds)),
        $.content),

      field("close", alias($[kind + "_close"],
                           $.token))
    ));
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
          alias(choice(...lower_level_sections),
                $.section),
          $.list,
          $.definition,
          // $.markdown_code_block,
          alias($.verbatim_tag, $.tag),
          $.tag,
          $.hashtag,
          $.paragraph,
          $.blank_line)),
      optional($.soft_break)
    ));
}

function gen_heading($, level) {
  return seq(
    field("level", alias($["heading_" + level + "_token"],
                         $["token" + level])),
    field("title", $.paragraph)
  );
}

function gen_list_item($, level, ordered = false) {
  let token = []
  token[0] = field("level",
    alias(
      $["list_" + level + "_token"],
      $["token" + level]))

  if (ordered)
    token[1] = alias($.ordered_list_label, $.label)

  let next_level_list = []
  if (level < MAX_LIST_LEVEL) {
    next_level_list[0] = optional(
      alias(
        choice(
          repeat1( alias($["item_" + (level + 1)],
                         $.item)),
          repeat1( alias($["ordered_item_" + (level + 1)],
                         $.item))
        ),
        $.list)
    )
  }

  return prec.right(1,
    seq(
      ...token,
      optional($.checkbox),
      repeat1( choice(
          // $.markdown_code_block,
          alias($.verbatim_tag, $.tag),
          $.tag,
          $.hashtag,
          $.paragraph,
          $.blank_line)
      ),
      ...next_level_list));
}

Object.assign(skald_grammar.rules, sections, lists, markup)

module.exports = grammar(skald_grammar)

// vim: ts=2 sts=2 sw=2
